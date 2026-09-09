#ifndef TEST_DATABASE_CONFIG_H
#define TEST_DATABASE_CONFIG_H

#include <QString>

// Configuration-only helper for future integration tests.  It never loads a
// SQL driver and never opens a database connection.
struct TestDatabaseConfig
{
    bool requested = false;
    bool valid = false;
    QString error;
    QString driver;
    QString host;
    quint16 port = 0;
    QString databaseName;
    QString user;
    QString password;

    static TestDatabaseConfig fromEnvironment();
};

#endif // TEST_DATABASE_CONFIG_H
