#include "test_database_config.h"

#include <QCoreApplication>
#include <QtTest>

#include <cstdio>

class Tc001LoginSessionFoundation : public QObject
{
    Q_OBJECT

private slots:
    void requiresExplicitDedicatedTestDatabase();
};

void Tc001LoginSessionFoundation::requiresExplicitDedicatedTestDatabase()
{
    const TestDatabaseConfig config = TestDatabaseConfig::fromEnvironment();

    if (!config.requested) {
        QSKIP("TC-001 is waiting for an explicitly selected test schema. The application is not launched and no database connection is attempted.");
    }

    QVERIFY2(config.valid, qPrintable(config.error));
    QCOMPARE(config.driver, QStringLiteral("QMYSQL"));
    QCOMPARE(config.databaseName, QStringLiteral("db4qtest"));
}

int main(int argc, char *argv[])
{
    const TestDatabaseConfig config = TestDatabaseConfig::fromEnvironment();
    if (!config.requested) {
        std::fputs("SKIPPED: TC-001 is waiting for an explicitly selected test schema. "
                   "The application is not launched and no database connection is attempted.\n",
                   stderr);
        return 77;
    }

    QCoreApplication application(argc, argv);
    Tc001LoginSessionFoundation test;
    return QTest::qExec(&test, argc, argv);
}

#include "tc001_login_session_foundation.moc"
