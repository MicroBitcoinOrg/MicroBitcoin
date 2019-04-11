// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2018-2019 MicroBitcoin developers
// Copyright (c) 2017-2018 The Bitcoin Gold developers
// Copyright (c) 2018 Zawy
// Copyright (c) 2019 Romeo Micev (Implementation of the RainForest V2 PoW Algorithm)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "chainparams.h"
#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"

#include "bignum.h"

// Rejected in favor of LWMA2
// https://github.com/zawy12/difficulty-algorithms/issues/31 - DGW issues
// https://github.com/zawy12/difficulty-algorithms/issues/3 - LWMA2 description
unsigned int static DarkGravityWave3(const CBlockIndex* pindexLast, const Consensus::Params& params) {
    /* current difficulty formula, darkcoin - DarkGravity v3, written by Evan Duffield - evan@darkcoin.io */
    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        // This is the first block or the height is < PastBlocksMin
        // Return minimal required work. (1e0fffff)
        return UintToArith256(params.powLimit).GetCompact(); 
    }
    
    // loop over the past n blocks, where n == PastBlocksMax
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        CountBlocks++;

        // Calculate average difficulty based on the blocks we iterate over in this for loop
        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) { PastDifficultyAverage.SetCompact(BlockReading->nBits); }
            else { PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks)+(arith_uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1); }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        // If this is the second iteration (LastBlockTime was set)
        if(LastBlockTime > 0){
            // Calculate time difference between previous block and current block
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            // Increment the actual timespan
            nActualTimespan += Diff;
        }
        // Set LasBlockTime to the block time for the block in current iteration
        LastBlockTime = BlockReading->GetBlockTime();      

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }
    
    // bnNew is the difficulty
    arith_uint256 bnNew(PastDifficultyAverage);
    const auto nTargetSpacing = params.nPowTargetSpacing;

    // nTargetTimespan is the time that the CountBlocks should have taken to be generated.
    int64_t nTargetTimespan = CountBlocks * nTargetSpacing;

    // Limit the re-adjustment to 3x or 0.33x
    // We don't want to increase/decrease diff too much.
    if (nActualTimespan < nTargetTimespan / 3)
        nActualTimespan = nTargetTimespan / 3;
    if (nActualTimespan > nTargetTimespan * 3)
        nActualTimespan = nTargetTimespan * 3;

    // Calculate the new difficulty based on actual and target timespan.
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    // If calculated difficulty is lower than the minimal diff, set the new difficulty to be the minimal diff.
    if (bnNew > UintToArith256(params.powLimit)) {
        bnNew = UintToArith256(params.powLimit);
    }

    // Return the new diff.
    return bnNew.GetCompact();
}

unsigned int Lwma2CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow;
    const int64_t k = N * (N + 1) * T / 2;
    const int height = pindexLast->nHeight;
    const arith_uint256 powLimit = UintToArith256(params.powLimitMBC);
    
    if (height < N) { return powLimit.GetCompact(); }

    arith_uint256 sum_target, previous_diff, next_target;
    int64_t t = 0, j = 0, solvetime_sum;

    // Loop through N most recent blocks. 
    for (int i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);
        const CBlockIndex* block_Prev = block->GetAncestor(i - 1);
        int64_t solvetime = block->GetBlockTime() - block_Prev->GetBlockTime();
        solvetime = std::max(-6*T, std::min(solvetime, 6 * T));
        j++;
        t += solvetime * j; // Weighted solvetime sum.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sum_target += target / (k * N);

        if (i > height - 3) {
            solvetime_sum += solvetime;
        }
        
        if (i == height) {
            previous_diff = target.SetCompact(block->nBits); // Latest block target
        }
    }

    // Keep t reasonable to >= 1/4 of expected t.
    if (t < k / 4 ) {
        t = k / 4;
    }
    next_target = t * sum_target;

    // Don't fix this (may cause network split)
    // if (solvetime_sum < (8 * T) / 10) {
    //     next_target = previous_diff * 100 / 106;
    // }

    return next_target.GetCompact();
}

