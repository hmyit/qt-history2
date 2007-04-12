/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>


#include <qvalidator.h>

class tst_QDoubleValidator : public QObject
{
    Q_OBJECT
private slots:
    void validate_data();
    void validate();
    void validateThouSep_data();
    void validateThouSep();
    void validateIntEquiv_data();
    void validateIntEquiv();
};

Q_DECLARE_METATYPE(QValidator::State);
#define INV QValidator::Invalid
#define ITM QValidator::Intermediate
#define ACC QValidator::Acceptable

void tst_QDoubleValidator::validateThouSep_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QString>("value");
    QTest::addColumn<QValidator::State>("result");

    QTest::newRow("1,000C") << "C" << QString("1,000") << INV;
    QTest::newRow("1.000C") << "C" << QString("1.000") << ACC;

    QTest::newRow("1,000de") << "de" << QString("1,000") << ACC;
    QTest::newRow("1.000de") << "de" << QString("1.000") << ACC;

    QTest::newRow(".C") << "C" << QString(".") << ITM;
    QTest::newRow(".de") << "de" << QString(".") << ITM;
    QTest::newRow(",C") << "C" << QString(",") << INV;
    QTest::newRow(",de") << "de" << QString(",") << ITM;
}

void tst_QDoubleValidator::validateThouSep()
{
    QFETCH(QString, localeName);
    QFETCH(QString, value);
    QFETCH(QValidator::State, result);
    int dummy = 0;

    QDoubleValidator iv(-10000, 10000, 3, 0);
    iv.setNotation(QDoubleValidator::ScientificNotation);
    iv.setLocale(QLocale(localeName));

    QCOMPARE(iv.validate(value, dummy), result);
}

