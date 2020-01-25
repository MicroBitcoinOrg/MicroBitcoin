// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2019 MicroBitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>

#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>
#include <snapshot.h>
#include <univalue.h>

#include <assert.h>
#include <chainparamsseeds.h>
#include <arith_uint256.h>

void GenesisGenerator(CBlock genesis) {
    printf("Searching for genesis block...\n");

    uint256 hash;
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow);

    while(true)
    {
        hash = genesis.GetWorkHash();
        if (UintToArith256(hash) <= bnTarget)
            break;
        if ((genesis.nNonce & 0xFFF) == 0)
        {
            printf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, hash.ToString().c_str(), bnTarget.ToString().c_str());
        }
        ++genesis.nNonce;
        if (genesis.nNonce == 0)
        {
            printf("NONCE WRAPPED, incrementing time\n");
            ++genesis.nTime;
        }
    }

    printf("block.nNonce = %u \n", genesis.nNonce);
    printf("block.GetIndexHash = %s\n", genesis.GetIndexHash().ToString().c_str());
    printf("block.GetWorkHash = %s\n", genesis.GetWorkHash().ToString().c_str());
    printf("block.MerkleRoot = %s \n", genesis.hashMerkleRoot.ToString().c_str());
}

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, std::vector<SnapshotEntry> vSnapshot)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(vSnapshot.size() + 1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    int i = 1;
    for (auto const& tx: vSnapshot) {
        txNew.vout[i].nValue = tx.amount;
        txNew.vout[i].scriptPubKey = tx.script;
        i++;
    }

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. It includes snapshot coins from vSnapshot
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, const char* pszTimestamp, std::vector<SnapshotEntry> vSnapshot)
{
    const CScript genesisOutputScript = CScript() << ParseHex("04cb16c90fdcd962e9b78b2b09c78b76b67ea205cb93efa8772c2df2ab265cbc1b3bb4e6b16f77378b768d293de62e6d14a4b348c679060fa43a44bfa78a2f4411") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward, vSnapshot);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams(const ArgsManager& args) {
        strNetworkID = "main";
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;
        consensus.powLimit = uint256S("003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        consensus.lwmaAveragingWindow = 90;
        consensus.baseReward = 5500 * COIN;

        // Decrease reward by 30% each 2 years
        consensus.rewardEpoch = 525960 * 2; 
        consensus.rewardEpochRate = 0.3;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x73;
        pchMessageStart[1] = 0x6d;
        pchMessageStart[2] = 0x62;
        pchMessageStart[3] = 0x63;
        nDefaultPort = 6502;
        nPruneAfterHeight = 100000;

        const char* pszTimestamp = "The WSJ 09/Oct/2019 Nobel Prize in Chemistry Awarded to Developers of Lithium-Ion Batteries";
        std::vector<SnapshotProvider> providers = {
            {"sman.pw", "/snapshot/mainnet.csv", 80}
        };

        vSnapshot = InitSnapshot("mainnet.csv", providers);
        genesis = CreateGenesisBlock(1570625829, 709, 0x1f3fffff, 1, consensus.baseReward, pszTimestamp, vSnapshot);

        consensus.hashGenesisBlock = genesis.GetIndexHash();
        consensus.hashGenesisBlockWork = genesis.GetWorkHash();

        assert(consensus.hashGenesisBlock == uint256S("0x14c03ecf20edc9887fb98bf34b53809f063fc491e73f588961f764fac88ecbae"));
        assert(consensus.hashGenesisBlockWork == uint256S("0x001cb6047ddf13074c4bce354ed3cf0cdd96a4287aa562b032eb81d03e183da8"));
        assert(genesis.hashMerkleRoot == uint256S("0x3426ccad3017e14a4ab6efddaa44cb31beca67a86c82f63de18705f1b6de88df"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,26);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,51);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "mbc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = {
            {

            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        /* enable fallback fee on mainnet */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Testnet
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams(const ArgsManager& args) {
        strNetworkID = "test";
        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;
        consensus.powLimit = uint256S("003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        consensus.lwmaAveragingWindow = 90;
        consensus.baseReward = 5500 * COIN;

        // Decrease reward by 30% each 2 years
        consensus.rewardEpoch = 525960 * 2; 
        consensus.rewardEpochRate = 0.3;

        pchMessageStart[0] = 0x74;
        pchMessageStart[1] = 0x6d;
        pchMessageStart[2] = 0x62;
        pchMessageStart[3] = 0x63;
        nDefaultPort = 16502;
        nPruneAfterHeight = 1000;

        const char* pszTimestamp = "The WSJ 05/Oct/2019 Hong Kong Shuts Down After Night of Violence";
        std::vector<SnapshotProvider> providers = {
            {"sman.pw", "/snapshot/testnet.csv", 80}
        };

        vSnapshot = InitSnapshot("testnet.csv", providers);
        genesis = CreateGenesisBlock(1568015489, 135893, 0x1f3fffff, 1, consensus.baseReward, pszTimestamp, vSnapshot);

        consensus.hashGenesisBlock = genesis.GetIndexHash();
        consensus.hashGenesisBlockWork = genesis.GetWorkHash();

        assert(consensus.hashGenesisBlock == uint256S("0xf6968578ea5b34f3fcbdb55cdf75e4a7bd1534dd8dc65de09a250d808686f05f"));
        assert(consensus.hashGenesisBlockWork == uint256S("0x002089a79759f268e63e8ffc3f90c601004b442e5191a0cf2874d1985aee6b0a"));
        assert(genesis.hashMerkleRoot == uint256S("0x0fb9c5ab92544b3e5b3af90567a75dec488dc89087c70ead7b6eabedc032d8b0"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,71);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,73);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "tmbc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = {
            {

            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        /* enable fallback fee on testnet */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams(const ArgsManager& args) {
        strNetworkID = "regtest";
        consensus.nBIP34Enabled = false;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        consensus.lwmaAveragingWindow = 90;
        consensus.baseReward = 5500 * COIN;

        // Decrease reward by 30% each 2 years
        consensus.rewardEpoch = 525960 * 2; 
        consensus.rewardEpochRate = 0.3;

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 26502;
        nPruneAfterHeight = 1000;

        const char* pszTimestamp = "The WSJ 10/Sep/2019 There’s Too Much Negativity About Negative Rates";
        std::vector<SnapshotProvider> providers = {
            {"sman.pw", "/snapshot/regtest.csv", 80}
        };

        vSnapshot = InitSnapshot("regtest.csv", providers);
        genesis = CreateGenesisBlock(1579961426, 11, 0x207fffff, 1, 900 * COIN, pszTimestamp, vSnapshot);
        
        consensus.hashGenesisBlock = genesis.GetIndexHash();
        consensus.hashGenesisBlockWork = genesis.GetWorkHash();

        assert(consensus.hashGenesisBlock == uint256S("0xb568d70ab1f31a49d996613bcb1bdaeed10060619a806321e13f24502bc08583"));
        assert(consensus.hashGenesisBlockWork == uint256S("0x3df77bfd6bf9f910e5dc849a7218e51e784648d93a25963ac24052a123500039"));
        assert(genesis.hashMerkleRoot == uint256S("0xeee9c2aee9997d25398dfbc9bc3dc45121381381db6b630330855181d44a760d"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {

            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "rmbc";

        /* enable fallback fee on regtest */
        m_fallback_fee_enabled = true;
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams(gArgs));
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams(gArgs));
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
