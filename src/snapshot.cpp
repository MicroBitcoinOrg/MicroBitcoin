// Copyright (c) 2019-2021 MicroBitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <snapshot.h>
#include <util/time.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <primitives/transaction.h>
#include <support/httplib.h>
#include <support/csv.h>
#include <core_io.h>
#include <random>
#include <fs.h>

std::string TimestampStr() {
    return FormatISO8601DateTime(GetTimeMicros() / 1000000) + " ";
}

CScript ReadScriptSnapshot(const std::string& s) {
    size_t pos = s.find(" ");
    size_t initial_pos = 0;
    std::vector<std::string> splited;
    while(pos != std::string::npos) {
        splited.push_back(s.substr(initial_pos, pos - initial_pos));
        initial_pos = pos + 1;
        pos = s.find(" ", initial_pos);
    }

    splited.push_back(s.substr(initial_pos, std::min(pos, s.size()) - initial_pos + 1));
    if (splited.size() == 5) {
    	// It's p2pkh script
    	// Some sanity checks
    	assert(splited[0] == "OP_DUP");
    	assert(splited[1] == "OP_HASH160");
    	assert(splited[2].length() == 40);
    	assert(splited[3] == "OP_EQUALVERIFY");
    	assert(splited[4] == "OP_CHECKSIG");

    	return CScript() << OP_DUP << OP_HASH160 << ParseHex(splited[2]) << OP_EQUALVERIFY << OP_CHECKSIG;
    } else {
    	// It's p2sh script
    	// Some sanity checks
    	assert(splited[0] == "OP_HASH160");
    	assert(splited[1].length() == 40);
    	assert(splited[2] == "OP_EQUAL");

    	return CScript() << OP_HASH160 << ParseHex(splited[1]) << OP_EQUAL;
    }
}

bool FetchSnapshot(fs::path &path, SnapshotProvider provider) {
    httplib::Client cli(provider.address.c_str());
    auto result = cli.Get(provider.path.c_str());

    if (result && result->status == 200) {
        std::cout << TimestampStr() << "Shapshot: Successfully fetched snapshot file" << std::endl;
        FILE *snapshot_file = fsbridge::fopen(path, "w");
        fwrite(result->body.data(), 1, result->body.size(), snapshot_file);
        fclose(snapshot_file);
        return true;
    } else {
        std::cout << TimestampStr() << "Shapshot: Failed to fetch snapshot file from the server" << std::endl;
        return false;
    }
}

std::vector<SnapshotEntry> LoadSnapshot(fs::path &path) {
    std::vector<SnapshotEntry> vSnapshot;
    fs::ifstream stream(path);
    csv::parser snapshot(stream);
    for (auto& row : snapshot) {
        SnapshotEntry utxo;
        bool key = false;
        for (auto& field : row) {
            if (!key) {
                key = true;
                utxo.script = ReadScriptSnapshot(field);
            } else {
                key = false;
                std::istringstream iss(field);
                iss >> utxo.amount;
                vSnapshot.push_back(utxo);
            }
        }
    }

    return vSnapshot;
}

std::vector<SnapshotEntry> EmptySnapshot() {
    std::vector<SnapshotEntry> vSnapshot;
    return vSnapshot;
}

std::vector<SnapshotEntry> InitSnapshot(const std::string fileName, std::vector<SnapshotProvider> providers) {
    fs::path snapshot_dir = gArgs.GetDataDirBase() / "snapshot";
    fs::path path = snapshot_dir / fileName;
    fs::create_directories(snapshot_dir);

    if (!fs::exists(path)) {
        // Pick random snapshot provider
        std::random_device random_device;
        std::mt19937 engine {random_device()};
        std::uniform_int_distribution<int> dist(0, providers.size() - 1);
        int provider_index = dist(engine);
        std::cout << TimestampStr() << "Shapshot: File " << fileName << " not found, trying to fetch it from " << providers[provider_index].address << providers[provider_index].path << std::endl;
        bool loaded = FetchSnapshot(path, providers[provider_index]);
        assert(loaded);
    }

    return LoadSnapshot(path);
}
