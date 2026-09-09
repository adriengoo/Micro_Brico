#include "test_database_config.h"

#include "datamodel.h"
#include "mainwindow.h"

#include <QApplication>
#include <QAction>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QStatusBar>
#include <QtTest>

#include <cstdlib>
#include <cstdio>

namespace {
constexpr auto kFixtureConnectionName = "camp001_tc001_fixture_db4qtest";
constexpr auto kApplicationConnectionName = "microbrico_test_db4qtest";
constexpr auto kAdminUtinfo = "TAD001";

bool execute(QSqlQuery &query, const QString &context)
{
    if (query.exec()) {
        return true;
    }
    qCritical().noquote() << context << ':' << query.lastError().text();
    return false;
}

void trace(const char *message)
{
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

Login_connect *visibleLoginDialog()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *dialog = qobject_cast<Login_connect *>(widget); dialog && dialog->isVisible()) {
            return dialog;
        }
    }
    return nullptr;
}
}

class Tc001LoginSession : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void loginAndLogout();
    void cleanupTestCase();

private:
    TestDatabaseConfig config;
    QSqlDatabase fixtureDatabase;
    QString adminPassword;

    bool resetFixture();
    bool insertFixtureAdmin();
    QString fixtureToken() const;
    int activeDomainRows() const;
};

void Tc001LoginSession::initTestCase()
{
    trace("TC-001: validating db4qtest configuration");
    config = TestDatabaseConfig::fromEnvironment();
    if (!config.requested) {
        QSKIP("TC-001 requires MB_TEST_DB_ENABLED=1 and an explicitly selected db4qtest schema.");
    }
    QVERIFY2(config.valid, qPrintable(config.error));
    QCOMPARE(config.databaseName, QStringLiteral("db4qtest"));
    adminPassword = qEnvironmentVariable("MB_TEST_ADMIN_PASSWORD");
    QVERIFY2(!adminPassword.isEmpty(), "MB_TEST_ADMIN_PASSWORD is required for the TC-001 fixture.");

    fixtureDatabase = QSqlDatabase::addDatabase(config.driver, QString::fromLatin1(kFixtureConnectionName));
    fixtureDatabase.setHostName(config.host);
    fixtureDatabase.setPort(config.port);
    fixtureDatabase.setDatabaseName(config.databaseName);
    fixtureDatabase.setUserName(config.user);
    fixtureDatabase.setPassword(config.password);
    QVERIFY2(fixtureDatabase.open(), qPrintable(fixtureDatabase.lastError().text()));
    trace("TC-001: db4qtest fixture connection opened");
    QVERIFY(resetFixture());
    QVERIFY(insertFixtureAdmin());
    trace("TC-001: fixture admin loaded");
}

