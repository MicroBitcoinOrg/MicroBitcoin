# MicroBitcoin Technical Specs

New network is using [Bitcoin Core 22](https://github.com/bitcoin/bitcoin/tree/22.x) codebase and include following features:
- UTXO Snapshot from legacy network from block 525001 (first MBC block) to most recent block at the moment of new network start.
- New emission schedule
- Smaller block size
- YesPower Proof-of-Work algorightm
- Taproot activation

### Prerequisites

There couple reasons which prerequisites for launching new network:
- Launch of new network will create natural growth of userbase.
- Current total MBC supply is over-minted for the current userbase. To improve this situation coins which haven't been moved since hardfork will be burned and block emission schedule will be adjusted which would create more fair distributrion of supply.
- Inefficiently slow validation of new blocks caused by RFv2 PoW algorightm.
- Premine will be locked with [OP_CHECKLOCKTIMEVERIFY](https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki) output script.

### UTXOs snapshot

Snapshot will be done in following maner: all UTXO set starting after block 525000 (fist MBC block) to most recent block at the moment of new network start will be copied and merged. For example if you had 3 unspent outputs on your address in old network, they will be merged into one output with sum amount. All merged outputs will be set into genesis block of new network. To make them spendable genesis block will be added to database as an actual block.

Example:
```
Output #1
Script: OP_DUP OP_HASH160 84169602ccd51a35c2ba54bb209320dddce62660 OP_EQUALVERIFY OP_CHECKSIG
Amount: 3700000

Output #2
Script: OP_DUP OP_HASH160 84169602ccd51a35c2ba54bb209320dddce62660 OP_EQUALVERIFY OP_CHECKSIG
Amount: 120000

Output #3
Script: OP_DUP OP_HASH160 84169602ccd51a35c2ba54bb209320dddce62660 OP_EQUALVERIFY OP_CHECKSIG
Amount: 34052
```

Will be merged:

```
Merged output
Script: OP_DUP OP_HASH160 84169602ccd51a35c2ba54bb209320dddce62660 OP_EQUALVERIFY OP_CHECKSIG
Amount: 3854052
```

### Emission

Reward for each new block will be calculated using following function:

```c++
#include <iostream>
#include <cmath>

// Amounts of satoshit per coins
const int64_t COIN = 10000;

int64_t reward(int height) {
	// Initial reward per block
	const int64_t reward = 5500 * COIN;
	// Reward decreasing epoch (2 years)
	const int epoch = 525960 * 2;
	// Decrease amount by 30% each epoch
	const long double r = 1 + (std::log(1 - 0.3) / epoch);
	return reward * std::pow(r, height);
}
```

Graph for reward and total supply:

![Emission](https://i.imgur.com/emnp0s3.png)

### Block size

To make network more reliable, prevent block spamming and create better fee market block size will be decreased to 300kb.

[Reference implementation](https://github.com/bitcoin/bitcoin/compare/v0.17.1...luke-jr:example_300k-0.17).

### Power2b PoW

Rainforest v2 aka RFv2 is causing inefficiently slow validation of blocks thats why YesPower was picked as new PoW algo.

Yespower in particular is designed to be CPU-friendly, GPU-unfriendly, and FPGA/ASIC-neutral. In other words, it's meant to be relatively efficient to compute on current CPUs and relatively inefficient on current GPUs. Unfortunately, being GPU-unfriendly also means that eventual FPGA and ASIC implementations will only compete with CPUs, and at least ASICs will win over the CPUs (FPGAs might not because of this market's peculiarities - large FPGAs are even more "over-priced" than large CPUs are), albeit by far not to the extent they did e.g. for Bitcoin and Litecoin.

We are using [Power2b](https://github.com/volbil/yespower/) modification which is replaces sha256 with blake2b.

[Source](https://www.openwall.com/yespower/).
