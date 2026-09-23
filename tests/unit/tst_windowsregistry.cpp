// Copyright (C) 2023-present Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: Apache-2.0

#include <QtCore/QUuid>
#include <QtTest/QTest>
#include <QWKCore/qwindowkit_windows.h>

class WindowsRegistryTest : public QObject {
    Q_OBJECT
    QString subKey;
    HKEY key = nullptr;

    const wchar_t *keyName() const {
        return reinterpret_cast<const wchar_t *>(subKey.utf16());
    }

    static QByteArray bytes(DWORD value) {
        return QByteArray(reinterpret_cast<const char *>(&value), sizeof(value));
    }

private Q_SLOTS:
    void init() {
        // A unique leaf under HKCU, never a system personalization key.
        subKey = QStringLiteral("Software\\QWindowKitRegistryTest-\u6d4b\u8bd5-") +
                 QUuid::createUuid().toString(QUuid::WithoutBraces);
        QCOMPARE(::RegCreateKeyExW(HKEY_CURRENT_USER, keyName(), 0, nullptr,
                                  REG_OPTION_VOLATILE, KEY_READ | KEY_WRITE, nullptr,
                                  &key, nullptr), LSTATUS(ERROR_SUCCESS));
    }

    void cleanup() {
        if (key) {
            const auto status = ::RegCloseKey(key);
            key = nullptr;
            // Remove the leaf even if closing the test's own handle failed.
            const auto removed = ::RegDeleteKeyW(HKEY_CURRENT_USER, keyName());
            QCOMPARE(status, LSTATUS(ERROR_SUCCESS));
            QCOMPARE(removed, LSTATUS(ERROR_SUCCESS));
        }
    }

    void readValue_data() {
        QTest::addColumn<quint32>("type");
        QTest::addColumn<QByteArray>("data");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<quint32>("expected");
        QTest::newRow("zero") << quint32(REG_DWORD) << bytes(0) << true << quint32(0);
        QTest::newRow("nonzero") << quint32(REG_DWORD) << bytes(42) << true << quint32(42);
        QTest::newRow("high-bit") << quint32(REG_DWORD) << bytes(0x80000000u)
                                  << true << quint32(0x80000000u);
        QTest::newRow("all-bits") << quint32(REG_DWORD) << bytes(0xffffffffu)
                                  << true << quint32(0xffffffffu);
        QTest::newRow("string") << quint32(REG_SZ) << QByteArray("A\0\0\0", 4)
                                << false << quint32(0);
        QTest::newRow("binary") << quint32(REG_BINARY) << bytes(42) << false << quint32(0);
        QTest::newRow("qword") << quint32(REG_QWORD) << QByteArray(8, '\0')
                               << false << quint32(0);
        QTest::newRow("short-dword") << quint32(REG_DWORD) << QByteArray(2, '\1')
                                     << false << quint32(0);
        QTest::newRow("oversized-dword") << quint32(REG_DWORD) << QByteArray(8, '\1')
                                         << false << quint32(0);
        QTest::newRow("empty-dword") << quint32(REG_DWORD) << QByteArray()
                                     << false << quint32(0);
    }

    void readValue() {
        QFETCH(quint32, type);
        QFETCH(QByteArray, data);
        QFETCH(bool, valid);
        QFETCH(quint32, expected);
        static const wchar_t name[] = L"value-\u503c";
        QCOMPARE(::RegSetValueExW(key, name, 0, type,
                                 reinterpret_cast<const BYTE *>(data.constData()), DWORD(data.size())),
                 LSTATUS(ERROR_SUCCESS));
        const auto value = QWK::Private::readRegistryDword(HKEY_CURRENT_USER, keyName(), name);
        QCOMPARE(value.has_value(), valid);
        if (valid)
            QCOMPARE(quint32(*value), expected);
    }

    void missingKey() {
        const auto missing = subKey + QStringLiteral("\\missing");
        QVERIFY(!QWK::Private::readRegistryDword(
            HKEY_CURRENT_USER, reinterpret_cast<const wchar_t *>(missing.utf16()), L"value"));
    }

    void missingValue() {
        QVERIFY(!QWK::Private::readRegistryDword(HKEY_CURRENT_USER, keyName(), L"missing"));
    }

    void observesChanges() {
        for (DWORD expected : {DWORD(17), DWORD(0)}) {
            QCOMPARE(::RegSetValueExW(key, L"value", 0, REG_DWORD,
                                     reinterpret_cast<const BYTE *>(&expected), sizeof(expected)),
                     LSTATUS(ERROR_SUCCESS));
            const auto actual = QWK::Private::readRegistryDword(HKEY_CURRENT_USER, keyName(), L"value");
            QVERIFY(actual.has_value());
            QCOMPARE(*actual, expected);
        }
        QCOMPARE(::RegDeleteValueW(key, L"value"), LSTATUS(ERROR_SUCCESS));
        QVERIFY(!QWK::Private::readRegistryDword(HKEY_CURRENT_USER, keyName(), L"value"));
    }
};

QTEST_APPLESS_MAIN(WindowsRegistryTest)
#include "tst_windowsregistry.moc"