void tst_QDoubleValidator::validate_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("value");
    QTest::addColumn<QValidator::State>("scientific_state");
    QTest::addColumn<QValidator::State>("standard_state");

    QTest::newRow("data0")  << "C" << 0.0 << 100.0 << 1 << QString("50.0") << ACC << ACC;
    QTest::newRow("data1")  << "C" << 00.0 << 100.0 << 1 << QString("500.0") << ITM << ITM;
    QTest::newRow("data1a")  << "C" << 00.0 << 100.0 << 1 << QString("5001.0") << ITM << INV;
    QTest::newRow("data2")  << "C" << 00.0 << 100.0 << 1 << QString("-35.0") << INV << INV;
    QTest::newRow("data3")  << "C" << 00.0 << 100.0 << 1 << QString("a") << INV << INV;
    QTest::newRow("data4")  << "C" << 0.0 << 100.0 << 1 << QString("-") << INV << INV;
    QTest::newRow("data5")  << "C" << 0.0 << 100.0 << 1 << QString("100.0") << ACC << ACC;
    QTest::newRow("data6")  << "C" << -100.0 << 100.0 << 1 << QString("-") << ITM << ITM;
    QTest::newRow("data7")  << "C" << -100.0 << 100.0 << 1 << QString("-500.0") << ITM << ITM;
    QTest::newRow("data8")  << "C" << -100.0 << 100.0 << 1 << QString("-100") << ACC << ACC;
    QTest::newRow("data9")  << "C" << -100.0 << -10.0 << 1 << QString("10") << ITM << ITM;
    QTest::newRow("data10") << "C" << 0.3 << 0.5 << 5 << QString("0.34567") << ACC << ACC;
    QTest::newRow("data11") << "C" << -0.3 << -0.5 << 5 << QString("-0.345678") << INV << INV;
    QTest::newRow("data12") << "C" << -0.32 << 0.32 << 1 << QString("0") << ACC << ACC;
    QTest::newRow("data13") << "C" << 0.0 << 100.0 << 1 << QString("3456a") << INV << INV;
    QTest::newRow("data14") << "C" << -100.0 << 100.0 << 1 << QString("-3456a") << INV << INV;
    QTest::newRow("data15") << "C" << -100.0 << 100.0 << 1 << QString("a-3456") << INV << INV;
    QTest::newRow("data16") << "C" << -100.0 << 100.0 << 1 << QString("a-3456a") << INV << INV;
    QTest::newRow("data17") << "C" << 1229.0 << 1231.0 << 0 << QString("123e") << ITM << INV;
    QTest::newRow("data18") << "C" << 1229.0 << 1231.0 << 0 << QString("123e+") << ITM << INV;
    QTest::newRow("data19") << "C" << 1229.0 << 1231.0 << 0 << QString("123e+1") << ACC << INV;
    QTest::newRow("data20") << "C" << 12290.0 << 12310.0 << 0 << QString("123e+2") << ACC << INV;
    QTest::newRow("data21") << "C" << 12.290 << 12.310 << 2 << QString("123e-") << ITM << INV;
    QTest::newRow("data22") << "C" << 12.290 << 12.310 << 2 << QString("123e-1") << ACC << INV;
    QTest::newRow("data23") << "C" << 1.2290 << 1.2310 << 3 << QString("123e-2") << ACC << INV;
    QTest::newRow("data24") << "C" << 1229.0 << 1231.0 << 0 << QString("123E") << ITM << INV;
    QTest::newRow("data25") << "C" << 1229.0 << 1231.0 << 0 << QString("123E+") << ITM << INV;
    QTest::newRow("data26") << "C" << 1229.0 << 1231.0 << 0 << QString("123E+1") << ACC << INV;
    QTest::newRow("data27") << "C" << 12290.0 << 12310.0 << 0 << QString("123E+2") << ACC << INV;
    QTest::newRow("data28") << "C" << 12.290 << 12.310 << 2 << QString("123E-") << ITM << INV;
    QTest::newRow("data29") << "C" << 12.290 << 12.310 << 2 << QString("123E-1") << ACC << INV;
    QTest::newRow("data30") << "C" << 1.2290 << 1.2310 << 3 << QString("123E-2") << ACC << INV;
    QTest::newRow("data31") << "C" << 1.2290 << 1.2310 << 3 << QString("e") << ITM << INV;
    QTest::newRow("data32") << "C" << 1.2290 << 1.2310 << 3 << QString("e+") << ITM << INV;
    QTest::newRow("data33") << "C" << 1.2290 << 1.2310 << 3 << QString("e+1") << ITM << INV;
    QTest::newRow("data34") << "C" << 1.2290 << 1.2310 << 3 << QString("e-") << ITM << INV;
    QTest::newRow("data35") << "C" << 1.2290 << 1.2310 << 3 << QString("e-1") << ITM << INV;
    QTest::newRow("data36") << "C" << 1.2290 << 1.2310 << 3 << QString("E") << ITM << INV;
    QTest::newRow("data37") << "C" << 1.2290 << 1.2310 << 3 << QString("E+") << ITM << INV;
    QTest::newRow("data38") << "C" << 1.2290 << 1.2310 << 3 << QString("E+1") << ITM << INV;
    QTest::newRow("data39") << "C" << 1.2290 << 1.2310 << 3 << QString("E-") << ITM << INV;
    QTest::newRow("data40") << "C" << 1.2290 << 1.2310 << 3 << QString("E-1") << ITM << INV;
    QTest::newRow("data41") << "C" << -100.0 << 100.0 << 0 << QString("10e") << ITM << INV;
    QTest::newRow("data42") << "C" << -100.0 << 100.0 << 0 << QString("10e+") << ITM << INV;
    QTest::newRow("data43") << "C" << 0.01 << 0.09 << 2 << QString("0") << ITM << ITM;
    QTest::newRow("data44") << "C" << 0.0 << 10.0 << 1 << QString("11") << ITM << ITM;
    QTest::newRow("data45") << "C" << 0.0 << 10.0 << 2 << QString("11") << ITM << ITM;
    QTest::newRow("data46")  << "C" << 0.0 << 100.0 << 1 << QString("0.") << ACC << ACC;
    QTest::newRow("data47")  << "C" << 0.0 << 100.0 << 1 << QString(".") << ITM << ITM;

    QTest::newRow("data_de0")  << "de" << 0.0 << 100.0 << 1 << QString("50,0") << ACC << ACC;
    QTest::newRow("data_de1")  << "de" << 00.0 << 100.0 << 1 << QString("500,0") << ITM << ITM;
    QTest::newRow("data_de1a")  << "de" << 00.0 << 100.0 << 1 << QString("5001,0") << ITM << INV;
    QTest::newRow("data_de0C")  << "de" << 0.0 << 100.0 << 1 << QString("50.0") << ACC << ACC;
    QTest::newRow("data_de1C")  << "de" << 00.0 << 100.0 << 1 << QString("500.0") << ITM << ITM;
    QTest::newRow("data_de1aC")  << "de" << 00.0 << 100.0 << 1 << QString("5001.0") << ITM << INV;
    QTest::newRow("data_de2")  << "de" << 00.0 << 100.0 << 1 << QString("-35,0") << INV << INV;
    QTest::newRow("data_de3")  << "de" << 00.0 << 100.0 << 1 << QString("a") << INV << INV;
    QTest::newRow("data_de4")  << "de" << 0.0 << 100.0 << 1 << QString("-") << INV << INV;
    QTest::newRow("data_de5")  << "de" << 0.0 << 100.0 << 1 << QString("100,0") << ACC << ACC;
    QTest::newRow("data_de6")  << "de" << -100.0 << 100.0 << 1 << QString("-") << ITM << ITM;
    QTest::newRow("data_de7")  << "de" << -100.0 << 100.0 << 1 << QString("-500,0") << ITM << ITM;
    QTest::newRow("data_de8")  << "de" << -100.0 << 100.0 << 1 << QString("-100") << ACC << ACC;
    QTest::newRow("data_de9")  << "de" << -100.0 << -10.0 << 1 << QString("10") << ITM << ITM;
    QTest::newRow("data_de10") << "de" << 0.3 << 0.5 << 5 << QString("0,34567") << ACC << ACC;
    QTest::newRow("data_de11") << "de" << -0.3 << -0.5 << 5 << QString("-0,345678") << INV << INV;
    QTest::newRow("data_de12") << "de" << -0.32 << 0.32 << 1 << QString("0") << ACC << ACC;
    QTest::newRow("data_de13") << "de" << 0.0 << 100.0 << 1 << QString("3456a") << INV << INV;
    QTest::newRow("data_de14") << "de" << -100.0 << 100.0 << 1 << QString("-3456a") << INV << INV;
    QTest::newRow("data_de15") << "de" << -100.0 << 100.0 << 1 << QString("a-3456") << INV << INV;
    QTest::newRow("data_de16") << "de" << -100.0 << 100.0 << 1 << QString("a-3456a") << INV << INV;
    QTest::newRow("data_de17") << "de" << 1229.0 << 1231.0 << 0 << QString("123e") << ITM << INV;
    QTest::newRow("data_de18") << "de" << 1229.0 << 1231.0 << 0 << QString("123e+") << ITM << INV;
    QTest::newRow("data_de19") << "de" << 1229.0 << 1231.0 << 0 << QString("123e+1") << ACC << INV;
    QTest::newRow("data_de20") << "de" << 12290.0 << 12310.0 << 0 << QString("123e+2") << ACC << INV;
    QTest::newRow("data_de21") << "de" << 12.290 << 12.310 << 2 << QString("123e-") << ITM << INV;
    QTest::newRow("data_de22") << "de" << 12.290 << 12.310 << 2 << QString("123e-1") << ACC << INV;
    QTest::newRow("data_de23") << "de" << 1.2290 << 1.2310 << 3 << QString("123e-2") << ACC << INV;
    QTest::newRow("data_de24") << "de" << 1229.0 << 1231.0 << 0 << QString("123E") << ITM << INV;
    QTest::newRow("data_de25") << "de" << 1229.0 << 1231.0 << 0 << QString("123E+") << ITM << INV;
    QTest::newRow("data_de26") << "de" << 1229.0 << 1231.0 << 0 << QString("123E+1") << ACC << INV;
    QTest::newRow("data_de27") << "de" << 12290.0 << 12310.0 << 0 << QString("123E+2") << ACC << INV;
    QTest::newRow("data_de28") << "de" << 12.290 << 12.310 << 2 << QString("123E-") << ITM << INV;
    QTest::newRow("data_de29") << "de" << 12.290 << 12.310 << 2 << QString("123E-1") << ACC << INV;
    QTest::newRow("data_de30") << "de" << 1.2290 << 1.2310 << 3 << QString("123E-2") << ACC << INV;
    QTest::newRow("data_de31") << "de" << 1.2290 << 1.2310 << 3 << QString("e") << ITM << INV;
    QTest::newRow("data_de32") << "de" << 1.2290 << 1.2310 << 3 << QString("e+") << ITM << INV;
    QTest::newRow("data_de33") << "de" << 1.2290 << 1.2310 << 3 << QString("e+1") << ITM << INV;
    QTest::newRow("data_de34") << "de" << 1.2290 << 1.2310 << 3 << QString("e-") << ITM << INV;
    QTest::newRow("data_de35") << "de" << 1.2290 << 1.2310 << 3 << QString("e-1") << ITM << INV;
    QTest::newRow("data_de36") << "de" << 1.2290 << 1.2310 << 3 << QString("E") << ITM << INV;
    QTest::newRow("data_de37") << "de" << 1.2290 << 1.2310 << 3 << QString("E+") << ITM << INV;
    QTest::newRow("data_de38") << "de" << 1.2290 << 1.2310 << 3 << QString("E+1") << ITM << INV;
    QTest::newRow("data_de39") << "de" << 1.2290 << 1.2310 << 3 << QString("E-") << ITM << INV;
    QTest::newRow("data_de40") << "de" << 1.2290 << 1.2310 << 3 << QString("E-1") << ITM << INV;
    QTest::newRow("data_de41") << "de" << -100.0 << 100.0 << 0 << QString("10e") << ITM << INV;
    QTest::newRow("data_de42") << "de" << -100.0 << 100.0 << 0 << QString("10e+") << ITM << INV;
    QTest::newRow("data_de43") << "de" << 0.01 << 0.09 << 2 << QString("0") << ITM << ITM;
    QTest::newRow("data_de44") << "de" << 0.0 << 10.0 << 1 << QString("11") << ITM << ITM;
    QTest::newRow("data_de45") << "de" << 0.0 << 10.0 << 2 << QString("11") << ITM << ITM;

    QString arabicNum;
    arabicNum += QChar(1633); // "18.4" in arabic
    arabicNum += QChar(1640);
    arabicNum += QChar(1643);
    arabicNum += QChar(1636);
    QTest::newRow("arabic") << "ar" << 0.0 << 20.0 << 2 << arabicNum << ACC << ACC;
}

