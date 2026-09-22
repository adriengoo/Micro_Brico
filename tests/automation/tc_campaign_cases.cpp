#include "test_database_config.h"

#include "datamodel.h"
#include "mainwindow.h"
#include "popups/login_connect.h"
#include "popups/poppupaddkit.h"
#include "popups/popupaddusers.h"
#include "popups/popupsortirresa.h"

#include <QAction>
#include <QApplication>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QtTest>

#include <cstdlib>
#include <cstdio>
#include <functional>

namespace {
constexpr auto kFixtureConnection = "camp001_cases_fixture_db4qtest";
constexpr auto kApplicationConnection = "microbrico_test_db4qtest";
constexpr auto kAdminUtinfo = "TEST_ADMIN";
constexpr auto kUser010 = "TEST_U_010";
constexpr auto kUser011 = "TEST_U_011";
constexpr auto kUser031 = "UINFO_031";
constexpr auto kKit021 = "TEST-K-021";
constexpr auto kKit020 = "TEST-K-020";
constexpr auto kKit003 = "TEST-K-003";
constexpr auto kKit031 = "TEST-K-031";
constexpr auto kKit031Witness = "TEST-K-31W";
constexpr auto kItem020A = "TEST_ITEM_A";
constexpr auto kItem020B = "TEST_ITEM_B";
constexpr auto kUser040 = "UINFO_040";
constexpr auto kUser042 = "UINFO_042";
constexpr auto kKit040 = "TEST-K-040";
constexpr auto kKit042 = "TEST-K-042";
constexpr auto kItem040 = "TEST_ITEM_040";
constexpr auto kItem042 = "TEST_ITEM_042";
constexpr auto kUserPassword = "camp001-synthetic-user-password";

bool run(QSqlQuery &query, const QString &context)
{
    if (query.exec()) return true;
    std::fprintf(stderr, "%s: %s\n", qPrintable(context), qPrintable(query.lastError().text()));
    return false;
}

bool runSql(QSqlQuery &query, const QString &sql, const QString &context)
{
    if (query.exec(sql)) return true;
    std::fprintf(stderr, "%s: %s\n", qPrintable(context), qPrintable(query.lastError().text()));
    return false;
}

bool waitFor(const std::function<bool()> &condition, int timeoutMs = 2000)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (condition()) return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QTest::qWait(20);
    }
    return condition();
}

QDate nextFriday()
{
    QDate date = QDate::currentDate();
    const int days = (5 - date.dayOfWeek() + 7) % 7;
    return date.addDays(days);
}

QDate plusSevenDays()
{
    return QDate::currentDate().addDays(7);
}

template <typename T>
T *visibleTopLevel()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<T *>(widget); candidate && candidate->isVisible()) return candidate;
    }
    return nullptr;
}

void scheduleMessageBoxButton(QMessageBox::StandardButton button)
{
    auto *timer = new QTimer(qApp);
    timer->setInterval(10);
    QObject::connect(timer, &QTimer::timeout, timer, [timer, button] {
        if (auto *box = visibleTopLevel<QMessageBox>()) {
            if (auto *target = box->button(button)) QTest::mouseClick(target, Qt::LeftButton);
            timer->stop();
            timer->deleteLater();
        }
    });
    timer->start();
}
}

class CampaignFixture
{
public:
    TestDatabaseConfig config;
    QSqlDatabase database;
    QString adminPassword;

    bool open();
    bool reset();
    bool insertUser(const QString &utinfo, const QString &name, const QString &firstName,
                    const QString &password, bool administrator = false);
    bool insertKit(const QString &code, const QString &name, bool broken, int *id = nullptr);
    bool insertItem(const QString &kitCode, const QString &name, int quantity);
    bool insertReservation(const QString &utinfo, const QString &kitCode, const QDate &date, int reservationNumber);
    int scalar(const QString &sql, const QVariantList &values = {}) const;
    QString value(const QString &sql, const QVariantList &values = {}) const;
};

bool CampaignFixture::open()
{
    config = TestDatabaseConfig::fromEnvironment();
    if (!config.requested || !config.valid) return false;
    adminPassword = qEnvironmentVariable("MB_TEST_ADMIN_PASSWORD");
    if (adminPassword.isEmpty()) return false;

    database = QSqlDatabase::addDatabase(config.driver, QString::fromLatin1(kFixtureConnection));
    database.setHostName(config.host);
    database.setPort(config.port);
    database.setDatabaseName(config.databaseName);
    database.setUserName(config.user);
    database.setPassword(config.password);
    return database.open();
}

bool CampaignFixture::reset()
{
    const QString userList = "'TEST_ADMIN','TEST_U_010','TEST_U_011','UINFO_030','UINFO_031','UINFO_040','UINFO_041','UINFO_042','UINFO_043'";
    const QString kitList = "'TEST-K-003','TEST-K-020','TEST-K-021','TEST-K-030','TEST-K-031','TEST-K-31W','TEST-K-040','TEST-K-041','TEST-K-042','TEST-K-043'";
    QSqlQuery query(database);

    if (!runSql(query, "DELETE l FROM log l LEFT JOIN utilisateur u ON l.id_user=u.id LEFT JOIN kit k ON l.id_kit=k.idkit "
                       "WHERE u.utinfo IN (" + userList + ") OR k.code_kit IN (" + kitList + ")", QStringLiteral("Reset fixture logs"))) return false;
    if (!runSql(query, "DELETE r FROM resa r LEFT JOIN utilisateur u ON r.id_user=u.id LEFT JOIN kit k ON r.id_kit=k.idkit "
                       "WHERE u.utinfo IN (" + userList + ") OR k.code_kit IN (" + kitList + ")", QStringLiteral("Reset fixture reservations"))) return false;
    if (!runSql(query, "DELETE s FROM sortie s LEFT JOIN utilisateur u ON s.id_user=u.id LEFT JOIN kit k ON s.id_kit=k.idkit "
                       "WHERE u.utinfo IN (" + userList + ") OR k.code_kit IN (" + kitList + ")", QStringLiteral("Reset fixture checkouts"))) return false;
    if (!runSql(query, "DELETE FROM item WHERE forkey IN (SELECT idkit FROM kit WHERE code_kit IN (" + kitList + "))", QStringLiteral("Reset fixture items"))) return false;
    if (!runSql(query, "DELETE FROM kit WHERE code_kit IN (" + kitList + ")", QStringLiteral("Reset fixture kits"))) return false;
    return runSql(query, "DELETE FROM utilisateur WHERE utinfo IN (" + userList + ")", QStringLiteral("Reset fixture users"));
}

