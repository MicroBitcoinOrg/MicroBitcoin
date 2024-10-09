// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <deploymentinfo.h>
#include <hash.h> // for signet block challenge hash
#include <util/system.h>

#include <snapshot.h>
#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
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

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, std::vector<SnapshotEntry>& vSnapshot)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(vSnapshot.size() + 1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    for (unsigned int i = 0; i < vSnapshot.size(); ++i)
    {
        txNew.vout[i + 1].nValue = vSnapshot[i].amount;
        txNew.vout[i + 1].scriptPubKey = vSnapshot[i].script;
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

/**
 * Main network on which people trade goods and services.
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();

        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;

        consensus.powLimit = uint256S("003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1634541943;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 1056905;

        // Supply hardfork
        consensus.nSubsidyAddress = "";
        consensus.nSubsidyHeight = 99999999999;
        consensus.nSubsidyAmount = 1000 * COIN;

        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000054d62a0d6408");
        consensus.defaultAssumeValid = uint256S("0x23eef88fbc540be75a7b61188bcf118c29db9d4f5fa663863d0458f74ba675b2"); // 1056000

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
        m_assumed_blockchain_size = 420;
        m_assumed_chain_state_size = 6;

        consensus.lwmaAveragingWindow = 90;
        consensus.baseReward = 5500 * COIN;

        // Decrease reward by 30% each 2 years
        consensus.rewardEpoch = 525960 * 2; 
        consensus.rewardEpochRate = 0.3;

        const char* pszTimestamp = "The WSJ 09/Oct/2019 Nobel Prize in Chemistry Awarded to Developers of Lithium-Ion Batteries";
        std::vector<SnapshotProvider> providers = {
            {"http://snapshot.microbitcoin.org", "/mainnet.csv"},
            {"http://micro.codepillow.io", "/mainnet.csv"},
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

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_main), std::end(chainparams_seed_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 10, uint256S("0xeb419dcd2e22d73aeb7dcaf798fb17f06698f920b690ed03ede27406a0020ecb")},
                { 1000, uint256S("0xa7a83700b5cd157c4c7a6f4ceea38f27bd2ec7282d1811634d8ea95ed693d4d8")},
                { 100000, uint256S("0xfa30671130a2cad47cb18b89edeb44f897e2686da4c05af6e1452076af5df9a0")},
                { 200000, uint256S("0xa280e163ee97a474d6838f74d746012216cea1e3ab5bff04d65098128a0c1634")},
                { 300000, uint256S("0x8a4b61320029aa93498c1637a91dfed2c49118ab34d4ba73e269a32430d6e245")},
                { 400000, uint256S("0x6dac5851eadbc34b3d80171491743e6e4421f10b4a599c923585be4fdf101ba3")},
                { 500000, uint256S("0x31c4118122391706475cf0c5ee445f8dee1f5bb7c2b92041776e8cef55bd1307")},
                { 600000, uint256S("0x417a18f0afd2ac52c0f738d3f12b0b3dc1dc7c39cca531073b3148d6bce91829")},
                { 700000, uint256S("0xb1c25846f387c9a408ccecfe7919e946b53974e2131416e1514c1b3676a15b7b")},
                { 800000, uint256S("0x038389a836e6f90e0f379bdc6a0c74d841eb6dbec8b23e2b335a981be3d0a56f")},
                { 900000, uint256S("0xb7381d1d7c281eec55fa985fd28bdb0c3bf5373a936ea37b3d861a7520d128f4")},
                { 1000000, uint256S("0x849d3a7f266d3ba3ebce81c20f467f532832652a86e6495b03ec77985535efc9")},
                { 2000000, uint256S("0xa4ecd2833734ce7bb8fb1af7ae53bc33c4bf986dbc7eb87d8d01d6873fdb7ded")},
                { 2600000, uint256S("0x23eef88fbc540be75a7b61188bcf118c29db9d4f5fa663863d0458f74ba675b2")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
         // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 23eef88fbc540be75a7b61188bcf118c29db9d4f5fa663863d0458f74ba675b2
            /* nTime    */ 1727905204,
            /* nTxCount */ 3043896,
            /* dTxRate  */ 0.01708408484745776
        };
    }
};

/**
 * Testnet: public test network which is reset from time to time.
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();

        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;

        consensus.powLimit = uint256S("003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1619222400; // April 24th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = 1628640000; // August 11th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Supply hardfork
        consensus.nSubsidyAddress = "";
        consensus.nSubsidyHeight = 99999999999;
        consensus.nSubsidyAmount = 1000 * COIN;

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000005180c3bd8290da33a1a");
        consensus.defaultAssumeValid = uint256S("0x0000000000004ae2f3896ca8ecd41c460a35bf6184e145d91558cece1c688a76"); // 2010000

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
        m_assumed_blockchain_size = 40;
        m_assumed_chain_state_size = 2;

        const char* pszTimestamp = "The WSJ 05/Oct/2019 Hong Kong Shuts Down After Night of Violence";

        vSnapshot = EmptySnapshot();
        genesis = CreateGenesisBlock(1634445073, 2131, 0x1f3fffff, 1, consensus.baseReward, pszTimestamp, vSnapshot);

        consensus.hashGenesisBlock = genesis.GetIndexHash();
        consensus.hashGenesisBlockWork = genesis.GetWorkHash();

        assert(consensus.hashGenesisBlock == uint256S("0xd36a04fdef89fe61faf23f84ba930308130adb83a44a905de9c07c46c46d1a6d"));
        assert(consensus.hashGenesisBlockWork == uint256S("0x002ccba2978484648cc5b9ebd95a277fa2d26a56e29e787279e14452cc195fb5"));
        assert(genesis.hashMerkleRoot == uint256S("0xeac469c73c951cbeab7f4cd074f3b280dbcf5027d53e04cc4f9cb50028e43af6"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,26);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,51);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "tmbc";

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_test), std::end(chainparams_seed_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 0000000000004ae2f3896ca8ecd41c460a35bf6184e145d91558cece1c688a76
            /* nTime    */ 0,
            /* nTxCount */ 0,
            /* dTxRate  */ 0,
        };
    }
};

