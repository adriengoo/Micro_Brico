#include "test_database_config.h"

#include <QByteArray>
#include <QStringList>

namespace {
QString environmentValue(const char *name)
{
    return qEnvironmentVariable(name).trimmed();
}

bool hasEnvironmentValue(const char *name)
{
    return qEnvironmentVariableIsSet(name) && !environmentValue(name).isEmpty();
}
}

TestDatabaseConfig TestDatabaseConfig::fromEnvironment()
{
    TestDatabaseConfig config;
    config.requested = environmentValue("MB_TEST_DB_ENABLED") == QStringLiteral("1");
    if (!config.requested) {
        config.error = QStringLiteral("MB_TEST_DB_ENABLED=1 is not set; no test database was selected.");
        return config;
    }

    const QStringList requiredValues = {
        QStringLiteral("MB_TEST_DB_DRIVER"),
        QStringLiteral("MB_TEST_DB_HOST"),
        QStringLiteral("MB_TEST_DB_PORT"),
        QStringLiteral("MB_TEST_DB_NAME"),
        QStringLiteral("MB_TEST_DB_USER")
    };
    for (const QString &name : requiredValues) {
        if (!hasEnvironmentValue(name.toUtf8().constData())) {
            config.error = QStringLiteral("Missing required test database variable: %1").arg(name);
            return config;
        }
    }
    if (!qEnvironmentVariableIsSet("MB_TEST_DB_PASSWORD")) {
        config.error = QStringLiteral("Missing required test database variable: MB_TEST_DB_PASSWORD");
        return config;
    }

    config.driver = environmentValue("MB_TEST_DB_DRIVER");
    config.host = environmentValue("MB_TEST_DB_HOST");
    config.databaseName = environmentValue("MB_TEST_DB_NAME");
    config.user = environmentValue("MB_TEST_DB_USER");
    config.password = qEnvironmentVariable("MB_TEST_DB_PASSWORD");

    bool portOk = false;
    const uint port = environmentValue("MB_TEST_DB_PORT").toUInt(&portOk);
    if (!portOk || port == 0 || port > 65535) {
        config.error = QStringLiteral("MB_TEST_DB_PORT must be an integer from 1 to 65535.");
        return config;
    }
    config.port = static_cast<quint16>(port);

    if (config.driver != QStringLiteral("QMYSQL")) {
        config.error = QStringLiteral("MB_TEST_DB_DRIVER must be QMYSQL for the Liquibase MySQL test schema.");
        return config;
    }
    if (config.databaseName.compare(QStringLiteral("db4qtest"), Qt::CaseInsensitive) != 0) {
        config.error = QStringLiteral("MB_TEST_DB_NAME must be db4qtest for CAMP-001; no other schema is permitted.");
        return config;
    }

    config.valid = true;
    return config;
}