void tst_QDoubleValidator::validate()
{
    QFETCH(QString, localeName);
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(int, decimals);
    QFETCH(QString, value);
    QFETCH(QValidator::State, scientific_state);
    QFETCH(QValidator::State, standard_state);

    QLocale::setDefault(QLocale(localeName));

    QDoubleValidator dv(minimum, maximum, decimals, 0);
    int dummy;
    QCOMPARE((int)dv.validate(value, dummy), (int)scientific_state);
    dv.setNotation(QDoubleValidator::StandardNotation);
    QCOMPARE((int)dv.validate(value, dummy), (int)standard_state);
}

void tst_QDoubleValidator::validateIntEquiv_data()
{
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QValidator::State>("state");

    QTest::newRow("1.1") << 0.0 << 10.0 << QString("") << ITM;
    QTest::newRow("1.2") << 10.0 << 0.0 << QString("") << ITM;

    QTest::newRow("2.1") << 0.0 << 10.0 << QString("-") << INV;
    QTest::newRow("2.2") << 0.0 << 10.0 << QString("-0") << INV;
    QTest::newRow("2.3") << -10.0 << -1.0 << QString("+") << INV;
    QTest::newRow("2.4") << -10.0 << 10.0 << QString("-") << ITM;
    QTest::newRow("2.5") << -10.0 << 10.0 << QString("+") << ITM;
    QTest::newRow("2.5a") << -10.0 << -9.0 << QString("+") << INV;
    QTest::newRow("2.6") << -10.0 << 10.0 << QString("+0") << ACC;
    QTest::newRow("2.7") << -10.0 << 10.0 << QString("+1") << ACC;
    QTest::newRow("2.8") << -10.0 << 10.0 << QString("+-") << INV;
    QTest::newRow("2.9") << -10.0 << 10.0 << QString("-+") << INV;

    QTest::newRow("3.1") << 0.0 << 10.0 << QString("12345678901234567890") << INV;
    QTest::newRow("3.2") << 0.0 << 10.0 << QString("-12345678901234567890") << INV;
    QTest::newRow("3.3") << 0.0 << 10.0 << QString("000000000000000000000") << ACC;
    QTest::newRow("3.4") << 1.0 << 10.0 << QString("000000000000000000000") << ITM;
    QTest::newRow("3.5") << 0.0 << 10.0 << QString("-000000000000000000000") << INV;
    QTest::newRow("3.6") << -10.0 << -1.0 << QString("-000000000000000000000") << ITM;
    QTest::newRow("3.7") << -10.0 << -1.0 << QString("-0000000000000000000001") << ACC;

    QTest::newRow("4.1") << 0.0 << 10.0 << QString(" ") << INV;
    QTest::newRow("4.2") << 0.0 << 10.0 << QString(" 1") << INV;
    QTest::newRow("4.3") << 0.0 << 10.0 << QString("1 ") << INV;
    QTest::newRow("4.4") << 0.0 << 10.0 << QString("1.0") << INV;
    QTest::newRow("4.5") << 0.0 << 10.0 << QString("0.1") << INV;
    QTest::newRow("4.6") << 0.0 << 10.0 << QString(".1") << INV;
    QTest::newRow("4.7") << 0.0 << 10.0 << QString("-1.0") << INV;

    QTest::newRow("5.1") << 6.0 << 8.0 << QString("5") << ITM;
    QTest::newRow("5.1") << 6.0 << 8.0 << QString("56") << INV;
    QTest::newRow("5.2") << 6.0 << 8.0 << QString("7") << ACC;
    QTest::newRow("5.3") << 6.0 << 8.0 << QString("9") << ITM;
    QTest::newRow("5.3") << 6.0 << 8.0 << QString("-") << INV;
    QTest::newRow("5.4a") << -8.0 << -6.0 << QString("+") << INV;
    QTest::newRow("5.4b") << -8.0 << -6.0 << QString("+5") << INV;
    QTest::newRow("5.4c") << -8.0 << -6.0 << QString("-5") << ITM;
    QTest::newRow("5.5") << -8.0 << -6.0 << QString("-7") << ACC;
    QTest::newRow("5.6") << -8.0 << -6.0 << QString("-9") << ITM;
    QTest::newRow("5.7") << -8.0 << -6.0 << QString("5") << ITM;
    QTest::newRow("5.8") << -8.0 << -6.0 << QString("7") << ITM;
    QTest::newRow("5.9") << -8.0 << -6.0 << QString("9") << ITM;
    QTest::newRow("5.10") << -6.0 << 8.0 << QString("-5") << ACC;
    QTest::newRow("5.11") << -6.0 << 8.0 << QString("5") << ACC;
    QTest::newRow("5.12") << -6.0 << 8.0 << QString("-7") << ITM;
    QTest::newRow("5.13") << -6.0 << 8.0 << QString("7") << ACC;
    QTest::newRow("5.14") << -6.0 << 8.0 << QString("-9") << ITM;
    QTest::newRow("5.15") << -6.0 << 8.0 << QString("9") << ITM;

    QTest::newRow("6.1") << 100.0 << 102.0 << QString("11") << ITM;
    QTest::newRow("6.2") << 100.0 << 102.0 << QString("-11") << INV;

    QTest::newRow("7.1") << 0.0 << 10.0 << QString("100") << INV;
    QTest::newRow("7.2") << 0.0 << -10.0 << QString("100") << INV;
    QTest::newRow("7.3") << 0.0 << -10.0 << QString("-100") << INV;
    QTest::newRow("7.4") << -100.0 << 10.0 << QString("100") << ITM;
}

void tst_QDoubleValidator::validateIntEquiv()
{
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(QString, input);
    QFETCH(QValidator::State, state);

    QDoubleValidator dv(minimum, maximum, 0, 0);
    dv.setNotation(QDoubleValidator::StandardNotation);
    int dummy;
    QCOMPARE(dv.validate(input, dummy), state);
}

QTEST_APPLESS_MAIN(tst_QDoubleValidator)
#include "tst_qdoublevalidator.moc"