bool CampaignFixture::insertUser(const QString &utinfo, const QString &name, const QString &firstName,
                                 const QString &password, bool administrator)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO utilisateur (nom, mdp, prenom, email, token, utinfo, privilege, telephone, caution, adhesion_payed, date_caution) "
        "VALUES (?, SHA1(?), ?, ?, NULL, ?, ?, '0612345678', '10.00', 1, '2026-01-01')"));
    query.addBindValue(name);
    query.addBindValue(password);
    query.addBindValue(firstName);
    query.addBindValue(utinfo.toLower() + QStringLiteral("@example.invalid"));
    query.addBindValue(utinfo);
    query.addBindValue(administrator ? QStringLiteral("admin") : QStringLiteral("user"));
    return run(query, QStringLiteral("Insert fixture user %1").arg(utinfo));
}

bool CampaignFixture::insertKit(const QString &code, const QString &name, bool broken, int *id)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO kit (nom, description, date_achat, prix_achat, texte_libre, en_panne, code_kit, caution, isgame) "
        "VALUES (?, 'CAMP-001 fixture', '2026-01-01', '1.00', '', ?, ?, '0.00', 0)"));
    query.addBindValue(name);
    query.addBindValue(broken ? 1 : 0);
    query.addBindValue(code);
    if (!run(query, QStringLiteral("Insert fixture kit %1").arg(code))) return false;
    if (id) *id = query.lastInsertId().toInt();
    return true;
}

bool CampaignFixture::insertItem(const QString &kitCode, const QString &name, int quantity)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO item (name, forkey, quantity, quantity_init, quantity_out) "
        "SELECT ?, idkit, ?, ?, 0 FROM kit WHERE code_kit=?"));
    query.addBindValue(name);
    query.addBindValue(quantity);
    query.addBindValue(quantity);
    query.addBindValue(kitCode);
    return run(query, QStringLiteral("Insert fixture item %1").arg(name));
}

bool CampaignFixture::insertReservation(const QString &utinfo, const QString &kitCode, const QDate &date, int reservationNumber)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO resa (start_date, id_user, id_kit, id_resa, is_active) "
        "SELECT ?, u.id, k.idkit, ?, 1 FROM utilisateur u CROSS JOIN kit k "
        "WHERE u.utinfo=? AND k.code_kit=?"));
    query.addBindValue(date);
    query.addBindValue(reservationNumber);
    query.addBindValue(utinfo);
    query.addBindValue(kitCode);
    return run(query, QStringLiteral("Insert fixture reservation %1").arg(reservationNumber));
}

int CampaignFixture::scalar(const QString &sql, const QVariantList &values) const
{
    QSqlQuery query(database);
    query.prepare(sql);
    for (const QVariant &value : values) query.addBindValue(value);
    if (!run(query, QStringLiteral("Fixture scalar query")) || !query.next()) return -1;
    return query.value(0).toInt();
}

QString CampaignFixture::value(const QString &sql, const QVariantList &values) const
{
    QSqlQuery query(database);
    query.prepare(sql);
    for (const QVariant &value : values) query.addBindValue(value);
    if (!run(query, QStringLiteral("Fixture value query")) || !query.next()) return {};
    return query.value(0).toString();
}

class CampaignCases : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void tc002_login_invalid();
    void tc003_session_invalidated_by_reservation_refresh();
    void tc010_create_and_find_user();
    void tc011_duplicate_utinfo_rejected();
    void tc020_create_kit_with_initial_items();
    void tc021_broken_kit_not_added_to_reservation_basket();
    void tc031_cancel_reservation_and_restore_availability();
    void tc040_output_requires_recipient_password();
    void tc042_output_with_admin_signature_keeps_recipient();
    void cleanup();
    void cleanupTestCase();

private:
    CampaignFixture fixture;
    MainWindow *newLoggedInWindow();
    bool login(MainWindow *window);
    bool fillUserDialog(popupAddUsers *dialog, const QString &utinfo, const QString &name,
                        const QString &firstName, const QString &email, const QString &password);
    bool selectOutputReservation(MainWindow *window, const QString &utinfo, const QString &kitCode);
    bool validateOutputPassword(MainWindow *window, const QString &password, bool administratorSignature);
    bool verifyPopupItemAndSubmit(int quantity);
};

void CampaignCases::initTestCase()
{
    if (TestDatabaseConfig::fromEnvironment().requested == false) {
        QSKIP("CAMP-001 cases require MB_TEST_DB_ENABLED=1 and db4qtest.");
    }
    QVERIFY2(fixture.open(), "Unable to open the db4qtest fixture connection.");
    QCOMPARE(fixture.config.databaseName, QStringLiteral("db4qtest"));
}

void CampaignCases::init()
{
    QVERIFY(fixture.reset());
    QVERIFY(fixture.insertUser(QString::fromLatin1(kAdminUtinfo), QStringLiteral("Admin"), QStringLiteral("Campaign"), fixture.adminPassword, true));
}

void CampaignCases::cleanup()
{
    QVERIFY(fixture.reset());
}

void CampaignCases::cleanupTestCase()
{
    if (fixture.database.isOpen()) fixture.database.close();
}