unsigned int Lwma3CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow;
    const int64_t k = N * (N + 1) * T / 2;
    const int64_t height = pindexLast->nHeight;

    arith_uint256 powLimitTmp = UintToArith256(params.powLimitMBC);    
    if (height >= params.rainforestHeightV2) {
        powLimitTmp = UintToArith256(params.powLimitRFv2);
    }

    const arith_uint256 powLimit = powLimitTmp;
    if (height < N) { return powLimit.GetCompact(); }

    arith_uint256 sumTarget, previousDiff, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t t = 0, j = 0, solvetimeSum = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? block->GetBlockTime() : previousTimestamp + 1;

        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);
        previousTimestamp = thisTimestamp;

        j++;
        t += solvetime * j; // Weighted solvetime sum.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sumTarget += target / (k * N);

        if (i > height - 3) { solvetimeSum += solvetime; }
        if (i == height) { previousDiff = target.SetCompact(block->nBits); }
    }

    nextTarget = t * sumTarget;
    
    if (nextTarget > (previousDiff * 150) / 100) { nextTarget = (previousDiff * 150) / 100; }
    if (nextTarget < (previousDiff * 67) / 100) { nextTarget = (previousDiff * 67) / 100; }
    if (solvetimeSum < (8 * T) / 10) { nextTarget = previousDiff * 100 / 106; }
    if (nextTarget > powLimit) { nextTarget = powLimit; }

    return nextTarget.GetCompact();
}