/**
 * Signet: test network with an additional consensus parameter (see BIP325).
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            vSeeds.emplace_back("178.128.221.177");
            vSeeds.emplace_back("2a01:7c8:d005:390::5");
            vSeeds.emplace_back("v7ajjeirttkbnt32wpy3c6w3emwnfr3fkla7hpxcfokr3ysd3kqtzmqd.onion:38333");

            consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000008546553c03");
            consensus.defaultAssumeValid = uint256S("0x000000187d4440e5bff91488b700a140441e089a8aaea707414982460edbfe54"); // 47200
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 000000187d4440e5bff91488b700a140441e089a8aaea707414982460edbfe54
                /* nTime    */ 1626696658,
                /* nTxCount */ 387761,
                /* dTxRate  */ 0.04035946932424404,
            };
        } else {
            const auto signet_challenge = args.GetArgs("-signetchallenge");
            if (signet_challenge.size() != 1) {
                throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
            }
            bin = ParseHex(signet_challenge[0]);

            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", signet_challenge[0]);
        }

        if (args.IsArgSet("-signetseednode")) {
            vSeeds = args.GetArgs("-signetseednode");
        }

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());

        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016;

        consensus.powLimit = uint256S("003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // message start is defined as the first 4 bytes of the sha256d of the block script
        CHashWriter h(SER_DISK, 0);
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38333;
        nPruneAfterHeight = 1000;

        consensus.lwmaAveragingWindow = 90;
        consensus.baseReward = 5500 * COIN;

        // Decrease reward by 30% each 2 years
        consensus.rewardEpoch = 525960 * 2; 
        consensus.rewardEpochRate = 0.3;

        const char* pszTimestamp = "The WSJ 09/Oct/2019 Nobel Prize in Chemistry Awarded to Developers of Lithium-Ion Batteries";
        vSnapshot = EmptySnapshot();

        genesis = CreateGenesisBlock(1598918400, 1487, 0x1f3fffff, 1, consensus.baseReward, pszTimestamp, vSnapshot);
        consensus.hashGenesisBlock = genesis.GetIndexHash();

        assert(consensus.hashGenesisBlock == uint256S("0x83468e58cd2bd0cb30f1b722c84db65029e0cd5718aabe6461294b33da809762"));
        assert(genesis.hashMerkleRoot == uint256S("0xd5322aa9dc80dda1982ba855afe7970bf246589a18f3c5920320849a813eb0fc"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tmbc";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test: intended for private networks only. Has minimal difficulty to ensure that
 * blocks can be found instantly.
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();

        consensus.nBIP34Enabled = true;
        consensus.nBIP65Enabled = true;
        consensus.nBIP66Enabled = true;
        consensus.nSegwitEnabled = true;
        consensus.nCSVEnabled = true;

        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // Supply hardfork
        consensus.nSubsidyAddress = "rmbc1qe6dtjj50gc4fq0k3pal73zargq96c9mgvljrd6";
        consensus.nSubsidyHeight = 200;
        consensus.nSubsidyAmount = 1000 * COIN;

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = args.GetBoolArg("-fastprune", false) ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        consensus.lwmaAveragingWindow = 90;
        consensus.baseReward = 5500 * COIN;

        // Decrease reward by 30% each 2 years
        consensus.rewardEpoch = 525960 * 2; 
        consensus.rewardEpochRate = 0.3;

        const char* pszTimestamp = "Cretaceous Bird-Like Dinosaur Had Adaptations for Swimming and Diving | Sci-News Dec 2, 2022";

        vSnapshot = EmptySnapshot();

        genesis = CreateGenesisBlock(1670163306, 1, 0x207fffff, 1, consensus.baseReward, pszTimestamp, vSnapshot);
        consensus.hashGenesisBlock = genesis.GetIndexHash();

        assert(consensus.hashGenesisBlock == uint256S("0x809f50088e594b701c0c9a3377cb165255ad621ea2664ee41b164ca28a899722"));
        assert(genesis.hashMerkleRoot == uint256S("0x2c3057ab4ec6d1a7a89079100bcdb9d5e3b17815f878ec007d4c9609c599dbc7"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("809f50088e594b701c0c9a3377cb165255ad621ea2664ee41b164ca28a899722")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{};

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
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int min_activation_height)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        consensus.vDeployments[d].min_activation_height = min_activation_height;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        int64_t nStartTime, nTimeout;
        int min_activation_height = 0;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4 && !ParseInt32(vDeploymentParams[3], &min_activation_height)) {
            throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, min_activation_height);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d\n", vDeploymentParams[0], nStartTime, nTimeout, min_activation_height);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new SigNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}