MainWindow *CampaignCases::newLoggedInWindow()
{
    // MainWindow currently has a pre-existing shutdown crash in LogBrowser.
    // The one-case-per-process CTest runner exits after fixture cleanup.
    auto *window = new MainWindow;
    if (g_connect_db.db.connectionName() != QString::fromLatin1(kApplicationConnection) || !g_connect_db.db.isOpen()) {
        std::fputs("The application database connection is not the required open db4qtest connection.\n", stderr);
        return nullptr;
    }
    window->show();
    if (!login(window)) return nullptr;
    return window;
}

bool CampaignCases::login(MainWindow *window)
{
    QAction *loginAction = window->findChild<QAction *>(QStringLiteral("actionSe_connecter"));
    QAction *logoutAction = window->findChild<QAction *>(QStringLiteral("actionSe_d_connecter"));
    if (!loginAction || !logoutAction) return false;
    loginAction->trigger();
    if (!waitFor([] { return visibleTopLevel<Login_connect>() != nullptr; })) return false;
    auto *dialog = visibleTopLevel<Login_connect>();
    auto *utinfo = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_utinfo"));
    auto *password = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_mdp"));
    auto *confirm = dialog->findChild<QPushButton *>(QStringLiteral("pushButton_ok"));
    if (!utinfo || !password || !confirm) return false;
    QTest::keyClicks(utinfo, QString::fromLatin1(kAdminUtinfo));
    QTest::keyClicks(password, fixture.adminPassword);
    QTest::mouseClick(confirm, Qt::LeftButton);
    if (!waitFor([logoutAction] { return logoutAction->isEnabled(); })) return false;
    return !loginAction->isEnabled();
}

bool CampaignCases::fillUserDialog(popupAddUsers *dialog, const QString &utinfo, const QString &name,
                                   const QString &firstName, const QString &email, const QString &password)
{
    const auto setText = [dialog](const QString &objectName, const QString &text) {
        auto *field = dialog->findChild<QLineEdit *>(objectName);
        if (!field) return false;
        field->setText(text);
        return true;
    };
    return setText(QStringLiteral("lineEdit_utinfo"), utinfo)
        && setText(QStringLiteral("lineEdit_Nom"), name)
        && setText(QStringLiteral("lineEdit_Prenom"), firstName)
        && setText(QStringLiteral("lineEditMail"), email)
        && setText(QStringLiteral("lineEdit_tel_mobile"), QStringLiteral("0612345678"))
        && setText(QStringLiteral("lineEdit_mdp"), password)
        && setText(QStringLiteral("lineEdit_confirme_mdp_2"), password)
        && setText(QStringLiteral("lineEdit_montant_cheque_caution"), QStringLiteral("10.00"));
}

bool CampaignCases::selectOutputReservation(MainWindow *window, const QString &utinfo, const QString &kitCode)
{
    auto *user = window->findChild<QLineEdit *>(QStringLiteral("SORTIE_lineEdit_utinfo"));
    auto *show = window->findChild<QPushButton *>(QStringLiteral("SORTIE_pushButton_resa_showResa"));
    auto *reservations = window->findChild<QListWidget *>(QStringLiteral("SORTIE_listWidget_resa_currentResa"));
    auto *kits = window->findChild<QListWidget *>(QStringLiteral("SORTIE_listWidget_resa_kitsOfResa"));
    if (!user || !show || !reservations || !kits) return false;

    user->setText(utinfo);
    QTest::mouseClick(show, Qt::LeftButton);
    if (!waitFor([reservations] { return reservations->count() == 1; })) return false;
    QTest::mouseClick(reservations->viewport(), Qt::LeftButton, Qt::NoModifier,
                      reservations->visualItemRect(reservations->item(0)).center());
    if (!waitFor([kits] { return kits->count() == 1; })) return false;
    if (!kits->item(0)->text().contains(kitCode)) return false;
    QTest::mouseClick(kits->viewport(), Qt::LeftButton, Qt::NoModifier,
                      kits->visualItemRect(kits->item(0)).center());
    return kits->currentItem() != nullptr;
}

bool CampaignCases::validateOutputPassword(MainWindow *window, const QString &password, bool administratorSignature)
{
    auto *adminSignature = window->findChild<QCheckBox *>(QStringLiteral("SORTIE_checkBox_resa_mdpadmin"));
    auto *passwordField = window->findChild<QLineEdit *>(QStringLiteral("SORTIE_lineEdit_resa_mdp"));
    auto *validate = window->findChild<QPushButton *>(QStringLiteral("SORTIE_pushButton_validate_mdp_sortie"));
    auto *checkout = window->findChild<QPushButton *>(QStringLiteral("SORTIE_pushButton_sortir"));
    if (!adminSignature || !passwordField || !validate || !checkout) return false;
    if (adminSignature->isChecked() != administratorSignature) QTest::mouseClick(adminSignature, Qt::LeftButton);
    passwordField->setText(password);
    scheduleMessageBoxButton(QMessageBox::Ok);
    QTest::mouseClick(validate, Qt::LeftButton);
    return waitFor([checkout] { return checkout->isEnabled(); });
}

bool CampaignCases::verifyPopupItemAndSubmit(int quantity)
{
    if (!waitFor([] { return visibleTopLevel<PopupSortirResa>() != nullptr; })) return false;
    auto *popup = visibleTopLevel<PopupSortirResa>();
    auto *source = popup->findChild<QListWidget *>(QStringLiteral("listWidget_popupSortie_ItemSource"));
    auto *destination = popup->findChild<QListWidget *>(QStringLiteral("listWidget_popupSortie_ItemDest"));
    auto *count = popup->findChild<QSpinBox *>(QStringLiteral("spinBox_popupSortie_countItemVerif"));
    auto *verify = popup->findChild<QPushButton *>(QStringLiteral("pushButton_popupSortie_pushDest"));
    auto *submit = popup->findChild<QPushButton *>(QStringLiteral("pushButton_sortir"));
    if (!source || !destination || !count || !verify || !submit || source->count() != 1) return false;
    QTest::mouseClick(source->viewport(), Qt::LeftButton, Qt::NoModifier,
                      source->visualItemRect(source->item(0)).center());
    count->setValue(quantity);
    QTest::mouseClick(verify, Qt::LeftButton);
    if (!waitFor([destination] { return destination->count() == 1; })) return false;
    scheduleMessageBoxButton(QMessageBox::Ok);
    QTest::mouseClick(submit, Qt::LeftButton);
    return waitFor([] { return visibleTopLevel<PopupSortirResa>() == nullptr; });
}