bool Tc001LoginSession::resetFixture()
{
    QSqlQuery query(fixtureDatabase);
    query.prepare(QStringLiteral("DELETE l FROM log l INNER JOIN utilisateur u ON l.id_user = u.id WHERE u.utinfo = ?"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    if (!execute(query, QStringLiteral("Delete TC-001 fixture logs"))) return false;

    query.prepare(QStringLiteral("DELETE r FROM resa r INNER JOIN utilisateur u ON r.id_user = u.id WHERE u.utinfo = ?"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    if (!execute(query, QStringLiteral("Delete TC-001 fixture reservations"))) return false;

    query.prepare(QStringLiteral("DELETE s FROM sortie s INNER JOIN utilisateur u ON s.id_user = u.id WHERE u.utinfo = ?"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    if (!execute(query, QStringLiteral("Delete TC-001 fixture checkouts"))) return false;

    query.prepare(QStringLiteral("DELETE FROM utilisateur WHERE utinfo = ?"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    return execute(query, QStringLiteral("Delete TC-001 fixture user"));
}

bool Tc001LoginSession::insertFixtureAdmin()
{
    QSqlQuery query(fixtureDatabase);
    query.prepare(QStringLiteral(
        "INSERT INTO utilisateur "
        "(nom, mdp, prenom, email, token, utinfo, privilege, telephone, caution, adhesion_payed, date_caution) "
        "VALUES (?, SHA1(?), ?, ?, NULL, ?, 'admin', ?, ?, 1, ?)") );
    query.addBindValue(QStringLiteral("Admin"));
    query.addBindValue(adminPassword);
    query.addBindValue(QStringLiteral("Test"));
    query.addBindValue(QStringLiteral("test.admin.camp001@example.invalid"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    query.addBindValue(QStringLiteral("0000000000"));
    query.addBindValue(QStringLiteral("0.00"));
    query.addBindValue(QStringLiteral("2026-01-01"));
    return execute(query, QStringLiteral("Insert TC-001 fixture admin"));
}

QString Tc001LoginSession::fixtureToken() const
{
    QSqlQuery query(fixtureDatabase);
    query.prepare(QStringLiteral("SELECT token FROM utilisateur WHERE utinfo = ?"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    if (!execute(query, QStringLiteral("Read TC-001 fixture token")) || !query.next()) {
        return {};
    }
    return query.value(0).toString();
}

int Tc001LoginSession::activeDomainRows() const
{
    QSqlQuery query(fixtureDatabase);
    query.prepare(QStringLiteral(
        "SELECT "
        "(SELECT COUNT(*) FROM resa WHERE id_user = u.id AND is_active = 1) + "
        "(SELECT COUNT(*) FROM sortie WHERE id_user = u.id AND is_active = 1) "
        "FROM utilisateur u WHERE u.utinfo = ?"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    if (!execute(query, QStringLiteral("Read TC-001 fixture domain rows")) || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

void Tc001LoginSession::loginAndLogout()
{
    trace("TC-001: starting IHM login/logout path");
    QVERIFY(fixtureToken().isEmpty());

    trace("TC-001: constructing MainWindow");
    // MainWindow currently closes the global LogBrowser after deleting its UI.
    // That pre-existing shutdown path emits a queued log event and crashes.
    // Keep this one-window process alive until Qt Test exits; shutdown is not
    // part of TC-001 and is tracked as a remaining product limitation.
    auto *window = new MainWindow;
    trace("TC-001: MainWindow constructed");
    QCOMPARE(g_connect_db.db.connectionName(), QString::fromLatin1(kApplicationConnectionName));
    QVERIFY(g_connect_db.db.isOpen());
    window->show();

    QAction *loginAction = window->findChild<QAction *>(QStringLiteral("actionSe_connecter"));
    QAction *logoutAction = window->findChild<QAction *>(QStringLiteral("actionSe_d_connecter"));
    QVERIFY(loginAction);
    QVERIFY(logoutAction);
    QVERIFY(loginAction->isEnabled());
    QVERIFY(!logoutAction->isEnabled());

    loginAction->trigger();
    trace("TC-001: login action triggered");
    QTRY_VERIFY(visibleLoginDialog() != nullptr);
    Login_connect *dialog = visibleLoginDialog();
    QVERIFY(dialog);
    QLineEdit *utinfo = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_utinfo"));
    QLineEdit *password = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_mdp"));
    QPushButton *confirm = dialog->findChild<QPushButton *>(QStringLiteral("pushButton_ok"));
    QVERIFY(utinfo);
    QVERIFY(password);
    QVERIFY(confirm);

    QTest::keyClicks(utinfo, QString::fromLatin1(kAdminUtinfo));
    QTest::keyClicks(password, adminPassword);
    QTest::mouseClick(confirm, Qt::LeftButton);
    trace("TC-001: login confirmation clicked");

    QTRY_VERIFY(!loginAction->isEnabled());
    QTRY_VERIFY(logoutAction->isEnabled());
    QVERIFY(!fixtureToken().isEmpty());
    QCOMPARE(activeDomainRows(), 0);

    logoutAction->trigger();
    trace("TC-001: logout action triggered");
    QTRY_VERIFY(loginAction->isEnabled());
    QTRY_VERIFY(!logoutAction->isEnabled());
    QCOMPARE(activeDomainRows(), 0);
    trace("TC-001: login/logout path passed");
}

void Tc001LoginSession::cleanupTestCase()
{
    trace("TC-001: cleaning db4qtest fixture");
    if (fixtureDatabase.isOpen()) {
        QVERIFY(resetFixture());
        QSqlQuery query(fixtureDatabase);
        query.prepare(QStringLiteral("SELECT COUNT(*) FROM utilisateur WHERE utinfo = ?"));
        query.addBindValue(QString::fromLatin1(kAdminUtinfo));
        QVERIFY(execute(query, QStringLiteral("Verify TC-001 fixture cleanup")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        fixtureDatabase.close();
    }
}

int main(int argc, char *argv[])
{
    const TestDatabaseConfig config = TestDatabaseConfig::fromEnvironment();
    if (!config.requested) {
        std::fputs("SKIPPED: TC-001 requires MB_TEST_DB_ENABLED=1 and db4qtest. "
                   "No application or database connection was started.\n", stderr);
        return 77;
    }

    QApplication application(argc, argv);
    Tc001LoginSession test;
    const int result = QTest::qExec(&test, argc, argv);
    std::fflush(nullptr);
    // See the shutdown limitation documented above. The fixture cleanup ran in
    // cleanupTestCase before this process-level exit.
    std::_Exit(result);
}

#include "tc001_login_session.moc"
