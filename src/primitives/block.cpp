// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2019 MicroBitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <crypto/common.h>
#include <crypto/yespower/yespower.h>
#include <streams.h>
#include <stdlib.h>
#include <sync.h>

uint256 CBlockHeaderUncached::GetIndexHash() const
{
    return Blake2b(BEGIN(nVersion), END(nNonce));
}

uint256 CBlockHeaderUncached::GetWorkHash() const
{
    static const yespower_params_t yespower_microbitcoin = {
        .N = 2048,
        .r = 32,
        .pers = (const uint8_t *)"Now I am become Death, the destroyer of worlds",
        .perslen = 46
    };

    uint256 thash;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *this;

    if (yespower_tls((unsigned char *)&ss[0], ss.size(), &yespower_microbitcoin, (yespower_binary_t *)&thash)) {
        fprintf(stderr, "Error: CBlockHeaderUncached::GetWorkHash(): failed to compute PoW hash (out of memory?)\n");
        exit(1);
    }

    return thash;
}

uint256 CBlockHeader::GetWorkHashCached() const
{
    uint256 indexHash = GetIndexHash();
    LOCK(cacheLock);
    if (cacheInit) {
        if (indexHash != cacheIndexHash) {
            fprintf(stderr, "Error: CBlockHeader: block hash changed unexpectedly\n");
            exit(1);
        }
    } else {
        cacheWorkHash = GetWorkHash();
        cacheIndexHash = indexHash;
        cacheInit = true;
    }
    return cacheWorkHash;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetIndexHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