void CampaignCases::tc002_login_invalid()
{
    // TC-002 — Étape 1 : soumettre une authentification invalide.
    auto *window = new MainWindow;
    window->show();
    const QString initialToken = fixture.value(QStringLiteral("SELECT token FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kAdminUtinfo)});
    QVERIFY(initialToken.isEmpty());

    QAction *loginAction = window->findChild<QAction *>(QStringLiteral("actionSe_connecter"));
    QAction *logoutAction = window->findChild<QAction *>(QStringLiteral("actionSe_d_connecter"));
    QVERIFY(loginAction);
    QVERIFY(logoutAction);
    loginAction->trigger();
    QTRY_VERIFY(visibleTopLevel<Login_connect>() != nullptr);
    auto *dialog = visibleTopLevel<Login_connect>();
    auto *utinfo = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_utinfo"));
    auto *password = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_mdp"));
    auto *confirm = dialog->findChild<QPushButton *>(QStringLiteral("pushButton_ok"));
    QVERIFY(utinfo && password && confirm);
    utinfo->setText(QString::fromLatin1(kAdminUtinfo));
    password->setText(QStringLiteral("mauvais-mdp"));
    scheduleMessageBoxButton(QMessageBox::Ok);
    QTest::mouseClick(confirm, Qt::LeftButton);
    // TC-002 — Étape 2 : vérifier que les accès protégés restent inactifs.
    QTRY_VERIFY(loginAction->isEnabled());
    QVERIFY(!logoutAction->isEnabled());
    // TC-002 — Étape 3 : vérifier l'absence d'effet persistant sur le jeton.
    QCOMPARE(fixture.value(QStringLiteral("SELECT token FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kAdminUtinfo)}), initialToken);
}

void CampaignCases::tc003_session_invalidated_by_reservation_refresh()
{
    QVERIFY(fixture.insertKit(QString::fromLatin1(kKit003), QStringLiteral("Kit session 003"), false));
    auto *window = newLoggedInWindow();
    QVERIFY(window);
    auto *calendar = window->findChild<QCalendarWidget *>(QStringLiteral("RESA_calendarWidget"));
    auto *reserve = window->findChild<QPushButton *>(QStringLiteral("RESA_pushButton_reserver"));
    auto *user = window->findChild<QLineEdit *>(QStringLiteral("RESA_lineEdit_resa_utinfo_user"));
    auto *refreshReservations = window->findChild<QPushButton *>(QStringLiteral("RESA_pushButton_resa_showResa"));
    auto *findKit = window->findChild<QLineEdit *>(QStringLiteral("RESA_lineEdit_findkitbycode"));
    auto *findKitButton = window->findChild<QPushButton *>(QStringLiteral("RESA_pushButton_getkit_resa"));
    auto *availableKits = window->findChild<QListWidget *>(QStringLiteral("RESA_listWidget_resa"));
    auto *basket = window->findChild<QListWidget *>(QStringLiteral("RESA_listWidget_panierResa"));
    auto *loginAction = window->findChild<QAction *>(QStringLiteral("actionSe_connecter"));
    auto *logoutAction = window->findChild<QAction *>(QStringLiteral("actionSe_d_connecter"));
    auto *addKitAction = window->findChild<QAction *>(QStringLiteral("actionajouter_nouveau_kit"));
    QVERIFY(calendar && reserve && user && refreshReservations && findKit && findKitButton && availableKits && basket && loginAction && logoutAction && addKitAction);

    // TC-003 — Étape 1 : établir une session administrateur avec réservation disponible.
    const QDate date = nextFriday();
    calendar->setSelectedDate(date);
    QMetaObject::invokeMethod(window, "on_RESA_calendarWidget_clicked", Q_ARG(QDate, date));
    findKit->setText(QString::fromLatin1(kKit003));
    QTest::mouseClick(findKitButton, Qt::LeftButton);
    QTRY_COMPARE(availableKits->count(), 1);
    availableKits->setCurrentRow(0);
    QVERIFY(QMetaObject::invokeMethod(window, "on_RESA_listWidget_resa_itemDoubleClicked",
                                      Q_ARG(QListWidgetItem *, availableKits->item(0))));
    QTRY_COMPARE(basket->count(), 1);
    QTRY_VERIFY(reserve->isEnabled());
    QVERIFY(logoutAction->isEnabled());
    QVERIFY(!loginAction->isEnabled());

    // TC-003 — Étape 2 : invalider uniquement le jeton serveur de TEST_ADMIN.
    QSqlQuery query(fixture.database);
    query.prepare(QStringLiteral("UPDATE utilisateur SET token=? WHERE utinfo=?"));
    query.addBindValue(QStringLiteral("TC003_INVALIDATED_TOKEN"));
    query.addBindValue(QString::fromLatin1(kAdminUtinfo));
    QVERIFY(run(query, QStringLiteral("Invalidate TC-003 fixture token")));
    QCOMPARE(fixture.value(QStringLiteral("SELECT token FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kAdminUtinfo)}),
             QStringLiteral("TC003_INVALIDATED_TOKEN"));

    // TC-003 — Étape 3 : actualiser les réservations, appelant get_user_by_utinfo puis is_user_connected.
    user->setText(QString::fromLatin1(kAdminUtinfo));
    QTest::mouseClick(refreshReservations, Qt::LeftButton);
    QTRY_VERIFY(loginAction->isEnabled());
    QTRY_VERIFY(!logoutAction->isEnabled());
    QVERIFY(!reserve->isEnabled());
    QVERIFY(!addKitAction->isEnabled());
    QVERIFY(window->isVisible());
    QCOMPARE(fixture.value(QStringLiteral("SELECT token FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kAdminUtinfo)}),
             QStringLiteral("TC003_INVALIDATED_TOKEN"));
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM resa r JOIN utilisateur u ON r.id_user=u.id WHERE u.utinfo=? AND r.is_active=1"),
                             {QString::fromLatin1(kAdminUtinfo)}), 0);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM sortie s JOIN utilisateur u ON s.id_user=u.id WHERE u.utinfo=? AND s.is_active=1"),
                             {QString::fromLatin1(kAdminUtinfo)}), 0);
}

void CampaignCases::tc010_create_and_find_user()
{
    auto *window = newLoggedInWindow();
    QVERIFY(window);
    // TC-010 — Étape 1 : créer l'utilisateur complet de fixture.
    auto *add = window->findChild<QAction *>(QStringLiteral("actionAjouter_utilisateur"));
    QVERIFY(add);
    add->trigger();
    QTRY_VERIFY(visibleTopLevel<popupAddUsers>() != nullptr);
    auto *dialog = visibleTopLevel<popupAddUsers>();
    QVERIFY(fillUserDialog(dialog, QString::fromLatin1(kUser010), QStringLiteral("Martin"), QStringLiteral("Test"),
                            QStringLiteral("test-u-010@example.invalid"), fixture.adminPassword));
    dialog->findChild<QCheckBox *>(QStringLiteral("checkBox_adhesion"))->setChecked(true);
    scheduleMessageBoxButton(QMessageBox::Ok);
    QTest::mouseClick(dialog->findChild<QPushButton *>(QStringLiteral("pushButton_OK")), Qt::LeftButton);
    QTRY_COMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kUser010)}), 1);

    // TC-010 — Étape 2 : rechercher par utinfo puis par nom.
    auto *byUtinfo = window->findChild<QLineEdit *>(QStringLiteral("GESUSER_lineEdit_finduserbyutinfo"));
    auto *byName = window->findChild<QLineEdit *>(QStringLiteral("GESUSER_lineEdit_finduserbyname"));
    auto *search = window->findChild<QPushButton *>(QStringLiteral("GESUSER_pushButton_getuser"));
    auto *table = window->findChild<QTableWidget *>(QStringLiteral("GESUSER_tableWidget_user"));
    QVERIFY(byUtinfo && byName && search && table);
    byUtinfo->setText(QString::fromLatin1(kUser010));
    QTest::mouseClick(search, Qt::LeftButton);
    QTRY_COMPARE(table->rowCount(), 1);
    byUtinfo->clear();
    byName->setText(QStringLiteral("Martin"));
    QTest::mouseClick(search, Qt::LeftButton);
    QTRY_COMPARE(table->rowCount(), 1);
    // TC-010 — Étape 3 : contrôler les données persistées de l'utilisateur.
    QCOMPARE(fixture.value(QStringLiteral("SELECT email FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kUser010)}),
             QStringLiteral("test-u-010@example.invalid"));
}

void CampaignCases::tc011_duplicate_utinfo_rejected()
{
    QVERIFY(fixture.insertUser(QString::fromLatin1(kUser011), QStringLiteral("Original"), QStringLiteral("User"), fixture.adminPassword));
    auto *window = newLoggedInWindow();
    QVERIFY(window);
    // TC-011 — Étape 1 : soumettre une seconde création avec le même utinfo.
    window->findChild<QAction *>(QStringLiteral("actionAjouter_utilisateur"))->trigger();
    QTRY_VERIFY(visibleTopLevel<popupAddUsers>() != nullptr);
    auto *dialog = visibleTopLevel<popupAddUsers>();
    QVERIFY(fillUserDialog(dialog, QString::fromLatin1(kUser011), QStringLiteral("Duplicate"), QStringLiteral("Attempt"),
                            QStringLiteral("duplicate-attempt@example.invalid"), fixture.adminPassword));
    scheduleMessageBoxButton(QMessageBox::Ok);
    QTest::mouseClick(dialog->findChild<QPushButton *>(QStringLiteral("pushButton_OK")), Qt::LeftButton);
    // TC-011 — Étape 2 : vérifier unicité et absence de persistance partielle.
    QTRY_COMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kUser011)}), 1);
    QCOMPARE(fixture.value(QStringLiteral("SELECT nom FROM utilisateur WHERE utinfo=?"), {QString::fromLatin1(kUser011)}), QStringLiteral("Original"));
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM utilisateur WHERE email=?"), {QStringLiteral("duplicate-attempt@example.invalid")}), 0);
    // TC-011 — Étape 3 : vérifier l'absence de log annexe.
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM log l JOIN utilisateur u ON l.id_user=u.id WHERE u.utinfo=?"), {QString::fromLatin1(kUser011)}), 0);
}