static unsigned int BitcoinNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // Go back by what we want to be 14 days worth of blocks
    const auto difficultyAdjustmentInterval = params.BitcoinDifficultyAdjustmentInterval();
    int nHeightFirst = pindexLast->nHeight - (difficultyAdjustmentInterval-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    int nHeight = pindexLast->nHeight + 1;

    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
    const auto isHardfork = nHeight > params.mbcHeight;
    const auto isLwma2 = nHeight >= params.lwma2Height && nHeight < params.lwma3Height;
    const auto isLwma3 = nHeight >= params.lwma3Height;

    const auto mbcWarmUp = (isHardfork && nHeight < (params.mbcHeight + 1) + params.nWarmUpWindow);
    const auto rainforestWarmUp = (nHeight >= params.rainforestHeight && nHeight < params.rainforestHeight + params.rainforestWarmUpWindow);
    const auto rainforestWarmUpV2 = (nHeight >= params.rainforestHeightV2 && nHeight < params.rainforestHeightV2 + params.rainforestWarmUpWindowV2);

    // PoW warm-up window

    if (rainforestWarmUpV2) {
        return UintToArith256(params.powLimitRFv2).GetCompact();
    } else if (mbcWarmUp || rainforestWarmUp) {
        return UintToArith256(params.powLimitMBC).GetCompact();
    }
    
    if (params.fPowNoRetargeting) return pindexLast->nBits;

    const auto difficultyAdjustmentInterval = isHardfork
        ? params.DifficultyAdjustmentInterval()
        : params.BitcoinDifficultyAdjustmentInterval();

    // Only change once per difficulty adjustment interval
    if (nHeight % difficultyAdjustmentInterval != 0) {
        if (params.fPowAllowMinDifficultyBlocks) {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2 * 10 minutes
            // then allow mining of a min-difficulty block.
            const auto powTargetSpacing = isHardfork ? params.nPowTargetSpacing : params.nBtcPowTargetSpacing;
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + powTargetSpacing * 2) {
                return nProofOfWorkLimit;
            } else {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % difficultyAdjustmentInterval != 0 && pindex->nBits == nProofOfWorkLimit) {
                    pindex = pindex->pprev;
                }

                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    if (isLwma2) {
        return Lwma2CalculateNextWorkRequired(pindexLast, params);
    } else if (isLwma3) {
        return Lwma3CalculateNextWorkRequired(pindexLast, params);
    } else {
        return pindexLast->nHeight > params.mbcHeight
            ? DarkGravityWave3(pindexLast, params)
            : BitcoinNextWorkRequired(pindexLast, pblock, params);
    }
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, int nHeight, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    arith_uint256 bnPowLimitTmp = UintToArith256(params.powLimit);
    if (nHeight >= params.rainforestHeightV2) {
        bnPowLimitTmp = UintToArith256(params.powLimitRFv2);
    } else if (nHeight > params.mbcHeight && nHeight < params.rainforestHeightV2) {
        bnPowLimitTmp = UintToArith256(params.powLimitMBC);
    }

    const arith_uint256 bnPowLimit = bnPowLimitTmp;
    // std::cout << bnPowLimit.ToString() << std::endl;
    
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > bnPowLimit) {
        return false;
    }
    
    if (nHeight >= params.rainforestHeightV2) {
        // RAINFOREST PoW Algorith Implementation
        // Pointer for allocation of the ByteCollusion Memory Space
        uint32_t *byteCollusionSpace = NULL;
        int32_t nBytesCollusion = 0x1f - (nBits >> 24);
        if(nBytesCollusion < 3) {
            nBytesCollusion = 3;
        }

        if (byteCollusionSpace == NULL) {
            // INIT BYTE COLLUSION SPACE USING
            // FIRST 32 BITS OF THE FRACTIONAl PARTS OF THE SQUARE ROOTS OF THE FIRST 1024 PRIMES 3..8167

            byteCollusionSpace = (uint32_t*)calloc(1024, sizeof(uint32_t));
            if(byteCollusionSpace == NULL) {
                LogPrintf("Could not allocate Memory Space for the ByteCollusiion\n");
                return false;
            }

            int32_t i = 3, nPrime, c;
            uint32_t primeSQRootFraction = 0;

            for(nPrime = 0; nPrime < 1024;) {
                for(c = 2; c <= i - 1; c++) {
                    if (i % c == 0) {
                        break;
                    }
                }

                if(c == i) {
                    primeSQRootFraction = (uint32_t)(fmod(sqrt(i), 1.0) * pow(2.0, 32.0));
                    memcpy(byteCollusionSpace+nPrime, &primeSQRootFraction, sizeof(unsigned int));
                    nPrime++;
                }

                i++;
            }
        }

        // CHECK RAINFOREST HASH
        std::cout << "RainForest HASH: " << hash.GetHex() << "\n";

        // Extract Hash
        uint64_t rf_hash[4];
        rf_hash[0] = hash.GetUint64(0);
        rf_hash[1] = hash.GetUint64(1);
        rf_hash[2] = hash.GetUint64(2);
        rf_hash[3] = hash.GetUint64(3);

        //CHECK EXTRACTED RAINFOREST HASH
        std::cout << "Extracted RainForest HASH:\n[0] " << std::hex << rf_hash[0] << "\n[1] " << rf_hash[1] << "\n[2] " << rf_hash[2] << "\n[3] " << rf_hash[3] << "\n";

        uint32_t nBytePos;
        //Loop Maximal 1024 concanated uint32 fraction of primes a4 Bytes minus the ByteCollusion part
        uint32_t nMaxBytesLookup = 1024 * sizeof(uint32_t) - nBytesCollusion;

        // SCAN TROUGH 4096 BYTES OF MEMORY TO CHECK FOR A BYTE COLLUSION WITH nBYTES FROM THE HASH
        for (nBytePos = 0; nBytePos < nMaxBytesLookup; nBytePos++) {
            // CHECK BYTE FOR BYTE IF A nBYTE COLLUSION EXIST IN THE MEM SPACE
            // IF YES SUBMIT SHARE / BLOCK TO NETWORK AS SOLUTION WAS FOUND
            if (!memcmp(&((char *)byteCollusionSpace)[nBytePos], rf_hash, nBytesCollusion)) {
                return true;
            }
        }

        return false;

    } else {
        // Check proof of work matches claimed amount
        if (UintToArith256(hash) > bnTarget) {
            return false;
        }
    }

    return true;
}
