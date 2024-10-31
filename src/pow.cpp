// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2018-2019 MicroMicroBitcoin developers
// Copyright (c) 2018 Zawy
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

unsigned int Lwma3CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow;
    const int64_t k = N * (N + 1) * T / 2;
    const int64_t height = pindexLast->nHeight;

    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    if (height < N) { return powLimit.GetCompact(); }

    // Subsidy hardfork
    // Here we reset the diff in order to guarantee smooth experience
    if (height >= params.nSubsidyHeight && height < params.nSubsidyHeight + N)
        return powLimit.GetCompact();

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

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    return Lwma3CalculateNextWorkRequired(pindexLast, params);
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