void CampaignCases::tc020_create_kit_with_initial_items()
{
    auto *window = newLoggedInWindow();
    QVERIFY(window);

    // TC-020 — Étape 1 : renseigner le kit et ajouter les deux items au formulaire.
    auto *addKit = window->findChild<QAction *>(QStringLiteral("actionajouter_nouveau_kit"));
    QVERIFY(addKit);
    addKit->trigger();
    QTRY_VERIFY(visibleTopLevel<PoppupAddKit>() != nullptr);
    auto *dialog = visibleTopLevel<PoppupAddKit>();
    auto *name = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_kit_name"));
    auto *code = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_kit_code"));
    auto *purchaseDate = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_kit_dateachat"));
    auto *purchasePrice = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_kitprix"));
    auto *caution = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_kitcaution"));
    auto *broken = dialog->findChild<QCheckBox *>(QStringLiteral("checkBox"));
    auto *itemName = dialog->findChild<QLineEdit *>(QStringLiteral("lineEdit_new_item"));
    auto *initialQuantity = dialog->findChild<QSpinBox *>(QStringLiteral("spinBox_item_quantity_init"));
    auto *currentQuantity = dialog->findChild<QSpinBox *>(QStringLiteral("spinBox_item_quantity_current"));
    auto *addItem = dialog->findChild<QPushButton *>(QStringLiteral("pushButton_addobject"));
    auto *basket = dialog->findChild<QListWidget *>(QStringLiteral("listWidget_itemBasket"));
    auto *confirm = dialog->findChild<QPushButton *>(QStringLiteral("pushButton_OK"));
    QVERIFY(name && code && purchaseDate && purchasePrice && caution && broken && itemName && initialQuantity && currentQuantity && addItem && basket && confirm);
    name->setText(QStringLiteral("Kit test 020"));
    code->setText(QString::fromLatin1(kKit020));
    purchaseDate->setText(QStringLiteral("01.01.2026"));
    purchasePrice->setText(QStringLiteral("1.00"));
    caution->setText(QStringLiteral("0.00"));
    broken->setChecked(false);
    itemName->setText(QString::fromLatin1(kItem020A));
    initialQuantity->setValue(2);
    QVERIFY(currentQuantity->value() == 2);
    QTest::mouseClick(addItem, Qt::LeftButton);
    itemName->setText(QString::fromLatin1(kItem020B));
    initialQuantity->setValue(1);
    QVERIFY(currentQuantity->value() == 1);
    QTest::mouseClick(addItem, Qt::LeftButton);
    QTRY_COMPARE(basket->count(), 2);
    QVERIFY(basket->item(0)->text().contains(QString::fromLatin1(kItem020A)));
    QVERIFY(basket->item(1)->text().contains(QString::fromLatin1(kItem020B)));

    // TC-020 — Étape 2 : valider la création et rechercher le kit par code.
    QTest::mouseClick(confirm, Qt::LeftButton);
    QTRY_VERIFY(visibleTopLevel<PoppupAddKit>() == nullptr);
    auto *findByCode = window->findChild<QLineEdit *>(QStringLiteral("GESKIT_lineEdit_findkitbycode"));
    auto *search = window->findChild<QPushButton *>(QStringLiteral("GESKIT_pushButton_getkit"));
    auto *kitTable = window->findChild<QTableWidget *>(QStringLiteral("GESKIT_tableWidget_kit"));
    QVERIFY(findByCode && search && kitTable);
    findByCode->setText(QString::fromLatin1(kKit020));
    QTest::mouseClick(search, Qt::LeftButton);
    QTRY_COMPARE(kitTable->rowCount(), 1);
    QVERIFY(kitTable->item(0, 0));
    QVERIFY(kitTable->item(0, 0)->text() == QString::fromLatin1(kKit020));
    QVERIFY(kitTable->item(0, 5));
    QCOMPARE(kitTable->item(0, 5)->text(), QStringLiteral("Non"));

    // TC-020 — Étape 3 : afficher les items et contrôler les quantités persistées.
    auto *itemTable = window->findChild<QTableWidget *>(QStringLiteral("GESKIT_tableWidget_item"));
    QVERIFY(itemTable);
    QTest::mouseClick(kitTable->viewport(), Qt::LeftButton, Qt::NoModifier,
                      kitTable->visualItemRect(kitTable->item(0, 0)).center());
    QTRY_COMPARE(itemTable->rowCount(), 2);
    QCOMPARE(itemTable->item(0, 0)->text(), QString::fromLatin1(kItem020A));
    QCOMPARE(itemTable->item(0, 1)->text(), QStringLiteral("2"));
    QCOMPARE(itemTable->item(0, 2)->text(), QStringLiteral("2"));
    QCOMPARE(itemTable->item(1, 0)->text(), QString::fromLatin1(kItem020B));
    QCOMPARE(itemTable->item(1, 1)->text(), QStringLiteral("1"));
    QCOMPARE(itemTable->item(1, 2)->text(), QStringLiteral("1"));
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM kit WHERE code_kit=? AND en_panne=0"),
                             {QString::fromLatin1(kKit020)}), 1);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit020), QString::fromLatin1(kItem020A)}), 2);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity_init FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit020), QString::fromLatin1(kItem020A)}), 2);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity_out FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit020), QString::fromLatin1(kItem020A)}), 0);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit020), QString::fromLatin1(kItem020B)}), 1);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity_init FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit020), QString::fromLatin1(kItem020B)}), 1);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity_out FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit020), QString::fromLatin1(kItem020B)}), 0);
}

