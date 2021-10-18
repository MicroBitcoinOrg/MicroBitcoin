// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MICRO_QT_MICROADDRESSVALIDATOR_H
#define MICRO_QT_MICROADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class MicroBitcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MicroBitcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** MicroBitcoin address widget validator, checks for a valid micro address.
 */
class MicroBitcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit MicroBitcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // MICRO_QT_MICROADDRESSVALIDATOR_H