void CampaignCases::tc021_broken_kit_not_added_to_reservation_basket()
{
    QVERIFY(fixture.insertKit(QString::fromLatin1(kKit021), QStringLiteral("Kit panne 021"), true));
    auto *window = newLoggedInWindow();
    QVERIFY(window);
    auto *calendar = window->findChild<QCalendarWidget *>(QStringLiteral("RESA_calendarWidget"));
    auto *code = window->findChild<QLineEdit *>(QStringLiteral("RESA_lineEdit_findkitbycode"));
    auto *search = window->findChild<QPushButton *>(QStringLiteral("RESA_pushButton_getkit_resa"));
    auto *available = window->findChild<QListWidget *>(QStringLiteral("RESA_listWidget_resa"));
    auto *basket = window->findChild<QListWidget *>(QStringLiteral("RESA_listWidget_panierResa"));
    QVERIFY(calendar && code && search && available && basket);
    // TC-021 — Étape 1 : afficher le kit en panne à la date sélectionnée.
    const QDate date = nextFriday();
    calendar->setSelectedDate(date);
    QMetaObject::invokeMethod(window, "on_RESA_calendarWidget_clicked", Q_ARG(QDate, date));
    code->setText(QString::fromLatin1(kKit021));
    QTest::mouseClick(search, Qt::LeftButton);
    QTRY_COMPARE(available->count(), 1);
    QVERIFY(available->item(0)->text().contains(QStringLiteral("En panne")));
    // TC-021 — Étape 2 : tenter l'ajout au panier.
    const QRect rect = available->visualItemRect(available->item(0));
    QTest::mouseDClick(available->viewport(), Qt::LeftButton, Qt::NoModifier, rect.center());
    QTRY_COMPARE(basket->count(), 0);
    // TC-021 — Étape 3 : vérifier l'absence de réservation persistée.
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM resa r JOIN kit k ON r.id_kit=k.idkit WHERE k.code_kit=? AND r.start_date=? AND r.is_active=1"),
                             {QString::fromLatin1(kKit021), date}), 0);
}

void CampaignCases::tc031_cancel_reservation_and_restore_availability()
{
    const QDate date = plusSevenDays();
    QVERIFY(fixture.insertUser(QString::fromLatin1(kUser031), QStringLiteral("Reservation"),
                               QStringLiteral("Fixture"), QString::fromLatin1(kUserPassword)));
    QVERIFY(fixture.insertKit(QString::fromLatin1(kKit031), QStringLiteral("Kit reservation 031"), false));
    QVERIFY(fixture.insertKit(QString::fromLatin1(kKit031Witness), QStringLiteral("Kit temoin 031"), false));
    QVERIFY(fixture.insertReservation(QString::fromLatin1(kUser031), QString::fromLatin1(kKit031), date, 31031));
    QVERIFY(fixture.insertReservation(QString::fromLatin1(kUser031), QString::fromLatin1(kKit031Witness), date, 31032));
    const int uidResaCountBefore = fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM uid_resa"));
    const int uidResaMaxBefore = fixture.scalar(QStringLiteral("SELECT COALESCE(MAX(iduid_resa), 0) FROM uid_resa"));

    auto *window = newLoggedInWindow();
    QVERIFY(window);
    auto *user = window->findChild<QLineEdit *>(QStringLiteral("RESA_lineEdit_resa_utinfo_user"));
    auto *show = window->findChild<QPushButton *>(QStringLiteral("RESA_pushButton_resa_showResa"));
    auto *reservations = window->findChild<QListWidget *>(QStringLiteral("RESA_listWidget_resa_currentResa"));
    auto *kitsOfReservation = window->findChild<QListWidget *>(QStringLiteral("RESA_listWidget_resa_kitsOfResa"));
    auto *cancel = window->findChild<QPushButton *>(QStringLiteral("RESA_pushButton_suppr_resa"));
    auto *calendar = window->findChild<QCalendarWidget *>(QStringLiteral("RESA_calendarWidget"));
    auto *kitCode = window->findChild<QLineEdit *>(QStringLiteral("RESA_lineEdit_findkitbycode"));
    auto *findKit = window->findChild<QPushButton *>(QStringLiteral("RESA_pushButton_getkit_resa"));
    auto *availableKits = window->findChild<QListWidget *>(QStringLiteral("RESA_listWidget_resa"));
    QVERIFY(user && show && reservations && kitsOfReservation && cancel && calendar && kitCode && findKit && availableKits);

    // TC-031 — Étape 1 : afficher et sélectionner sans ambiguïté la réservation 31031.
    user->setText(QString::fromLatin1(kUser031));
    QTest::mouseClick(show, Qt::LeftButton);
    QTRY_COMPARE(reservations->count(), 2);
    int reservationRow = -1;
    for (int row = 0; row < reservations->count(); ++row) {
        if (reservations->item(row)->text().startsWith(QStringLiteral("31031 -"))) {
            reservationRow = row;
            break;
        }
    }
    QVERIFY(reservationRow >= 0);
    QTest::mouseClick(reservations->viewport(), Qt::LeftButton, Qt::NoModifier,
                      reservations->visualItemRect(reservations->item(reservationRow)).center());
    QTRY_COMPARE(kitsOfReservation->count(), 1);
    QVERIFY(kitsOfReservation->item(0)->text().contains(QString::fromLatin1(kKit031)));
    QVERIFY(cancel->isEnabled());

    // TC-031 — Étape 2 : confirmer l'annulation complète de la réservation sélectionnée.
    scheduleMessageBoxButton(QMessageBox::Yes);
    QTest::mouseClick(cancel, Qt::LeftButton);
    QTRY_COMPARE(fixture.scalar(QStringLiteral("SELECT is_active FROM resa WHERE id_resa=31031")), 0);
    QTRY_COMPARE(reservations->count(), 1);

    // TC-031 — Étape 3 : contrôler la désactivation et la disponibilité du kit à J+7.
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM resa WHERE id_resa=31031 AND is_active=1")), 0);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM resa WHERE id_resa=31032 AND is_active=1")), 1);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM uid_resa")), uidResaCountBefore);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COALESCE(MAX(iduid_resa), 0) FROM uid_resa")), uidResaMaxBefore);
    calendar->setSelectedDate(date);
    kitCode->setText(QString::fromLatin1(kKit031));
    QTest::mouseClick(findKit, Qt::LeftButton);
    QTRY_COMPARE(availableKits->count(), 1);
    QVERIFY(availableKits->item(0)->text().contains(QString::fromLatin1(kKit031)));
    QVERIFY(!availableKits->item(0)->text().contains(QStringLiteral("déjà réservé à cette date")));

    // TC-031 — Étape 4 : vérifier le log utilisateur+kit et préserver l'enregistrement témoin.
    QCOMPARE(fixture.scalar(QStringLiteral(
                 "SELECT COUNT(*) FROM log l JOIN utilisateur u ON l.id_user=u.id "
                 "JOIN kit k ON l.id_kit=k.idkit WHERE u.utinfo=? AND k.code_kit=? "
                 "AND l.text LIKE ?"),
                 {QString::fromLatin1(kUser031), QString::fromLatin1(kKit031), QStringLiteral("%31031%")} ), 1);
    QCOMPARE(fixture.scalar(QStringLiteral(
                 "SELECT COUNT(*) FROM log l JOIN kit k ON l.id_kit=k.idkit "
                 "WHERE k.code_kit=? AND l.text LIKE ?"),
                 {QString::fromLatin1(kKit031Witness), QStringLiteral("%31031%")} ), 0);
}

void CampaignCases::tc040_output_requires_recipient_password()
{
    const QDate date = plusSevenDays();
    QVERIFY(fixture.insertUser(QString::fromLatin1(kUser040), QStringLiteral("Output"), QStringLiteral("Blocked"), QString::fromLatin1(kUserPassword)));
    QVERIFY(fixture.insertKit(QString::fromLatin1(kKit040), QStringLiteral("Kit sortie 040"), false));
    QVERIFY(fixture.insertItem(QString::fromLatin1(kKit040), QString::fromLatin1(kItem040), 2));
    QVERIFY(fixture.insertReservation(QString::fromLatin1(kUser040), QString::fromLatin1(kKit040), date, 40040));
    auto *window = newLoggedInWindow();
    QVERIFY(window);

    // TC-040 — Étape 1 : sélectionner la réservation sans identifier le bénéficiaire.
    QVERIFY(selectOutputReservation(window, QString::fromLatin1(kUser040), QString::fromLatin1(kKit040)));
    auto *checkout = window->findChild<QPushButton *>(QStringLiteral("SORTIE_pushButton_sortir"));
    auto *status = window->findChild<QLabel *>(QStringLiteral("SORTIE_label_resa_password_status"));
    QVERIFY(checkout && status);
    QVERIFY(!checkout->isEnabled());
    QVERIFY(status->text().contains(QStringLiteral("Mot de passe requis")));

    // TC-040 — Étape 2 : tenter une sortie sans validation du mot de passe.
    checkout->click();
    QVERIFY(!checkout->isEnabled());

    // TC-040 — Étape 3 : vérifier l'absence de sortie ou de mouvement de stock.
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM sortie s JOIN kit k ON s.id_kit=k.idkit WHERE k.code_kit=? AND s.is_active=1"),
                             {QString::fromLatin1(kKit040)}), 0);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit040), QString::fromLatin1(kItem040)}), 2);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT quantity_out FROM item i JOIN kit k ON i.forkey=k.idkit WHERE k.code_kit=? AND i.name=?"),
                             {QString::fromLatin1(kKit040), QString::fromLatin1(kItem040)}), 0);
}

void CampaignCases::tc042_output_with_admin_signature_keeps_recipient()
{
    const QDate date = plusSevenDays();
    QVERIFY(fixture.insertUser(QString::fromLatin1(kUser042), QStringLiteral("Output"), QStringLiteral("Absent"), QString::fromLatin1(kUserPassword)));
    QVERIFY(fixture.insertKit(QString::fromLatin1(kKit042), QStringLiteral("Kit sortie 042"), false));
    QVERIFY(fixture.insertItem(QString::fromLatin1(kKit042), QString::fromLatin1(kItem042), 1));
    QVERIFY(fixture.insertReservation(QString::fromLatin1(kUser042), QString::fromLatin1(kKit042), date, 40042));
    auto *window = newLoggedInWindow();
    QVERIFY(window);

    // TC-042 — Étape 1 : sélectionner la signature administrateur.
    QVERIFY(selectOutputReservation(window, QString::fromLatin1(kUser042), QString::fromLatin1(kKit042)));

    // TC-042 — Étape 2 : valider avec le mot de passe de l'administrateur et sortir le kit.
    QVERIFY(validateOutputPassword(window, fixture.adminPassword, true));
    auto *checkout = window->findChild<QPushButton *>(QStringLiteral("SORTIE_pushButton_sortir"));
    QVERIFY(checkout && checkout->isEnabled());
    QTest::mouseClick(checkout, Qt::LeftButton);
    QVERIFY(verifyPopupItemAndSubmit(1));

    // TC-042 — Étape 3 : vérifier bénéficiaire, trace de délégation et réservation source.
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM sortie s JOIN utilisateur u ON s.id_user=u.id JOIN kit k ON s.id_kit=k.idkit WHERE u.utinfo=? AND k.code_kit=? AND s.is_active=1"),
                             {QString::fromLatin1(kUser042), QString::fromLatin1(kKit042)}), 1);
    QCOMPARE(fixture.scalar(QStringLiteral("SELECT is_active FROM resa r JOIN kit k ON r.id_kit=k.idkit WHERE r.id_resa=40042 AND k.code_kit=?"),
                             {QString::fromLatin1(kKit042)}), 1);
    QVERIFY(fixture.scalar(QStringLiteral("SELECT COUNT(*) FROM log l JOIN kit k ON l.id_kit=k.idkit WHERE k.code_kit=? AND l.text LIKE ?"),
                            {QString::fromLatin1(kKit042), QStringLiteral("%signée par l'administrateur 'TEST_ADMIN'%")}) >= 1);
}

int main(int argc, char *argv[])
{
    const TestDatabaseConfig config = TestDatabaseConfig::fromEnvironment();
    if (!config.requested) {
        std::fputs("SKIPPED: CAMP-001 automated cases require MB_TEST_DB_ENABLED=1 and db4qtest.\n", stderr);
        return 77;
    }
    QApplication application(argc, argv);
    CampaignCases test;
    const int result = QTest::qExec(&test, argc, argv);
    std::fflush(nullptr);
    std::_Exit(result);
}

#include "tc_campaign_cases.moc"
