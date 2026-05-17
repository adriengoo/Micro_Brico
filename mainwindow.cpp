#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "connect_db.h"
#include "datamodel.h"
#include <QDialogButtonBox>
#include <QListWidgetItem>
#include <QTimer>
#include "logger/logbrowser.h"
#include <QPointer>
#include <QDebug>
#include <QString>
#include <QDateTime>
#include <QCloseEvent>
#include <QMessageBox>
#include <QRegularExpression>
#include <QAction>
#include <QMenu>
#include <QEvent>
#include <QLineEdit>


#define DAY_OF_RESA 5
static QString VERSION = "1.0.5";

QPointer<LogBrowser> logBrowser;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if(logBrowser)
        logBrowser->outputMessage( type,msg );
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->ui->splitter_main_logs->setStretchFactor(0, 1);
    this->ui->splitter_main_logs->setStretchFactor(1, 0);
    this->ui->splitter_main_logs->setSizes({650, 140});
    this->GESKIT_show_broken_kits = true;
    this->sortie_resaPasswordValidated = false;
    this->sortie_resaForcedByAdmin = false;
    this->sortie_lastSelectedResaNb = -1;
    this->utinfoCompleterShowPending = false;
    SORTIE_refresh_resa_password_validation_state();

    this->p_resaUtinfoCompleterModel = new QStringListModel(this);
    this->p_resaUtinfoCompleter = new QCompleter(this->p_resaUtinfoCompleterModel, this);
    this->p_sortieUtinfoCompleter = new QCompleter(this->p_resaUtinfoCompleterModel, this);
    auto configureUtinfoCompleter = [](QCompleter *completer) {
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        completer->setMaxVisibleItems(12);
    };
    configureUtinfoCompleter(this->p_resaUtinfoCompleter);
    configureUtinfoCompleter(this->p_sortieUtinfoCompleter);
    this->ui->RESA_lineEdit_resa_utinfo_user->setCompleter(this->p_resaUtinfoCompleter);
    this->ui->SORTIE_lineEdit_utinfo->setCompleter(this->p_sortieUtinfoCompleter);
    this->ui->RESA_lineEdit_resa_utinfo_user->installEventFilter(this);
    this->ui->SORTIE_lineEdit_utinfo->installEventFilter(this);
    connect(this->p_resaUtinfoCompleter, QOverload<const QString &>::of(&QCompleter::activated), this, [this](const QString &utinfo) {
        this->ui->RESA_lineEdit_resa_utinfo_user->setText(utinfo);
        this->on_RESA_pushButton_resa_showResa_clicked();
    });
    connect(this->p_sortieUtinfoCompleter, QOverload<const QString &>::of(&QCompleter::activated), this, [this](const QString &utinfo) {
        this->ui->SORTIE_lineEdit_utinfo->setText(utinfo);
        this->on_SORTIE_pushButton_resa_showResa_clicked();
    });

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "dbtest");

#ifdef NDEBUG //Release compilation
    db.setHostName("192.168.10.120");
    db.setDatabaseName("db_pret_micro");
    db.setUserName("admin");
    db.setPassword("PAPYRUS");
#else //Debug compilation
    db.setHostName("localhost");
    db.setDatabaseName("db_dev");
    db.setUserName("adrien");
    db.setPassword("adrien");
    VERSION += "_DEBUG";
#endif
    db.open();
    g_connect_db.set_db();

    // Set active user in g_db_connect
    g_connect_db.setActiveUser(&(this->login_user));
    this->setWindowTitle("Micro Brico Resa (v"+VERSION+")");

    //Gestion de la bar de status
    this->p_status_bar_label = new QLabel;
    this->statusBar()->addPermanentWidget(this->p_status_bar_label);
    this->p_status_bar_label->setTextFormat(Qt::MarkdownText);

    //Init default buttons status
    this->GESKIT_enable_geskit_buttons(false);
    // Init of the connection status slot
    this->login_user.setIs_logged_on(true);
    update_connection_status(false);
    connect(&g_connect_db, &Connect_db::log_value_changed, this, &MainWindow::update_connection_status);

    // Init of the logger connected to the application slot
    logBrowser = new LogBrowser;
    qInstallMessageHandler(myMessageOutput);
    connect(logBrowser, &LogBrowser::sendMessage, this, &MainWindow::log_stuffs, Qt::QueuedConnection);

    QStringList header_list;
    // Init of User display table
    ui->GESUSER_tableWidget_user->setRowCount(0);
    ui->GESUSER_tableWidget_user->setColumnCount(6);
    header_list.append("UTINFO");
    header_list.append("Prénom");
    header_list.append("Nom");
    header_list.append("Mail");
    header_list.append("Privilège");
    header_list.append("Adhésion payée?");
    ui->GESUSER_tableWidget_user->setHorizontalHeaderLabels(header_list);
    ui->GESUSER_tableWidget_user->setColumnWidth(1, 150);
    ui->GESUSER_tableWidget_user->setColumnWidth(2, 150);
    ui->GESUSER_tableWidget_user->setColumnWidth(3, 250);

    // Init of Kit display table
    header_list.clear();
    ui->GESKIT_tableWidget_kit->setRowCount(0);
    ui->GESKIT_tableWidget_kit->setColumnCount(7);
    header_list.append("Code");
    header_list.append("Nom");
    header_list.append("Caution");
    header_list.append("Date achat");
    header_list.append("Prix d'achat");
    header_list.append("En Panne?");
    header_list.append("Sorti?");
    ui->GESKIT_tableWidget_kit->setHorizontalHeaderLabels(header_list);
    ui->GESKIT_tableWidget_kit->setColumnWidth(1, 250);

    // Init of items display table
    header_list.clear();
    ui->GESKIT_tableWidget_item->setRowCount(0);
    ui->GESKIT_tableWidget_item->setColumnCount(3);
    header_list.append("Nom");
    header_list.append("Qté originale");
    header_list.append("Qté actuelle");
    ui->GESKIT_tableWidget_item->setHorizontalHeaderLabels(header_list);
    ui->GESKIT_tableWidget_item->setColumnWidth(0, 100);
    ui->GESKIT_tableWidget_item->setColumnWidth(1, 80);
    ui->GESKIT_tableWidget_item->setColumnWidth(2, 80);



    // Init date Edit in resa tab to current date
    QDate maxDate;
    maxDate.setDate(9999,12,30);
    this->ui->RESA_calendarWidget->setDateRange(QDate::currentDate(), maxDate);
    this->ui->RESA_calendarWidget->setSelectedDate(this->RESA_get_next_resa_day(QDate::currentDate())); // Set Selected date to next friday
    this->on_RESA_calendarWidget_clicked(this->RESA_get_next_resa_day(QDate::currentDate()));
    this->ui->RESA_pushButton_reserver->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this->ui->RESA_pushButton_reserver, &QPushButton::customContextMenuRequested, this, &MainWindow::on_RESA_pushButton_reserver_customContextMenuRequested);

    // Disable menu items that should not be activable if no user is connected
    this->ui->actionEffacer_les_reservations_pass_es->setEnabled(false);

    this->p_loginConnect = new (Login_connect);
#ifndef NDEBUG //DEBUG compilation
    // Connection automatique avec l'utilisateur "admin" à chaque démarrage
    this->login_user.setUtinfo("admin");
    this->login_user.setMdp("admin");
    this-> on_popupLogin_ok();
#endif


}

MainWindow::~MainWindow()
{
    delete ui;
    logBrowser->close();
    delete logBrowser;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    delete this;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    QLineEdit *utinfoLineEdit = nullptr;
    QCompleter *utinfoCompleter = nullptr;

    if (watched == this->ui->RESA_lineEdit_resa_utinfo_user)
    {
        utinfoLineEdit = this->ui->RESA_lineEdit_resa_utinfo_user;
        utinfoCompleter = this->p_resaUtinfoCompleter;
    }
    else if (watched == this->ui->SORTIE_lineEdit_utinfo)
    {
        utinfoLineEdit = this->ui->SORTIE_lineEdit_utinfo;
        utinfoCompleter = this->p_sortieUtinfoCompleter;
    }

    if (utinfoLineEdit != nullptr
        && utinfoLineEdit->isEnabled()
        && (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress)
        && utinfoCompleter->popup()->isVisible() == false
        && this->utinfoCompleterShowPending == false)
    {
        this->utinfoCompleterShowPending = true;
        QTimer::singleShot(0, this, [this, utinfoLineEdit, utinfoCompleter]() {
            this->utinfoCompleterShowPending = false;
            if (utinfoLineEdit->isEnabled())
            {
                utinfoCompleter->setCompletionPrefix(utinfoLineEdit->text());
                utinfoCompleter->complete();
            }
        });
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::on_actionAfficher_les_logs_appli_triggered()
{
    logBrowser->show();
}

void MainWindow::on_actionEffacer_les_reservations_pass_es_triggered()
{
    RESA_deactivate_outdated_resa();
}

void MainWindow::on_actionafficher_les_logs_kit_triggered()
{
    GESKIT_display_kit_logs();
}

void MainWindow::on_actiondupliquer_kit_triggered()
{
    GESKIT_duplicate_kit();
}

void MainWindow::on_actionmodifier_kit_triggered()
{
    GESKIT_modify_kit();
}

void MainWindow::on_actionajouter_nouveau_kit_triggered()
{
    GESKIT_add_kit();
}


// actions pour GESUSER
void MainWindow::on_actionAfficher_logs_utilisateur_triggered()
{
    GESUSER_display_user_logs();
}

void MainWindow::on_actionAjouter_utilisateur_triggered()
{
    GESUSER_add_new_user();
}

void MainWindow::on_actionModifier_l_utilisateur_triggered()
{
    GESUSER_edit_user();
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    //Context menu for GESKIT table widget only
    if (this->focusWidget() == this->ui->GESKIT_tableWidget_kit)
    {
        QAction show_broken_kits_action("Afficher les kits en panne", this);
        show_broken_kits_action.setCheckable(true);
        show_broken_kits_action.setChecked(this->GESKIT_show_broken_kits);
        connect(&show_broken_kits_action, &QAction::triggered, this, [this](bool checked) {
            this->GESKIT_show_broken_kits = checked;
            this->on_GESKIT_pushButton_getkit_clicked();
        });

        const bool is_kit_selected = this->GESKIT_get_kit_selected() != nullptr;
        this->ui->actionafficher_les_logs_kit->setEnabled(is_kit_selected && this->login_user.getPrivilege() == E_admin);
        this->ui->actiondupliquer_kit->setEnabled(is_kit_selected && this->login_user.getPrivilege() == E_admin);
        this->ui->actionmodifier_kit->setEnabled(is_kit_selected && this->login_user.getPrivilege() == E_admin);

        menu.addAction(&show_broken_kits_action);
        menu.addSeparator();
        menu.addAction(this->ui->actionafficher_les_logs_kit);
        menu.addSeparator();
        menu.addAction(this->ui->actiondupliquer_kit);
        menu.addSeparator();
        menu.addAction(this->ui->actionmodifier_kit);
        menu.addSeparator();
        menu.addAction(this->ui->actionajouter_nouveau_kit);
        menu.exec(event->globalPos());
    }
    //Context menu for GESUSER table widget only
    else if (this->focusWidget() == this->ui->GESUSER_tableWidget_user)
    {
        menu.addAction(this->ui->actionAfficher_logs_utilisateur);
        menu.addSeparator();
        menu.addAction(this->ui->actionNouvel_Utilisateur);
        menu.addSeparator();
        menu.addAction(this->ui->actionModifier_l_utilisateur);
        if (this->GESUSER_get_user_selected() == nullptr) // Only activate menu if a user has been selected
        {
            menu.setEnabled(false);
        }
        menu.exec(event->globalPos());
    }
}


void MainWindow::GEN_raise_popup_info(QString msg)
{

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(msg);
    msgBox.setTextFormat(Qt::TextFormat::MarkdownText);
    msgBox.setWindowTitle("Informations");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    qDebug() << msg;
}


void MainWindow::GEN_raise_popup_warning(QString msg)
{

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(msg);
    msgBox.setTextFormat(Qt::TextFormat::MarkdownText);
    msgBox.setWindowTitle("Attention");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    qDebug() << msg;
}

int MainWindow::GEN_raise_popup_ask_to_continue(QString msg)
{

    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.setTextFormat(Qt::TextFormat::MarkdownText);
    msgBox.setWindowTitle("Attention");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    qDebug() << msg;
    return ret;
}


//---------------------------------------------------------
// vvvvvv POPUP ADD USER SECTION vvvvvv

void MainWindow::on_popupaddUser_destroyed()
{
    qDebug() << "destroyed popup! ;)";
    delete (p_popupAddUser);


}

void MainWindow::on_popupaddUser_ok()
{
    Utilisateur *p_user;
    p_user = this->p_popupAddUser->get_user_from_form();

    if (p_user == NULL)
    {

    }
    else
    {
        if (p_user->getCreate_type() == E_modify) // modify user in database
        {
            if (g_connect_db.update_user(p_user) == false)// no errors occured
            {
                GEN_raise_popup_info("L'utilisateur '"+p_user->getNom() + "' correctement modifié!");
                delete (p_popupAddUser);
            }
            else
            {
                GEN_raise_popup_info("Une erreur est survenue lors de la modification de l'utilisteur.");
            }

        }
        else // insert new user in database
        {
            if (g_connect_db.add_user(p_user) == false)// no errors occured
            {
                GEN_raise_popup_info("Nouvel utilisateur '"+p_user->getNom() + "' correctement ajouté!");
                delete (p_popupAddUser);
            }
            else
            {
                GEN_raise_popup_info("Une erreur est survenue lors de l'ajout de l'utilisateur.");
            }

        }

        this->GESUSER_refresh_user_list_from_server(&this->userList);
        on_GESUSER_pushButton_getuser_clicked();

    }
}

void MainWindow::on_actionNouvel_Utilisateur_triggered()
{
    GESUSER_add_new_user();
}


void MainWindow::on_GESUSER_tableWidget_user_cellDoubleClicked(int row, int column)
{
    GESUSER_edit_user();
}



// ^^^^^^ POPUP ADD USER SECTION ^^^^^^
//---------------------------------------------------------
// vvvvvv POPUP LOGIN SECTION vvvvvv


void MainWindow::on_popupLogin_destroyed()
{
    qDebug() << "destroyed popup! ;)";
    delete (this->p_loginConnect);
}

void MainWindow::on_popupLogin_ok()
{
    qDebug() << "OK popup! ;)";
    if (g_connect_db.connect_user() == true)
    {
        delete (this->p_loginConnect);
        activateWidgets();
    }
    else
    {
        this->GEN_raise_popup_warning("Attention, UTINFO ou mot de passe incorrectes.");
    }

}

void MainWindow::on_actionSe_connecter_triggered()
{
    this->p_loginConnect = new (Login_connect);
    this->p_loginConnect->setUser(&this->login_user);
    this->p_loginConnect->show();
    QObject::connect(this->p_loginConnect->getOkPushButton(), &QPushButton::clicked, this, &MainWindow::on_popupLogin_ok);
    QObject::connect(this->p_loginConnect->getAnnulPushButton(), &QPushButton::clicked, this, &MainWindow::on_popupLogin_destroyed);
}

void MainWindow::on_actionSe_d_connecter_triggered()
{
    update_connection_status(false);
    clean_HMI();
}

// ^^^^^^ POPUP LOGIN SECTION ^^^^^^
//---------------------------------------------------------
// vvvvvv MAIN WINDOW "Gestion Utilisateur" SECTION vvvvvv

void MainWindow::on_GESUSER_pushButton_getuser_clicked()
{

    this->userListView.clear();

    /* if user entered a name, then search for users with this name*/
    if (ui->GESUSER_lineEdit_finduserbyname->text()!="")
    {
        GESUSER_get_user_by_name(&this->userList, &this->userListView, ui->GESUSER_lineEdit_finduserbyname->text());
    }
    /* if user entered a utinfo, then search for users with this utinfo */
    else if (ui->GESUSER_lineEdit_finduserbyutinfo->text()!="")
    {
        GESUSER_get_user_by_utinfo(&this->userList, &this->userListView, ui->GESUSER_lineEdit_finduserbyutinfo->text());
    }
    else
    {
        /* if user entered nothing, get every available kits */
        for(const auto& elem : this->userList)
        {
            this->userListView.push_back(elem);
        }
    }


    MainWindow::GESUSER_refresh_user_table();
}


void MainWindow::GESUSER_get_user_by_name(std::vector<Utilisateur*> *from_user, std::vector<Utilisateur*> *to_user, QString name)
{
    for(const auto& user_elem : *from_user)
    {
        if (user_elem->getNom().contains(name, Qt::CaseInsensitive))
        {
            to_user->push_back(user_elem);
        }
    }
}

void MainWindow::GESUSER_get_user_by_utinfo(std::vector<Utilisateur*> *from_user, std::vector<Utilisateur*> *to_user, QString utinfo)
{
    for(const auto& user_elem : *from_user)
    {
        if (user_elem->getUtinfo().contains(utinfo, Qt::CaseInsensitive))
        {
            to_user->push_back(user_elem);
        }
    }
}

void MainWindow::GESUSER_add_new_user()
{
    T_user_privilege l_privilege_of_connected_user = E_basic;
    this->p_popupAddUser = new popupAddUsers;
    // check if a user is connected when creating a new account, and if so, get its privilege
    if (this->login_user.getIs_logged_on())
    {
        l_privilege_of_connected_user = this->login_user.getPrivilege();
    }
    this->p_popupAddUser->setCaller_privilege(l_privilege_of_connected_user);
    this->p_popupAddUser->setWindowTitle("Ajout d'un nouvel utilisateur");
    this->p_popupAddUser->show_wrapper();
    QObject::connect(this->p_popupAddUser->getOkButton(), &QPushButton::clicked, this, &MainWindow::on_popupaddUser_ok);
    QObject::connect(this->p_popupAddUser->getCancelButton(), &QPushButton::clicked, this, &MainWindow::on_popupaddUser_destroyed);
    QObject::connect(this->p_popupAddUser, &popupAddUsers::delete_popup, this, &MainWindow::on_popupaddUser_destroyed);
}

void MainWindow::GESUSER_edit_user()
{
    T_user_privilege l_privilege_of_connected_user = E_basic;
    Utilisateur* p_user = GESUSER_get_user_selected();
    this->p_popupAddUser = new popupAddUsers;
    // check if a user is connected when creating a new account, and if so, get its privilege
    if (this->login_user.getIs_logged_on())
    {
        l_privilege_of_connected_user = this->login_user.getPrivilege();
    }
    this->p_popupAddUser->setCaller_privilege(l_privilege_of_connected_user);
    this->p_popupAddUser->setWindowTitle("Modification de l'utilisateur: ");
    p_user->setCreate_type(E_modify);
    this->p_popupAddUser->set_form_from_user(p_user);
    this->p_popupAddUser->show_wrapper();
    QObject::connect(this->p_popupAddUser->getOkButton(), &QPushButton::clicked, this, &MainWindow::on_popupaddUser_ok);
    QObject::connect(this->p_popupAddUser->getCancelButton(), &QPushButton::clicked, this, &MainWindow::on_popupaddUser_destroyed);
    QObject::connect(this->p_popupAddUser, &popupAddUsers::delete_popup, this, &MainWindow::on_popupaddUser_destroyed);
}

void MainWindow::activateWidgets(void)
{
    bool do_enable = false;
    if (login_user.getPrivilege() == E_basic)
    {
        this->ui->tabwidget->setCurrentIndex(2);//By default, when user connects, open on the reservation page

        //Do not let user book items if adhesion was not payed
        if (login_user.getAdhesion_payed() == false)
        {
            this->ui->TAB_RESA->setEnabled(false);

            GEN_raise_popup_info("Attention, vous n'avez pas réglé votre adhésion cette année, vous ne pouvez donc pas réserver de matériel. Veuillez demander à un bénévol la marche à suivre.");
        }
        else
        {
            this->ui->TAB_RESA->setEnabled(true);
        }

        do_enable = false;
    }
    else
    {
        do_enable = true;
        this->ui->TAB_RESA->setEnabled(true);// always enable reservation tab when admin is connected

        //Automatically populates geskit and gesuser table if admin connects
        on_GESKIT_pushButton_getkit_clicked();
        on_GESUSER_pushButton_getuser_clicked();
    }
    this->ui->TAB_ges_user->setEnabled(do_enable);
    this->ui->TAB_ges_kits->setEnabled(do_enable);
    this->ui->TAB_SORTIE->setEnabled(do_enable);

}

void MainWindow::GESUSER_refresh_user_table(void)
{
    int row = 0;

    for(const auto& user_elem : this->userListView)
    {
        row++;
        GESUSER_push_back_new_user_on_table(user_elem, row);
    }
}


void MainWindow::GESUSER_clear_display()
{
    // Block signal because the clearing of contents when "currentCellChanged" signal is activated causes crash for some reasons
    this->ui->GESUSER_tableWidget_user->blockSignals(true);
    this->ui->GESUSER_tableWidget_user->clearContents();
    this->ui->GESUSER_tableWidget_user->setRowCount(0);
    this->ui->GESUSER_tableWidget_user->blockSignals(false);
}


Utilisateur *MainWindow::GESUSER_get_user_selected()
{
    QList<QTableWidgetSelectionRange> items = this->ui->GESUSER_tableWidget_user->selectedRanges();
    if (items.size() != 0)
    {
        QTableWidgetSelectionRange  selectedRange = items.first();
        int row = selectedRange.topRow();
        Utilisateur* p_user = userListView.at(row);
        return p_user;
    }
    return nullptr;

}

void MainWindow::GESUSER_refresh_user_list_from_server(std::vector<Utilisateur *> *i_list)
{
    /* clean the kit list and delete objects pointed by element of this list */
    g_utils.clearList(i_list);
    // clear every depending lists
    this->userListView.clear();

    //Get every kits on server
    g_connect_db.select_all_users(i_list);
    RESA_refresh_user_utinfo_completer();
}

void MainWindow::GESUSER_push_back_new_user_on_table(Utilisateur *user, int row)
{
    uint column_index = 0;
    ui->GESUSER_tableWidget_user->setRowCount(row);
    QTableWidgetItem* p_widget_user_utinfo;
    QTableWidgetItem* p_widget_user_prenom;
    QTableWidgetItem* p_widget_user_nom;
    QTableWidgetItem* p_widget_user_mail;
    QTableWidgetItem* p_widget_user_privilege;
    QTableWidgetItem* p_widget_user_adhesion_payed;
    QString l_privilege;
    QString l_adhesion_payed;
    // set Utinfo
    p_widget_user_utinfo= new QTableWidgetItem(user->getUtinfo());
    ui->GESUSER_tableWidget_user->setItem(row-1, column_index, p_widget_user_utinfo);
    column_index ++;
    // set Prenom
    p_widget_user_prenom= new QTableWidgetItem(user->getPrenom());
    ui->GESUSER_tableWidget_user->setItem(row-1, column_index, p_widget_user_prenom);
    column_index ++;
    // set Nom
    p_widget_user_nom= new QTableWidgetItem(user->getNom());
    ui->GESUSER_tableWidget_user->setItem(row-1, column_index, p_widget_user_nom);
    column_index ++;
    // set Mail
    p_widget_user_mail= new QTableWidgetItem(user->getEmail());
    ui->GESUSER_tableWidget_user->setItem(row-1, column_index, p_widget_user_mail);
    column_index ++;

    // set Privilege
    if (user->getPrivilege() == E_basic)
    {
        l_privilege = "Utilisateur";
    }
    else
    {
        l_privilege = "Administrateur";
    }
    p_widget_user_privilege= new QTableWidgetItem(l_privilege);
    ui->GESUSER_tableWidget_user->setItem(row-1, column_index, p_widget_user_privilege);
    column_index ++;
    if (user->getAdhesion_payed() == true)
    {
        l_adhesion_payed = "Oui";
    }
    else
    {
        l_adhesion_payed = "Non";
    }
    p_widget_user_adhesion_payed= new QTableWidgetItem(l_adhesion_payed);
    ui->GESUSER_tableWidget_user->setItem(row-1, column_index, p_widget_user_adhesion_payed);
    column_index ++;

}

void MainWindow::GESUSER_display_user_logs()
{
    //First get user selected
    Utilisateur* p_user = GESUSER_get_user_selected();
    if (p_user != nullptr)
    {
        this->p_log_user_kit_appli = new (log_user_kit_appli);
        this->p_log_user_kit_appli->setUser(p_user);
        this->p_log_user_kit_appli->setWindowTitle("Logs de l'utilisateur: "+p_user->getUtinfo());
        this->p_log_user_kit_appli->update_logs_from_user(100);
        this->p_log_user_kit_appli->show();
    }
}


// ^^^^^^ MAIN WINDOW "Gestion Utilisateur" ^^^^^^
//---------------------------------------------------------

//---------------------------------------------------------
// vvvvvv MAIN WINDOW "Gestion Kits" SECTION vvvvvv

void MainWindow::GESKIT_get_kits_by_code(std::vector<Kit*> *from_kits, std::vector<Kit*> *to_kits, QString code)
{

    for(const auto& kit_elem : *from_kits)
    {
        if (kit_elem->getCode().contains(code, Qt::CaseInsensitive))
        {
            to_kits->push_back(kit_elem);
        }
    }
}

void MainWindow::GESKIT_get_kits_by_name(std::vector<Kit*> *from_kits, std::vector<Kit*> *to_kits, QString code)
{
    for(const auto& kit_elem : *from_kits)
    {
        if (kit_elem->getNom().contains(code, Qt::CaseInsensitive))
        {
            to_kits->push_back(kit_elem);
        }
    }
}

///
/// \brief MainWindow::GESKIT_clear_display
/// Clears every tables and textEdit in GESKIT tab
///
void MainWindow::GESKIT_clear_display(void)
{
    this->ui->GESKIT_textEdit_descritpionKit->clear();
    this->ui->GESKIT_tableWidget_item->clearContents();
    this->ui->GESKIT_tableWidget_item->setRowCount(0);

    // Block signal because the clearing of contents when "currentCellChanged" signal is activated causes crash for some reasons
    this->ui->GESKIT_tableWidget_kit->blockSignals(true);
    this->ui->GESKIT_tableWidget_kit->clearContents();
    this->ui->GESKIT_tableWidget_kit->setRowCount(0);
    this->ui->GESKIT_tableWidget_kit->blockSignals(false);
}

void MainWindow::GESKIT_enable_geskit_buttons(bool i_enable)
{
    bool l_enable = false;
    //Enable buttons only if user is admin
    if (this->login_user.getPrivilege() == E_admin)
    {
        l_enable = i_enable;
        this->ui->GESKIT_pushButton_addkit->setEnabled(true); // always enable this one when admin
    }
    else
    {
        l_enable = false;
        this->ui->GESKIT_pushButton_addkit->setEnabled(false); // always enable this one when admin
    }

    this->ui->GESKIT_pushButton_duplicate_kit->setEnabled(l_enable);
    this->ui->GESKIT_pushButton_modify_kit->setEnabled(l_enable);
}

void MainWindow::GESKIT_refresh_kit_list_from_server(std::vector<Kit*> *i_list)
{
    std::vector<Sortie *> l_sortie;
    /* clean the kit list and delete objects pointed by element of this list */
    g_utils.clearList(i_list);
    // clear every depending lists
    this->kitListBasket_view.clear();
    this->kitListGeskit_view.clear();
    this->kitListResa_view.clear();
    this->kitListSortie_kitsOfResaView.clear();
    this->ui->RESA_listWidget_panierResa->clear();
    this->ui->RESA_listWidget_resa->clear();
    GESKIT_clear_display();
    //Get every kits on server and find if kit is out
    g_connect_db.select_all_kits(i_list);
    g_connect_db.select_all_active_sortie(&l_sortie);

    for(const auto& sortie_elem : l_sortie)
    {
        for(const auto& kit_elem : *i_list)
        {
            if (kit_elem->getIdKit() == sortie_elem->getId_kit())
            {
                kit_elem->setIs_out(true);
                break;
            }
        }
    }

}



void MainWindow::on_GESKIT_lineEdit_findkitbyname_returnPressed()
{
    on_GESKIT_pushButton_getkit_clicked();
}


void MainWindow::on_GESKIT_lineEdit_findkitbycode_returnPressed()
{
    on_GESKIT_pushButton_getkit_clicked();
}

void MainWindow::on_GESKIT_pushButton_getkit_clicked()
{
    this->kitListGeskit_view.clear();
    /* if user enterd a code, then search for Kits with this code */
    if (ui->GESKIT_lineEdit_findkitbycode->text()!="")
    {
        GESKIT_get_kits_by_code(&this->kitList,&this->kitListGeskit_view,ui->GESKIT_lineEdit_findkitbycode->text() );
    }

    /* if user enterd a text, then search for Kits with this code */
    else if (ui->GESKIT_lineEdit_findkitbyname->text()!="")
    {
        GESKIT_get_kits_by_name(&this->kitList,&this->kitListGeskit_view,ui->GESKIT_lineEdit_findkitbyname->text() );
    }
    else
    {        /* if user enterd nothing, get every available kits */
        for(const auto& kit_elem : this->kitList)
        {
            this->kitListGeskit_view.push_back(kit_elem);
        }
    }

    if (this->GESKIT_show_broken_kits == false)
    {
        std::vector<Kit*> filtered_kits;
        for(const auto& kit_elem : this->kitListGeskit_view)
        {
            if (kit_elem->getEn_panne() == false)
            {
                filtered_kits.push_back(kit_elem);
            }
        }
        this->kitListGeskit_view = filtered_kits;
    }

    // If user gives an input that leads to 0 matches, then disable duplicate button
    if(this->kitListGeskit_view.size() == 0)
    {
        this->GESKIT_enable_geskit_buttons(false);
    }
    MainWindow::GESKIT_refresh_kit_table();
}

void MainWindow::GESKIT_refresh_kit_table(void)
{
    vector<Kit*>::iterator it;
    int row = 0;

    this->ui->GESKIT_tableWidget_kit->blockSignals(true);
    this->ui->GESKIT_tableWidget_kit->clearContents();
    this->ui->GESKIT_tableWidget_kit->setRowCount(0);
    this->ui->GESKIT_tableWidget_kit->blockSignals(false);

    for(const auto& kit_elem : this->kitListGeskit_view)
    {
        row++;
        GESKIT_push_back_new_kit_on_table(kit_elem, row);
    }
}

void MainWindow::GESKIT_push_back_new_kit_on_table(Kit* kit, int row)
{
    uint column_index = 0;
    ui->GESKIT_tableWidget_kit->setRowCount(row);
    QTableWidgetItem* p_widget_item_nom;
    QTableWidgetItem* p_widget_item_code;
    QTableWidgetItem* p_widget_item_caution;
    QTableWidgetItem* p_widget_item_date;
    QTableWidgetItem* p_widget_item_prix;
    QTableWidgetItem* p_widget_item_etat;
    QTableWidgetItem* p_widget_isOut;
    Utilisateur l_user;
    QBrush brush_out = Qt::red;
    QBrush brush;
    // Change line color depending on kit status
    if (kit->getIs_out() == true)
    {
        brush = Qt::gray;
        //Get user having the kit out informations
        g_connect_db.get_user_who_has_kit_out(kit, &l_user);
    }
    else if (kit->getEn_panne() == true)
    {
        brush = Qt::red;
    }
    else
    {
        brush = Qt::black;
    }

    // Set second item of last row (code)
    p_widget_item_code= new QTableWidgetItem(kit->getCode());
    p_widget_item_code->setForeground(brush);
    ui->GESKIT_tableWidget_kit->setItem(row-1, column_index, p_widget_item_code);
    column_index ++;

    // Set first item of last row (name)
    p_widget_item_nom= new QTableWidgetItem(kit->getNom());
    p_widget_item_nom->setForeground(brush);
    ui->GESKIT_tableWidget_kit->setItem(row-1, column_index, p_widget_item_nom);
    column_index ++;

    // Set third item of last row (caution)
    p_widget_item_caution= new QTableWidgetItem(kit->getCaution().getStringValue());
    p_widget_item_caution->setForeground(brush);
    ui->GESKIT_tableWidget_kit->setItem(row-1, column_index, p_widget_item_caution);
    column_index ++;

    // Set fourth item of last row (date)
    QString date = kit->getDate_achat().toString("dd.MM.yyyy");
    p_widget_item_date= new QTableWidgetItem(date);
    p_widget_item_date->setForeground(brush);
    ui->GESKIT_tableWidget_kit->setItem(row-1, column_index, p_widget_item_date);
    column_index ++;

    // Set fifth item of last row (prix)
    p_widget_item_prix= new QTableWidgetItem(kit->getPrix_achat().getStringValue());
    p_widget_item_prix->setForeground(brush);
    ui->GESKIT_tableWidget_kit->setItem(row-1, column_index, p_widget_item_prix);
    column_index ++;

    // Set sixth item of last row (etat)
    p_widget_item_etat= new QTableWidgetItem(kit->getEn_panne_str());
    p_widget_item_etat->setForeground(brush);
    ui->GESKIT_tableWidget_kit->setItem(row-1, column_index, p_widget_item_etat);
    column_index ++;

    // Set seventh item of last row (Is out?)
    QString str = kit->getIs_out_str();
    if (kit->getIs_out())
    {
        str = str + " ("+l_user.getUtinfo()+")"; //only set utinfo if the kit is out
    }
    p_widget_isOut= new QTableWidgetItem( str );
    p_widget_isOut->setForeground(brush);
    ui->GESKIT_tableWidget_kit->setItem(row-1, column_index, p_widget_isOut);
    column_index ++;
}

void MainWindow::on_GESKIT_tableWidget_kit_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    if (currentRow >= 0 && currentRow < (int)this->kitListGeskit_view.size() && currentRow != previousRow)
    {
        Kit* k = this->kitListGeskit_view[currentRow];
        // Get every items associated with kit from server
        if (k->item_list.empty())
        {
            g_connect_db.select_items_by_kit(k);
        }

        GESKIT_enable_geskit_buttons(true);

        GESKIT_refresh_item_list_table(k);
        GESKIT_refresh_descritption(k);
    }
}

///
/// \brief MainWindow::on_GESKIT_tableWidget_kit_cellDoubleClicked
///     Function used to modify Kit easily
///
void MainWindow::on_GESKIT_tableWidget_kit_cellDoubleClicked(int row, int column)
{
    this->on_GESKIT_pushButton_modify_kit_clicked();
}

void MainWindow::GESKIT_refresh_descritption(Kit* kit)
{
    this->ui->GESKIT_textEdit_descritpionKit->setText(kit->getDescription());
}
/*
This Function updates the item table with every items
associated with the currently selected kit
*/
void MainWindow::GESKIT_refresh_item_list_table(Kit* kit)
{
    vector<Item*>::iterator it;
    int row = 0;
    this->ui->GESKIT_tableWidget_item->clearContents();
    for (it = kit->item_list.begin(); it != kit->item_list.end(); it++)
    {
        row++;
        GESKIT_push_back_new_item_on_table((*it), row);
    }
}

void MainWindow::GESKIT_push_back_new_item_on_table(Item* item, int row)
{
    uint8_t column_index = 0;
    ui->GESKIT_tableWidget_item->setRowCount(row);

    // Set first item of last row (name)
    QTableWidgetItem* p_widget_item_name = new QTableWidgetItem(item->getName());
    ui->GESKIT_tableWidget_item->setItem(row-1, column_index, p_widget_item_name);
    column_index++;

    // Set second item of last row (Quantity init)
    QTableWidgetItem* p_widget_item_quantity= new QTableWidgetItem(QString::number(item->getQuantity_init()));
    ui->GESKIT_tableWidget_item->setItem(row-1, column_index, p_widget_item_quantity);
    column_index++;

    // Set second item of last row (Quantity current)
    QTableWidgetItem* p_widget_item_quantity_current= new QTableWidgetItem(QString::number(item->getQuantity_current()));
    ui->GESKIT_tableWidget_item->setItem(row-1, column_index, p_widget_item_quantity_current);
    column_index++;
}

Kit * MainWindow::GESKIT_find_kit_by_id(int id)
{
    for(const auto& kit_elem : this->kitList)
    {
        if (kit_elem->getIdKit() == id)
        {
            return kit_elem;
        }
    }
    return nullptr;
}


void MainWindow::on_GESKIT_pushButton_logkit_clicked()
{
    GESKIT_display_kit_logs();
}

void MainWindow::GESKIT_display_kit_logs()
{
    //First get kit selected by user
    Kit* p_kit = GESKIT_get_kit_selected();
    if (p_kit != nullptr)
    {
        this->p_log_user_kit_appli = new (log_user_kit_appli);
        this->p_log_user_kit_appli->setKit(p_kit);
        this->p_log_user_kit_appli->setWindowTitle("Logs du kit: "+p_kit->getNom());
        this->p_log_user_kit_appli->update_logs_from_kit(100);
        this->p_log_user_kit_appli->show();
    }
}



// ^^^^^^ MAIN WINDOW "Gestion Kits" ^^^^^^
//---------------------------------------------------------



//---------------------------------------------------------
// vvvvvv POPUP DELETE_USER SECTION vvvvvv




void MainWindow::on_popupDelete_destroyed()
{
    delete (this->p_popupDeleteUser);
}

void MainWindow::on_popupDelete_ok()
{
    Utilisateur tmp_user;
    tmp_user.setEmail(this->p_popupDeleteUser->getMail());
    tmp_user.setUtinfo(this->p_popupDeleteUser->getUtinfo());
    g_connect_db.delete_user(&tmp_user);

    delete (this->p_popupDeleteUser);
}



// ^^^^^^ POPUP DELETE_USER SECTION ^^^^^^
//---------------------------------------------------------


//---------------------------------------------------------
// vvvvvv SLOTS SECTION vvvvvv
void MainWindow::clean_HMI(void)
{
    //Clear every table, list and line edit
    this->ui->GESUSER_lineEdit_finduserbyname->clear();
    this->ui->GESUSER_lineEdit_finduserbyutinfo->clear();
    this->GESKIT_clear_display();
    this->GESUSER_clear_display();
    this->ui->GESKIT_lineEdit_findkitbycode->clear();
    this->ui->GESKIT_lineEdit_findkitbyname->clear();
    this->ui->RESA_lineEdit_findkitbycode->clear();
    this->ui->RESA_lineEdit_findkitbyname->clear();
    this->ui->RESA_lineEdit_resa_utinfo_user->clear();
    this->ui->RESA_listWidget_panierResa->clear();
    this->ui->RESA_listWidget_resa->clear();
    this->ui->RESA_listWidget_resa_currentResa->clear();
    this->ui->RESA_listWidget_resa_kitsOfResa->clear();

    //Clear every vector lists
    g_utils.clearList(&this->userList);
    g_utils.clearList(&this->kitList);
    g_utils.clearList(&this->resaList);
    RESA_refresh_user_utinfo_completer();

}

void MainWindow::update_connection_status(bool is_user_logged)
{
    if (is_user_logged != this->login_user.getIs_logged_on())
    {
        this->login_user.setIs_logged_on(is_user_logged);
        this->ui->tabwidget->setEnabled(is_user_logged);
        this->ui->TAB_ges_user->setEnabled(is_user_logged);
        this->ui->TAB_ges_kits->setEnabled(is_user_logged);
        this->ui->actionSe_connecter->setEnabled(!is_user_logged);
        this->ui->actionSe_d_connecter->setEnabled(is_user_logged);
        this->GESKIT_enable_geskit_buttons(false);


        if (is_user_logged == true)
        {
            qInfo() << "Vous êtes maintenant connecté.";
            this->p_status_bar_label->setStyleSheet("QLabel { color : blue; }");
            this->p_status_bar_label->setText("Connecté en tant que: **"+ this->login_user.getUtinfo()+"**");

            //Refresh kit list every time a user logs in
            this->GESKIT_refresh_kit_list_from_server(&this->kitList);
            //Refresh user list every time a user logs in
            this->GESUSER_refresh_user_list_from_server(&this->userList);

            //Refresh resa RESA_lineEdit_resa_utinfo_user with user name and enable it only if user is admin
            this->ui->RESA_lineEdit_resa_utinfo_user->setText(this->login_user.getUtinfo());

            if (this->login_user.getPrivilege() == E_admin)
            {
                this->ui->RESA_lineEdit_resa_utinfo_user->setEnabled(true);

                // Enable some actions only for admins
                this->ui->actionEffacer_les_reservations_pass_es->setEnabled(true);
                this->ui->actionAfficher_les_logs_appli->setEnabled(true);
                this->ui->actionafficher_les_logs_kit->setEnabled(true);
                this->ui->actionAfficher_logs_utilisateur->setEnabled(true);
                this->ui->actionajouter_nouveau_kit->setEnabled(true);
                this->ui->actiondupliquer_kit->setEnabled(true);
                this->ui->actionmodifier_kit->setEnabled(true);
                this->ui->actionModifier_l_utilisateur->setEnabled(true);
            }
            else
            {
                this->ui->RESA_lineEdit_resa_utinfo_user->setEnabled(false);

                // Disable actions for user
                this->ui->actionEffacer_les_reservations_pass_es->setEnabled(false);
                this->ui->actionAfficher_les_logs_appli->setEnabled(false);
                this->ui->actionafficher_les_logs_kit->setEnabled(false);
                this->ui->actionAfficher_logs_utilisateur->setEnabled(false);
                this->ui->actionajouter_nouveau_kit->setEnabled(false);
                this->ui->actiondupliquer_kit->setEnabled(false);
                this->ui->actionmodifier_kit->setEnabled(false);
                this->ui->actionModifier_l_utilisateur->setEnabled(false);
            }
            // Display every kits on resa view for a more user friendly approach. He can filter after if he wants
            this->on_RESA_pushButton_getkit_resa_clicked();
            this->on_RESA_pushButton_resa_showResa_clicked();

        }
        else
        {
            qInfo() << "Vous êtes maintenant déconnecté.";
            this->p_status_bar_label->setStyleSheet("QLabel { color : red; }");
            this->p_status_bar_label->setText("Déconnecté");

            //Clear lineEdit_resa_email_use
            this->ui->RESA_lineEdit_resa_utinfo_user->setText("");
            this->ui->RESA_pushButton_suppr_resa->setEnabled(false);
            this->ui->RESA_pushButton_reserver->setEnabled(false);
            this->ui->SORTIE_pushButton_sortir->setEnabled(false);
            this->ui->pushButton_restituerKit->setEnabled(false);
            this->ui->SORTIE_pushButton_retirer_kit_from_resa->setEnabled(false);


            // Disable some actions when disconected
            this->ui->actionEffacer_les_reservations_pass_es->setEnabled(false);
            this->ui->actionAfficher_les_logs_appli->setEnabled(false);
            this->ui->actionafficher_les_logs_kit->setEnabled(false);
            this->ui->actionAfficher_logs_utilisateur->setEnabled(false);
            this->ui->actionajouter_nouveau_kit->setEnabled(false);
            this->ui->actiondupliquer_kit->setEnabled(false);
            this->ui->actionmodifier_kit->setEnabled(false);
            this->ui->actionModifier_l_utilisateur->setEnabled(false);
        }
    }
}


void MainWindow::log_stuffs(QtMsgType type, const QString &msg)
{
    QDateTime date ;
    QString date_str = date.currentDateTime().toString("ddd d MMM yyyy hh:mm:ss");
    switch (type) {
    default:
    case QtInfoMsg:
        this->ui->textEditLogs->append(date_str +" : "+ msg);
        break;

    case QtWarningMsg:
        this->ui->textEditLogs->append(date_str +tr(" : — WARNING: %1").arg(msg));
        break;

    case QtCriticalMsg:
        this->ui->textEditLogs->append(date_str +tr(" : — CRITICAL: %1").arg(msg));
        break;

    case QtFatalMsg:
        this->ui->textEditLogs->append(date_str +tr(" : — FATAL: %1").arg(msg));
        break;

    case QtDebugMsg:
        // this->ui->textEditLogs->append(date_str +tr(" : — DEBUG: %1").arg(msg));
        break;

    }
}

// ^^^^^^ SLOTS SECTION ^^^^^^
//---------------------------------------------------------

//---------------------------------------------------------
// vvvvvv POPUP Add_kit SECTION vvvvvv


void MainWindow::on_GESKIT_pushButton_addkit_clicked()
{
    GESKIT_add_kit();
}


void MainWindow::GESKIT_add_kit()
{
    this->p_popupAddKit = new (PoppupAddKit);
    this->p_popupAddKit->setWindowTitle("Ajout d'un nouveau Kit");
    this->p_popupAddKit->show();
    QObject::connect(this->p_popupAddKit->getOkButton(), &QPushButton::clicked, this, &MainWindow::on_popupAddKit_ok);
    QObject::connect(this->p_popupAddKit->getCancelButton(), &QPushButton::clicked, this, &MainWindow::on_popupAddKit_destroyed);
    QObject::connect(this->p_popupAddKit, &PoppupAddKit::delete_popup, this, &MainWindow::on_popupAddKit_destroyed);
}

void MainWindow::on_popupAddKit_destroyed()
{
    // this->p_popupAddKit->close();
    delete (this->p_popupAddKit);
}

void MainWindow::on_popupAddKit_ok()
{
    Utilisateur tmp_user;
    Kit * p_kit;
    std::list<Item*> p_list_item;
    p_kit = this->p_popupAddKit->get_kit_from_form();
    if (p_kit == NULL)
    {

    }
    else if (p_kit->getTo_duplicate() == true)
    {
        g_connect_db.add_kit(p_kit);
        // this->p_popupAddKit->close();
        delete (this->p_popupAddKit);
        /* refresh the kit list by cleaning and loading it again */
        this->GESKIT_refresh_kit_list_from_server(&this->kitList);

        //Refresh display of kits
        on_GESKIT_pushButton_getkit_clicked();
    }
    else //if (p_kit->getTo_duplicate() == false)
    {
        g_connect_db.update_kit(p_kit);
        // this->p_popupAddKit->close();
        delete (this->p_popupAddKit);
        /* refresh the kit list by cleaning and loading it again */
        this->GESKIT_refresh_kit_list_from_server(&this->kitList);

        //Refresh display of kits
        on_GESKIT_pushButton_getkit_clicked();
    }
}


Kit* MainWindow::GESKIT_get_kit_selected()
{
    //First get kit selected by user

    QList<QTableWidgetSelectionRange> items = this->ui->GESKIT_tableWidget_kit->selectedRanges();
    if (items.size() !=0)
    {
        QTableWidgetSelectionRange  selectedRange = items.first();
        int row = selectedRange.topRow();
        Kit* p_kit = kitListGeskit_view.at(row);
        return p_kit;

    }
    return nullptr;
}

void MainWindow::on_GESKIT_pushButton_duplicate_kit_clicked()
{
    GESKIT_duplicate_kit();
}

void MainWindow::GESKIT_duplicate_kit()
{
    //First get kit selected by user
    Kit* p_kit = GESKIT_get_kit_selected();
    if (p_kit !=nullptr)
    {
        p_kit->setTo_duplicate(true);

        //Open popup pre filled with selected kit informations
        this->p_popupAddKit = new (PoppupAddKit);
        this->p_popupAddKit->set_form_from_kit(p_kit);
        this->p_popupAddKit->setWindowTitle("Duplication du kit : "+ p_kit->getNom());
        this->p_popupAddKit->show();
        QObject::connect(this->p_popupAddKit->getOkButton(), &QPushButton::clicked, this, &MainWindow::on_popupAddKit_ok);
        QObject::connect(this->p_popupAddKit->getCancelButton(), &QPushButton::clicked, this, &MainWindow::on_popupAddKit_destroyed);
        QObject::connect(this->p_popupAddKit, &PoppupAddKit::delete_popup, this, &MainWindow::on_popupAddKit_destroyed);

    }
}

void MainWindow::on_GESKIT_pushButton_modify_kit_clicked()
{
    GESKIT_modify_kit();
}

void MainWindow::GESKIT_modify_kit()
{
    //First get kit selected by user
    Kit* p_kit = GESKIT_get_kit_selected();
    if (p_kit !=nullptr)
    {
        p_kit->setTo_duplicate(false);

        //Open popup pre filled with selected kit informations
        this->p_popupAddKit = new (PoppupAddKit);
        this->p_popupAddKit->set_form_from_kit(p_kit);
        this->p_popupAddKit->setWindowTitle("Modification du kit : "+ p_kit->getNom());
        this->p_popupAddKit->show();
        QObject::connect(this->p_popupAddKit->getOkButton(), &QPushButton::clicked, this, &MainWindow::on_popupAddKit_ok);
        QObject::connect(this->p_popupAddKit->getCancelButton(), &QPushButton::clicked, this, &MainWindow::on_popupAddKit_destroyed);
        QObject::connect(this->p_popupAddKit, &PoppupAddKit::delete_popup, this, &MainWindow::on_popupAddKit_destroyed);
    }
}

// ^^^^^^ POPUP Add_kit SECTION ^^^^^^
//---------------------------------------------------------


//---------------------------------------------------------
// vvvvvv MAIN WINDOW "Gestion Reservation" vvvvvv



///
/// \brief MainWindow::RESA_get_next_resa_day. Seek next "RESA_DAY" relatively to start_date.
/// \param start_date: day of the week from which "RESA_DAY" must be searched
/// \return
///
QDate MainWindow::RESA_get_next_resa_day(QDate start_date)
{
    QDate next_friday;
    int remaining_days_to_resa_day = 0;
    remaining_days_to_resa_day = DAY_OF_RESA - start_date.dayOfWeek();
    if (remaining_days_to_resa_day < 0)// saturday or sunday
    {
        next_friday = start_date.addDays(remaining_days_to_resa_day+7);
    }
    else if (remaining_days_to_resa_day > 0) //every days except friday
    {
        next_friday = start_date.addDays(remaining_days_to_resa_day);
    }
    else
    {
        next_friday = start_date;
    }
    return next_friday;
}

void MainWindow::on_RESA_calendarWidget_clicked(const QDate &date)
{
    if(date.dayOfWeek() == DAY_OF_RESA) // if Day_of_resa
    {

        ui->RESA_pushButton_reserver->setEnabled(true);
        ui->RESA_pushButton_getkit_resa->setEnabled(true);


        //Find out if kits in kit list are already booked
        if (this->kitList.size() > 0)
        {
            g_connect_db.set_kit_booked_status(&this->kitList, this->ui->RESA_calendarWidget->selectedDate());
        }


        RESA_refresh_kit_list_table();
        RESA_refresh_basket_kit_list_table();
    }
    else
    {
        qInfo()<< "Date de début: Vous devez choisir un vendredi!";
        //Force date to next friday from selected day
        this->ui->RESA_calendarWidget->setSelectedDate(this->RESA_get_next_resa_day(this->ui->RESA_calendarWidget->selectedDate()));
        this->on_RESA_calendarWidget_clicked(this->ui->RESA_calendarWidget->selectedDate());
    }
}

void MainWindow::on_RESA_lineEdit_resa_utinfo_user_returnPressed()
{
    on_RESA_pushButton_resa_showResa_clicked();
}




void MainWindow::on_RESA_lineEdit_findkitbyname_returnPressed()
{
    on_RESA_pushButton_getkit_resa_clicked();
}


void MainWindow::on_RESA_lineEdit_findkitbycode_returnPressed()
{
    on_RESA_pushButton_getkit_resa_clicked();
}

void MainWindow::on_RESA_pushButton_getkit_resa_clicked()
{


    // Set property "is_in_resa_view"" of items in kitListResa_view to "false"
    for(const auto& kit_elem : this->kitListResa_view)
    {
        kit_elem->setIs_in_resa_view(false);
    }
    this->kitListResa_view.clear();

    /* if user entered a code, then search for Kits with this code */
    if (ui->RESA_lineEdit_findkitbycode->text()!="")
    {
        RESA_get_kits_by_code(&this->kitList,&this->kitListResa_view,ui->RESA_lineEdit_findkitbycode->text() );
    }
    /* if user entered a text, then search for Kits with this code */
    else if (ui->RESA_lineEdit_findkitbyname->text()!="")
    {
        RESA_get_kits_by_name(&this->kitList,&this->kitListResa_view,ui->RESA_lineEdit_findkitbyname->text() );
    }
    else
    {
        /* if user entered nothing, get every available kits */
        for(const auto& kit_elem : this->kitList)
        {
            this->kitListResa_view.push_back(kit_elem);
        }
    }


    // Refresh kits in basket to see if none booked them
    g_connect_db.set_kit_booked_status(&this->kitListResa_view, this->ui->RESA_calendarWidget->selectedDate());
    MainWindow::RESA_refresh_kit_list_table();
}

//Function finding every kit with a specific name in from_kit list, and store those kit in to_kits list
void MainWindow::RESA_get_kits_by_name(std::vector<Kit*> *from_kits, std::vector<Kit*> *to_kits, QString code)
{
    for(const auto& kit_elem : *from_kits)
    {
        if (kit_elem->getIs_in_resa_view())
        {
            //do nothing, already in resa view
        }
        else
        {
            if (kit_elem->getNom().contains(code, Qt::CaseInsensitive))
            {
                to_kits->push_back(kit_elem);
                kit_elem->setIs_in_resa_view(true);
            }
        }
    }
}

//Function finding every kit with a specific code in from_kit list, and store those kit in to_kits list
void MainWindow::RESA_get_kits_by_code(std::vector<Kit*> *from_kits, std::vector<Kit*> *to_kits, QString code)
{

    for(const auto& kit_elem : *from_kits)
    {
        if (kit_elem->getIs_in_resa_view())
        {
            //do nothing, already in resa view
        }
        else
        {
            if (kit_elem->getCode().contains(code, Qt::CaseInsensitive))
            {
                to_kits->push_back(kit_elem);
                kit_elem->setIs_in_resa_view(true);
            }
        }
    }
}


void MainWindow::RESA_refresh_kit_list_table(void)
{
    vector<Kit*>::iterator it;
    QBrush brush_booked;
    QBrush brush_free;
    QBrush brush_basket;
    QBrush brush_enpanne;
    // Define brush to display kit free in kit reservation list
    brush_free.setColor(Qt::GlobalColor::blue);
    brush_free.setStyle(Qt::SolidPattern);
    // Define brush to display kit booked in kit reservation list
    brush_booked.setColor(Qt::GlobalColor::red);
    brush_booked.setStyle(Qt::SolidPattern);
    // Define brush to display kit in basket
    brush_basket.setColor(Qt::GlobalColor::yellow);
    brush_basket.setStyle(Qt::SolidPattern);
    // Define brush to display kit in basket
    brush_enpanne.setColor(Qt::GlobalColor::gray);
    brush_enpanne.setStyle(Qt::SolidPattern);
    this->ui->RESA_listWidget_resa->clear();
    for(const auto& kit_elem : this->kitListResa_view)
    {
        QListWidgetItem* p_item = new QListWidgetItem(kit_elem->toString(), this->ui->RESA_listWidget_resa);
        if(kit_elem->getIs_in_basket())
        {
            p_item->setText(p_item->text() + " (déjà dans le panier)");
            p_item->setBackground(brush_basket);
        }
        else if (kit_elem->getIs_booked())
        {
            p_item->setText(p_item->text() + " (déjà réservé à cette date)");
            p_item->setBackground(brush_booked);
        }
        else if (kit_elem->getEn_panne())
        {
            p_item->setText(p_item->text() + " (En panne)");
            p_item->setBackground(brush_enpanne);
        }
        else
        {
            p_item->setForeground(brush_free);
        }
    }
}

void MainWindow::on_RESA_listWidget_resa_itemDoubleClicked(QListWidgetItem *item)
{
    int row = this->ui->RESA_listWidget_resa->row(item);
    Kit* p_kit = this->kitListResa_view.at(row);

    if (p_kit->getIs_in_basket() || p_kit->getEn_panne()) // Do not allow to book a kit if Out of order , or already in basket
    {
        //do nothing
    }
    else
    {
        p_kit->setIs_in_basket(true);
        this->kitListBasket_view.push_back(p_kit);
    }
    // Refresh kits booked status
    g_connect_db.set_kit_booked_status(&this->kitListBasket_view, this->ui->RESA_calendarWidget->selectedDate());

    // Refresh list displays
    RESA_refresh_kit_list_table();
    RESA_refresh_basket_kit_list_table();
}

void MainWindow::RESA_refresh_basket_kit_list_table(void)
{
    vector<Kit*>::iterator it;
    QBrush brush_booked;
    QBrush brush_free;
    this->isBasketReadyToBook = true;

    // Define brush to display kit booked in kit reservation list
    brush_booked.setColor(Qt::GlobalColor::red);
    brush_booked.setStyle(Qt::SolidPattern);
    // Define brush to display kit free in kit reservation list
    brush_free.setColor(Qt::GlobalColor::blue);
    brush_free.setStyle(Qt::SolidPattern);
    this->ui->RESA_listWidget_panierResa->clear();
    for(const auto& kit_elem : this->kitListBasket_view)
    {
        QListWidgetItem* p_item = new QListWidgetItem(kit_elem->toString(), this->ui->RESA_listWidget_panierResa);
        if (kit_elem->getIs_booked())
        {
            p_item->setBackground(brush_booked);
            isBasketReadyToBook = false;
        }
        else
        {
            p_item->setForeground(brush_free);
        }
    }

    //Only activate "Reserver" button when basket is no empty and ready to book.
    if ((isBasketReadyToBook == true) && (this->kitListBasket_view.size() >0))
    {
        ui->RESA_pushButton_reserver->setEnabled(true);
    }
    else if ((this->login_user.getPrivilege() == E_admin) && (this->kitListBasket_view.size() >0))
    {
        ui->RESA_pushButton_reserver->setEnabled(true);
    }
    else
    {
        ui->RESA_pushButton_reserver->setEnabled(false);
    }
}


void MainWindow::on_RESA_listWidget_panierResa_itemDoubleClicked(QListWidgetItem *item)
{
    int row = this->ui->RESA_listWidget_panierResa->row(item);
    Kit* p_kit = this->kitListBasket_view.at(row);
    p_kit->setIs_in_basket(false);

    this->ui->RESA_listWidget_panierResa->removeItemWidget(item);//Delete kit from basket view
    delete(item);
    this->kitListBasket_view.erase(this->kitListBasket_view.begin()+row);//Delete kit from basket list

    RESA_refresh_kit_list_table();
    RESA_refresh_basket_kit_list_table();
}




void MainWindow::on_RESA_pushButton_reserver_clicked()
{
    int resa_nb = 0;
    Utilisateur l_user;
    bool has_errors = false;
    QDate start_date;
    start_date = this->ui->RESA_calendarWidget->selectedDate();

    g_connect_db.start_resa();// Start of LOCK

    // Refresh kits in basket to see if none booked them
    g_connect_db.set_kit_booked_status(&this->kitListBasket_view, this->ui->RESA_calendarWidget->selectedDate());
    RESA_refresh_basket_kit_list_table();

    //Retrieve user id
    has_errors = g_connect_db.get_user_by_utinfo(this->ui->RESA_lineEdit_resa_utinfo_user->text(), &l_user);
    if (has_errors == true)
    {
        this->GEN_raise_popup_warning("Impossible de réserver pour l'utilisateur: **" + this->ui->RESA_lineEdit_resa_utinfo_user->text()+"**");
    }
    else
    {
        // Proceed booking
        if (this->isBasketReadyToBook == true)
        {
            if (this->kitListBasket_view.empty() == false)
            {
                resa_nb = g_connect_db.guess_next_resa_nb();
                g_connect_db.insert_log_by_user_and_kit(nullptr,&l_user,"L'utilisateur '"+l_user.getUtinfo()+"' a effectué la reservation numéro: " + QString::number(resa_nb));
                for(const auto& kit_elem : this->kitListBasket_view)
                {
                    g_connect_db.add_resa_from_kit(kit_elem, l_user.getId(), start_date, resa_nb );
                    g_connect_db.insert_log_by_user_and_kit(kit_elem,&l_user,"-----> Dans la réservation n°" + QString::number(resa_nb) + ", l'utilisateur '"+l_user.getUtinfo()+"' a réservé le kit '"+kit_elem->getNom()+"' (code: "+kit_elem->getCode()+") pour la date: '"+start_date.toString("ddd d MMM yyyy")+"'");
                    kit_elem->setIs_in_basket(false);
                    kit_elem->setIs_booked(true);
                }
                GEN_raise_popup_info("Votre réservation n° **"+QString::number(resa_nb)+"** est bien prise en compte.");
                this->ui->RESA_listWidget_panierResa->clear();
                this->kitListBasket_view.clear();
                RESA_refresh_kit_list_table();
                on_RESA_pushButton_resa_showResa_clicked();
            }
        }
        else
        {
            this->GEN_raise_popup_warning("Impossible de réserver car un Kit n'est pas disponible à la date souhaitée.");
        }
    }
    g_connect_db.end_resa();// End of LOCK

}

void MainWindow::on_RESA_pushButton_reserver_customContextMenuRequested(const QPoint &pos)
{
    if ((this->login_user.getIs_logged_on() == false) || (this->login_user.getPrivilege() != E_admin))
    {
        return;
    }

    QMenu menu(this);
    QAction sortie_immediate_action("Sortir immediatement le panier", this);
    sortie_immediate_action.setEnabled(this->kitListBasket_view.empty() == false);
    connect(&sortie_immediate_action, &QAction::triggered, this, &MainWindow::RESA_sortir_panier_immediatement);
    menu.addAction(&sortie_immediate_action);
    menu.exec(this->ui->RESA_pushButton_reserver->mapToGlobal(pos));
}

void MainWindow::RESA_sortir_panier_immediatement()
{
    int sortie_nb = 0;
    Utilisateur l_user;
    bool has_errors = false;
    QDate start_date = QDate::currentDate();

    if (this->kitListBasket_view.empty())
    {
        this->GEN_raise_popup_warning("Impossible de sortir immediatement un panier vide.");
        return;
    }

    has_errors = g_connect_db.get_user_by_utinfo(this->ui->RESA_lineEdit_resa_utinfo_user->text(), &l_user);
    if (has_errors == true)
    {
        this->GEN_raise_popup_warning("Impossible de sortir immediatement pour l'utilisateur: **" + this->ui->RESA_lineEdit_resa_utinfo_user->text()+"**");
        return;
    }

    g_connect_db.set_kit_out_status(&this->kitListBasket_view);
    for(const auto& kit_elem : this->kitListBasket_view)
    {
        if (kit_elem->getIs_out())
        {
            qInfo() << "GESTION RESA: tentative de sortie immediate sans succes pour le kit"
                    << kit_elem->getNom()
                    << "(code:" << kit_elem->getCode() << "), car il est deja sorti.";
            continue;
        }

        std::vector<Item *> items_to_take_out;
        g_connect_db.select_items_by_kit(kit_elem);
        for(const auto& item_elem : kit_elem->item_list)
        {
            Item *p_item = new Item(item_elem->getId(),
                                    item_elem->getName(),
                                    item_elem->getForkey(),
                                    item_elem->getQuantity_init(),
                                    item_elem->getQuantity_current(),
                                    item_elem->getQuantity_current());
            items_to_take_out.push_back(p_item);
        }
        QString log_str = g_connect_db.update_items_quantity_of_kit(kit_elem, items_to_take_out);

        g_connect_db.start_sortie();
        sortie_nb = g_connect_db.guess_next_sortie_nb();
        g_connect_db.add_sortie_from_kit(kit_elem, l_user.getId(), start_date, sortie_nb);
        g_connect_db.insert_log_by_user_and_kit(kit_elem,&l_user,"L'utilisateur '"+l_user.getUtinfo()+"' a sorti immediatement le kit '"+kit_elem->getNom()+"' (code: "+kit_elem->getCode()+", numero de sortie: "+QString::number(sortie_nb)+") depuis le panier de reservation.");
        g_connect_db.insert_log_by_user_and_kit(kit_elem,&l_user,"-----> Sortie immediate effectuee par l'administrateur '"+this->login_user.getUtinfo()+"' sans verification des quantites d'items.");
        g_connect_db.insert_log_by_user_and_kit(kit_elem,&l_user,log_str);
        g_connect_db.end_sortie();

        kit_elem->setIs_in_basket(false);
        kit_elem->setIs_out(true);
        g_utils.clearList(&items_to_take_out);
    }

    GEN_raise_popup_info("La sortie immediate du panier est bien prise en compte.");
    this->ui->RESA_listWidget_panierResa->clear();
    this->kitListBasket_view.clear();
    this->GESKIT_refresh_kit_list_from_server(&this->kitList);
    this->on_RESA_pushButton_getkit_resa_clicked();
    this->on_RESA_pushButton_resa_showResa_clicked();
}

void MainWindow::on_RESA_pushButton_resa_showResa_clicked()
{
    Utilisateur l_user;
    bool has_errors = false;

    g_utils.clearList(&this->resaList);
    //Retrieve user id
    has_errors = g_connect_db.get_user_by_utinfo(this->ui->RESA_lineEdit_resa_utinfo_user->text(), &l_user);
    if (has_errors != true)
    {
        this->statusBar()->showMessage("GESTION RESA: Réservation pour l'utilisateur: "+this->ui->RESA_lineEdit_resa_utinfo_user->text());

        g_connect_db.select_active_resa_by_user(&this->resaList, l_user.getId());
        RESA_refresh_current_resa_list_table();

    }
    else
    {
        //maybe utinfo is empty: in this case display every resevations
        if (this->ui->RESA_lineEdit_resa_utinfo_user->text() == "")
        {
            this->statusBar()->showMessage("GESTION RESA: toutes les reservations actives");
            g_connect_db.select_all_active_resa(&this->resaList);
            RESA_refresh_current_resa_list_table();
        }
        else
        {
            this->statusBar()->showMessage("GESTION RESA: L'UTINFO fournie est inconnue");
        }
    }
}


void MainWindow::RESA_refresh_current_resa_list_table(void)
{
    int prev_id_resa = 0;
    Utilisateur user;
    this->ui->RESA_listWidget_resa_currentResa->clear();
    for(const auto& resa_elem : this->resaList)
    {
        if (resa_elem->getId_resa() == prev_id_resa)
        {
            //pass
        }
        else
        {
            g_connect_db.get_user_by_id(resa_elem->getId_user(), &user);
            new QListWidgetItem(resa_elem->toString() +" (réservé par "+  user.getUtinfo() +" tel: "+ user.getTelephone()+")", this->ui->RESA_listWidget_resa_currentResa);
            prev_id_resa = resa_elem->getId_resa();
        }
    }
}

void MainWindow::RESA_refresh_user_utinfo_completer()
{
    QStringList utinfoList;

    for (const auto& user_elem : this->userList)
    {
        const QString utinfo = user_elem->getUtinfo();
        if (utinfo.isEmpty() == false && utinfoList.contains(utinfo, Qt::CaseInsensitive) == false)
        {
            utinfoList.append(utinfo);
        }
    }

    utinfoList.sort(Qt::CaseInsensitive);
    this->p_resaUtinfoCompleterModel->setStringList(utinfoList);
}


int MainWindow::RESA_find_resa_nb_selected(QListWidgetItem *item)
{
    int resa_nb = 0;
    QRegularExpression re;
    re.setPattern("(\\d*) -");
    QRegularExpressionMatch match;
    match = re.match(item->text());
    if (match.hasMatch())
    {
        resa_nb = match.captured(1).toInt();
    }
    else
    {
        resa_nb = -1;
    }
    return resa_nb;
}

void MainWindow::on_RESA_listWidget_resa_currentResa_itemClicked(QListWidgetItem *item)
{

    Kit * p_kit;
    int resa_nb = 0;

    //clear kit list widget
    this->ui->RESA_listWidget_resa_kitsOfResa->clear();
    this->ui->RESA_pushButton_suppr_resa->setEnabled(true);

    resa_nb = this->RESA_find_resa_nb_selected(item);

    if (resa_nb != -1)
    {
        //iter through resa list to find every reservation with resa_nb and get their associated kits
        for(const auto& resa_elem : this->resaList)
        {
            if (resa_elem->getId_resa() == resa_nb)
            {
                p_kit = GESKIT_find_kit_by_id(resa_elem->getId_kit());
                new QListWidgetItem(p_kit->toString(), this->ui->RESA_listWidget_resa_kitsOfResa);
            }
        }
    }
}

void MainWindow::on_RESA_pushButton_suppr_resa_clicked()
{

    Utilisateur l_user;
    bool has_errors = false;
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Suppression de réservation.",
                                                               tr("Êtes-vous sûr de vouloir supprimer votre réservation ?"),
                                                               QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                               QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        // event->ignore();


    } else {

        //Retrieve user id
        has_errors = g_connect_db.get_user_by_utinfo(this->ui->RESA_lineEdit_resa_utinfo_user->text(), &l_user);

        //clear kit list widget
        this->ui->RESA_listWidget_resa_kitsOfResa->clear();

        int resa_nb = 0;
        resa_nb = this->RESA_find_resa_nb_selected(this->ui->RESA_listWidget_resa_currentResa->currentItem());
        if (resa_nb != -1)
        {
            g_connect_db.deactivate_resa_from_id(resa_nb);
            this->on_RESA_pushButton_resa_showResa_clicked();

            g_connect_db.insert_log_by_user_and_kit(nullptr,&l_user,"L'utilisateur '"+l_user.getUtinfo()+"' a annulé la reservation portant le numéro: "+QString::number(resa_nb));
        }
    }
}

void MainWindow::RESA_deactivate_outdated_resa(void)
{
    int ret = GEN_raise_popup_ask_to_continue("Êtes-vous sûr de vouloir désactiver toutes les réservations passées?");
    if (ret == QMessageBox::Yes)
    {
        g_connect_db.deactivate_resa_prior_to_date(QDate::currentDate());
        GEN_raise_popup_info("Réservations passées désactivées avec succès.");
    }
    else
    {

    }
}
// ^^^^^^ MAIN WINDOW "Gestion Reservation" ^^^^^^
//---------------------------------------------------------


//---------------------------------------------------------
// vvvvvv MAIN WINDOW "Gestion SORTIES" vvvvvv



void MainWindow::on_SORTIE_lineEdit_utinfo_returnPressed()
{

    on_SORTIE_pushButton_resa_showResa_clicked();
}

void MainWindow::on_SORTIE_pushButton_resa_showResa_clicked()
{
    bool has_errors = false;
    this->sortie_resaPasswordValidated = false;
    this->sortie_resaForcedByAdmin = false;
    this->sortie_lastSelectedResaNb = -1;

    //Delete every kits in "KitsOfResa" view
    this->kitListSortie_kitsOfResaView.clear(); //do not delete pointed kits though
    this->ui->SORTIE_listWidget_resa_kitsOfResa->clear();
    SORTIE_refresh_resa_password_validation_state();

    //Delete every resa in "currentResa" view
    g_utils.clearList(&this->resaListSortie_byUser);
    this->ui->SORTIE_listWidget_resa_currentResa->clear();

    //Delete every kits in "kitsOut" view
    g_utils.clearList(&this->sortieList_byUser);
    this->ui->SORTIE_listWidget_kitsOut->clear();


    // Populates "sortie_user" object with user informations if utinfo is in database
    if (this->ui->SORTIE_lineEdit_utinfo->text() != "")
    {
        has_errors = g_connect_db.get_user_by_utinfo(this->ui->SORTIE_lineEdit_utinfo->text(), &this->sortie_user);
        if (has_errors == false)
        {
            this->statusBar()->showMessage("GESTION SORTIE: Sortie/Retour pour l'utilisateur: "+this->ui->SORTIE_lineEdit_utinfo->text());

            // Updates resa list
            g_connect_db.select_active_resa_by_user(&this->resaListSortie_byUser, this->sortie_user.getId());
            SORTIE_refresh_current_resa_list_table();

            // Updates sortie list
            g_connect_db.select_active_sortie_by_user(&this->sortieList_byUser, this->sortie_user.getId());
            SORTIE_refresh_kitsOut_table();


            this->ui->SORTIE_pushButton_sortir->setEnabled(false);
            this->ui->pushButton_restituerKit->setEnabled(false);
        }
        else
        {
            this->statusBar()->showMessage("GESTION SORTIE: L'UTINFO fournie est inconnue");
        }
    }
    else
    {
        // Updates resa list
        g_connect_db.select_all_active_resa(&this->resaListSortie_byUser);
        SORTIE_refresh_current_resa_list_table();
        g_connect_db.select_all_active_sortie(&this->sortieList_byUser);
        SORTIE_refresh_kitsOut_table();
        this->statusBar()->showMessage("GESTION SORTIE: Sortie/Retour pour tous les utilisateurs");
    }

}

void MainWindow::on_SORTIE_listWidget_resa_currentResa_itemClicked(QListWidgetItem *item)
{
    //Save last selected resa item
    item_lastSelectedResa = item;

    // get resa number from item
    int resa_nb = this->RESA_find_resa_nb_selected(item);
    if (resa_nb != this->sortie_lastSelectedResaNb)
    {
        // this->sortie_resaPasswordValidated = false;
        // this->sortie_resaForcedByAdmin = false;
        this->sortie_lastSelectedResaNb = resa_nb;
        this->ui->SORTIE_lineEdit_resa_mdp->clear();
    }

    SORTIE_refresh_kits_of_resa_table(resa_nb);

}

void MainWindow::SORTIE_refresh_kits_of_resa_table(int i_resa_nb)
{
    Kit * p_kit;
    QBrush brush_out_by_sortie_user;
    QBrush brush_out_by_someone_else;
    QBrush brush_available;

    brush_out_by_sortie_user.setColor(Qt::GlobalColor::green);
    brush_out_by_sortie_user.setStyle(Qt::SolidPattern);

    brush_out_by_someone_else.setColor(Qt::GlobalColor::red);
    brush_out_by_someone_else.setStyle(Qt::SolidPattern);

    brush_available.setColor(Qt::GlobalColor::blue);
    brush_available.setStyle(Qt::SolidPattern);

    //clear kit list widget
    this->ui->SORTIE_listWidget_resa_kitsOfResa->clear();
    this->ui->SORTIE_pushButton_sortir->setEnabled(false); // Disable "sortir" to force user to select a kit
    this->ui->SORTIE_pushButton_retirer_kit_from_resa->setEnabled(false); // Disable "retirer_kit_from_resa" to force user to select a kit
    this->kitListSortie_kitsOfResaView.clear();
    if (i_resa_nb != -1)
    {
        //iter through resa list to find every reservation with resa_nb and get their associated kits
        for(const auto& resa_elem : this->resaListSortie_byUser)
        {
            if (resa_elem->getId_resa() == i_resa_nb)
            {

                p_kit = GESKIT_find_kit_by_id(resa_elem->getId_kit());
                new QListWidgetItem(p_kit->toString(), this->ui->SORTIE_listWidget_resa_kitsOfResa);
                this->kitListSortie_kitsOfResaView.push_back(p_kit);
            }
        }


        //refresh "isOut" and "id_user_out" parameters of kits inside "kitListSortie_kitsOfResaView"
        g_connect_db.set_kit_out_status(&this->kitListSortie_kitsOfResaView);

        //Set background color of every items in "SORTIE_listWidget_resa_kitsOfResa"
        int cnt = 0;
        QListWidgetItem* p_item2;
        for(const auto& kit_elem : this->kitListSortie_kitsOfResaView)
        {
            // get corresponding item of listWidget
            p_item2 = this->ui->SORTIE_listWidget_resa_kitsOfResa->item(cnt);

            // Check if kit is already out
            if (kit_elem->getIs_out() == true)
            {
                //If kit out by "sortie_user" set to green
                if (kit_elem->getId_user_out() == this->sortie_user.getId())
                {
                    p_item2->setBackground(brush_out_by_sortie_user);
                }
                //If kit out by anybody else set to red
                else
                {
                    p_item2->setBackground(brush_out_by_someone_else);
                    //If item is out by another user, add user UTINFO to text
                    Utilisateur user;
                    if (g_connect_db.get_user_by_id(kit_elem->getId_user_out(), &user) == false)
                    {
                        QString text = p_item2->text();
                        p_item2->setText(text + " (sorti par " + user.getPrenom() + " "+ user.getNom()+", téléphone: '"+user.getTelephone()+"')");
                    }
                }
            }
            else
            {
                //If kit is available, set to blue
                p_item2->setForeground(brush_available);
            }
            cnt++;
        }
    }
    SORTIE_refresh_resa_password_validation_state();
}

void MainWindow::SORTIE_refresh_resa_password_validation_state()
{
    bool resa_selected = (this->sortie_lastSelectedResaNb != -1) && (this->kitListSortie_kitsOfResaView.empty() == false);
    bool kit_out_selected = this->ui->SORTIE_listWidget_kitsOut->selectedItems().empty() == false;
    bool user_selected = this->ui->SORTIE_lineEdit_utinfo->text().isEmpty() == false;
    bool enabled = user_selected && (resa_selected || kit_out_selected) && (this->sortie_resaPasswordValidated == false);

    this->ui->SORTIE_groupBox_resa_password->setEnabled(true);
    this->ui->SORTIE_lineEdit_resa_mdp->setEnabled(enabled);
    this->ui->SORTIE_checkBox_resa_mdpadmin->setEnabled(enabled);
    this->ui->SORTIE_pushButton_validate_mdp_sortie->setEnabled(enabled);
    this->ui->SORTIE_label_resa_password_user->setText(this->sortie_user.getEmail());

    if (this->ui->SORTIE_checkBox_resa_mdpadmin->isChecked())
    {
        this->ui->SORTIE_label_resa_password->setText("Mot de passe admin:");
        this->ui->SORTIE_label_resa_password_user->setText(this->login_user.getEmail());
    }
    else
    {
        this->ui->SORTIE_label_resa_password->setText("Mot de passe utilisateur:");
    }

    if (this->sortie_resaPasswordValidated)
    {
        this->ui->SORTIE_label_resa_password_status->setText("Mot de passe valide. Vous pouvez sortir ou restituer les kits.");
        this->ui->SORTIE_label_resa_password_status->setStyleSheet("QLabel { color : green; font-weight: bold; }");
    }
    else if (user_selected == false)
    {
        this->ui->SORTIE_label_resa_password_status->setText("Selectionnez un utilisateur pour valider le mot de passe.");
        this->ui->SORTIE_label_resa_password_status->setStyleSheet("");
    }
    else if (resa_selected)
    {
        this->ui->SORTIE_label_resa_password_status->setText("Mot de passe requis avant de sortir les kits.");
        this->ui->SORTIE_label_resa_password_status->setStyleSheet("QLabel { color : red; font-weight: bold; }");
    }
    else if (kit_out_selected)
    {
        this->ui->SORTIE_label_resa_password_status->setText("Mot de passe requis avant de restituer les kits.");
        this->ui->SORTIE_label_resa_password_status->setStyleSheet("QLabel { color : red; font-weight: bold; }");
    }
    else
    {
        this->ui->SORTIE_label_resa_password_status->setText("Selectionnez une reservation ou un kit sorti pour valider le mot de passe.");
        this->ui->SORTIE_label_resa_password_status->setStyleSheet("");
    }
}

void MainWindow::SORTIE_refresh_kitsOut_table(void)
{
    Kit *p_kit;
    this->ui->SORTIE_listWidget_kitsOut->clear();
    for(const auto& sortie_elem : this->sortieList_byUser)
    {
        p_kit = GESKIT_find_kit_by_id(sortie_elem->getId_kit());
        new QListWidgetItem(QString::number(sortie_elem->getId_sortie())+" - " + p_kit->getNom()+" (code: " +p_kit->getCode() +")" , this->ui->SORTIE_listWidget_kitsOut);

    }
}


///
/// \brief MainWindow::SORTIE_get_kitOfResa_selected
///     Function returning a pointer to the kit selected inside "SORTIE_listWidget_resa_kitsOfResa" widget
///     It also populates the item associated to the kit
/// \return
///
Kit* MainWindow::SORTIE_get_kitOfResa_selected()
{
    //First get kit selected by user
    QList<QListWidgetItem*> items = this->ui->SORTIE_listWidget_resa_kitsOfResa->selectedItems();
    int row = this->ui->SORTIE_listWidget_resa_kitsOfResa->row(items.first());
    Kit* p_kit = kitListSortie_kitsOfResaView.at(row);
    g_connect_db.select_items_by_kit (p_kit); // get every items of this kit
    return p_kit;
}

///
/// \brief MainWindow::SORTIE_get_kitOut_selected
///     Function returning a pointer to the kit selected inside "SORTIE_listWidget_kitsOut" widget
///     It also populates the item associated to the kit
/// \return
///
Kit* MainWindow::SORTIE_get_kitOut_selected()
{
    //First get kit selected by user
    QList<QListWidgetItem*> items = this->ui->SORTIE_listWidget_kitsOut->selectedItems();
    int row = this->ui->SORTIE_listWidget_kitsOut->row(items.first());
    Sortie* p_sortie = sortieList_byUser.at(row);
    Kit * p_kit = GESKIT_find_kit_by_id(p_sortie->getId_kit());
    g_connect_db.select_items_by_kit (p_kit); // get every items of this kit
    return p_kit;
}


void MainWindow::SORTIE_refresh_current_resa_list_table(void)
{
    int prev_id_resa = 0;

    this->ui->SORTIE_listWidget_resa_currentResa->clear();
    for(const auto& resa_elem : this->resaListSortie_byUser)
    {
        if (resa_elem->getId_resa() == prev_id_resa)
        {
            //pass
        }
        else
        {
            new QListWidgetItem(resa_elem->toString(), this->ui->SORTIE_listWidget_resa_currentResa);
            prev_id_resa = resa_elem->getId_resa();
        }
    }
}

void MainWindow::on_SORTIE_listWidget_resa_kitsOfResa_itemClicked(QListWidgetItem *item)
{
    Kit * l_kit = SORTIE_get_kitOfResa_selected();
    //if kit not out, then enable actions

    // Si le kit est sorti par l'utilisateur actuel alors on ne peut plus le sortir ni le retirer de la résa
    if (    (l_kit->getIs_out() == true) && 
            (l_kit->getId_user_out() == this->sortie_user.getId()))
    {
        this->ui->SORTIE_pushButton_sortir->setEnabled(false);
        this->ui->SORTIE_pushButton_retirer_kit_from_resa->setEnabled(false);
    }
    else
    {
        this->ui->SORTIE_pushButton_sortir->setEnabled(this->sortie_resaPasswordValidated);
        this->ui->SORTIE_pushButton_retirer_kit_from_resa->setEnabled(true);
    }
}

void MainWindow::on_SORTIE_checkBox_resa_mdpadmin_clicked(bool checked)
{
    Q_UNUSED(checked);
    SORTIE_refresh_resa_password_validation_state();
}

void MainWindow::on_SORTIE_pushButton_validate_mdp_sortie_clicked()
{
    bool resa_selected = (this->sortie_lastSelectedResaNb != -1) && (this->kitListSortie_kitsOfResaView.empty() == false);
    bool kit_out_selected = this->ui->SORTIE_listWidget_kitsOut->selectedItems().empty() == false;

    if (this->ui->SORTIE_lineEdit_utinfo->text().isEmpty())
    {
        GEN_raise_popup_warning("Selectionnez un utilisateur avant de verifier le mot de passe.");
        SORTIE_refresh_resa_password_validation_state();
        return;
    }

    if ((resa_selected == false) && (kit_out_selected == false))
    {
        GEN_raise_popup_warning("Selectionnez une reservation ou un kit sorti avant de verifier le mot de passe.");
        SORTIE_refresh_resa_password_validation_state();
        return;
    }

    bool forced_by_admin = this->ui->SORTIE_checkBox_resa_mdpadmin->isChecked();
    Utilisateur * user_to_check = forced_by_admin ? &this->login_user : &this->sortie_user;
    QString mdp_user_sha1 = g_connect_db.get_sha1_from_Qstring(this->ui->SORTIE_lineEdit_resa_mdp->text());
    if (mdp_user_sha1 != user_to_check->getMdp())
    {
        GEN_raise_popup_warning("Erreur dans le mot de passe.");
        return;
    }

    this->sortie_resaPasswordValidated = true;
    this->sortie_resaForcedByAdmin = forced_by_admin;
    this->ui->SORTIE_lineEdit_resa_mdp->clear();
    SORTIE_refresh_resa_password_validation_state();
    if (this->ui->SORTIE_listWidget_resa_kitsOfResa->selectedItems().empty() == false)
    {
        on_SORTIE_listWidget_resa_kitsOfResa_itemClicked(this->ui->SORTIE_listWidget_resa_kitsOfResa->currentItem());
    }
    if (this->ui->SORTIE_listWidget_kitsOut->selectedItems().empty() == false)
    {
        this->ui->pushButton_restituerKit->setEnabled(true);
    }
    GEN_raise_popup_info("Le mot de passe est validé");
}

///
/// \brief MainWindow::on_SORTIE_pushButton_sortir_clicked: Function called when "Sortir" Button is called inside main window
///
void MainWindow::on_SORTIE_pushButton_sortir_clicked()
{
    if (this->sortie_resaPasswordValidated == false)
    {
        GEN_raise_popup_warning("Veuillez valider le mot de passe de la reservation avant de sortir un kit.");
        return;
    }

    Kit * l_kit = SORTIE_get_kitOfResa_selected();
    this->p_popupSortirResa = new (PopupSortirResa);
    this->p_popupSortirResa->setMode(E_MODE_SORTIE);
    this->p_popupSortirResa->setUser(&this->sortie_user);
    this->p_popupSortirResa->setP_kit(l_kit);
    this->p_popupSortirResa->refresh_source_item_list();
    this->p_popupSortirResa->setWindowTitle(l_kit->getNom());
    this->p_popupSortirResa->setButtonText("Sortir");
    this->p_popupSortirResa->show();
    QObject::connect(this->p_popupSortirResa->getSortirButton(), &QPushButton::clicked, this, &MainWindow::on_SORTIE_popupSortirResaPushSortir);
    QObject::connect(this->p_popupSortirResa->getAnnulerButton(), &QPushButton::clicked, this, &MainWindow::on_SORTIE_popupSortirResaPushAnnuler);
    QObject::connect(this->p_popupSortirResa, &PopupSortirResa::delete_popup, this, &MainWindow::on_SORTIE_popupSortirResaPushAnnuler);
}



///
/// \brief MainWindow::on_SORTIE_popupSortirResaPushSortir : Callback called when "Sortir" button is pushed inside popup
/////
void MainWindow::on_SORTIE_popupSortirResaPushSortir()
{
    bool forced_by_admin_from_popup;
    bool forced_by_admin = this->sortie_resaForcedByAdmin;
    QString optional_text = "";
    if (this->p_popupSortirResa->checkIfOk(&forced_by_admin_from_popup, &optional_text) == true)
    {
        SORTIE_sortir_kit(&forced_by_admin, &optional_text);
        delete(this->p_popupSortirResa);


        // Updates sortie list
        g_utils.clearList(&this->sortieList_byUser);
        g_connect_db.select_active_sortie_by_user(&this->sortieList_byUser, this->sortie_user.getId());

        /* refresh the kit list by cleaning and loading it again */
        this->GESKIT_refresh_kit_list_from_server(&this->kitList);

        SORTIE_refresh_kitsOut_table();

        // select RESA and last selected kit by user in GUI
        this->ui->SORTIE_listWidget_resa_currentResa->setCurrentItem(this->item_lastSelectedResa);
        on_SORTIE_listWidget_resa_currentResa_itemClicked(this->item_lastSelectedResa);
    }
}

void MainWindow::SORTIE_sortir_kit(bool * i_forced_by_admin, QString *i_optional_text)
{
    int sortie_nb = 0;
    Utilisateur l_user;
    bool has_errors = false;
    QDate start_date;
    Kit * p_kit = this->p_popupSortirResa->getP_kit();
    has_errors = g_connect_db.get_user_by_utinfo(this->ui->SORTIE_lineEdit_utinfo->text(), &l_user);
    if (has_errors != true)
    {
        start_date = QDate::currentDate();

        QString log_str = g_connect_db.update_items_quantity_of_kit (p_kit, this->p_popupSortirResa->item_list_dest);

        // Start of LOCK
        g_connect_db.start_sortie();
        sortie_nb = g_connect_db.guess_next_sortie_nb();
        g_connect_db.add_sortie_from_kit(p_kit, l_user.getId(), start_date, sortie_nb);
        g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,"L'utilisateur '"+l_user.getUtinfo()+"' a sorti le kit '"+p_kit->getNom()+"' (code: "+p_kit->getCode()+", n° de sortie: "+QString::number(sortie_nb)+")");
        if (*i_forced_by_admin)
        {
            g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,"-----> La sortie du kit a été signée par l'administrateur '"+this->login_user.getUtinfo()+"' (en l'absence de "+l_user.getUtinfo()+").");
        }
        if (i_optional_text->isEmpty() !=  true) // if an optionnal string has been set by user
        {
            g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,"-----> Message optionnel lié à la sortie: " + *i_optional_text);
        }
        g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,log_str);
        // End of LOCK
        g_connect_db.end_sortie();

        p_kit->setIs_out(true);
        GEN_raise_popup_info("Votre Sortie n° **"+QString::number(sortie_nb)+"** est bien prise en compte.");

    }

}



void MainWindow::on_SORTIE_listWidget_kitsOut_itemClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    this->ui->pushButton_restituerKit->setEnabled(this->sortie_resaPasswordValidated);
    SORTIE_refresh_resa_password_validation_state();
}

void MainWindow::on_SORTIE_listWidget_kitsOut_itemDoubleClicked(QListWidgetItem *item)
{
    on_pushButton_restituerKit_clicked();
}

///
/// \brief MainWindow::on_pushButton_restituerKit_clicked: Function called when "Restituer" Button is called inside main window
///
void MainWindow::on_pushButton_restituerKit_clicked()
{
    if (this->sortie_resaPasswordValidated == false)
    {
        GEN_raise_popup_warning("Veuillez valider le mot de passe avant de restituer un kit.");
        return;
    }

    if (this->ui->SORTIE_listWidget_kitsOut->selectedItems().empty())
    {
        GEN_raise_popup_warning("Selectionnez un kit sorti avant de le restituer.");
        return;
    }

    Kit * l_kit = SORTIE_get_kitOut_selected();
    this->p_popupSortirResa = new (PopupSortirResa);
    this->p_popupSortirResa->setMode(E_MODE_RESTITUTION);
    this->p_popupSortirResa->setUser(&this->sortie_user);
    this->p_popupSortirResa->setP_kit(l_kit);
    this->p_popupSortirResa->refresh_source_item_list();
    this->p_popupSortirResa->setWindowTitle(l_kit->getNom());
    this->p_popupSortirResa->setButtonText("Restituer");
    this->p_popupSortirResa->show();
    QObject::connect(this->p_popupSortirResa->getSortirButton(), &QPushButton::clicked, this, &MainWindow::on_SORTIE_popupSortirResaPushRestituer);
    QObject::connect(this->p_popupSortirResa->getAnnulerButton(), &QPushButton::clicked, this, &MainWindow::on_SORTIE_popupSortirResaPushAnnuler);
    QObject::connect(this->p_popupSortirResa, &PopupSortirResa::delete_popup, this, &MainWindow::on_SORTIE_popupSortirResaPushAnnuler);
}

///
/// \brief MainWindow::on_SORTIE_popupSortirResaPushRestituer : Callback called when "Restituer" button is pushed inside popup
/////
void MainWindow::on_SORTIE_popupSortirResaPushRestituer()
{
    bool forced_by_admin_from_popup;
    bool forced_by_admin = this->sortie_resaForcedByAdmin;
    QString optional_text = "";
    if (this->p_popupSortirResa->checkIfOk(&forced_by_admin_from_popup, &optional_text) == true)
    {
        SORTIE_restit_kit(&forced_by_admin, &optional_text);
        delete(this->p_popupSortirResa);


        /* refresh the kit list by cleaning and loading it again */
        this->GESKIT_refresh_kit_list_from_server(&this->kitList);

        // Updates sortie list
        g_utils.clearList(&this->sortieList_byUser);
        g_connect_db.select_active_sortie_by_user(&this->sortieList_byUser, this->sortie_user.getId());
        SORTIE_refresh_kitsOut_table();
        this->ui->pushButton_restituerKit->setEnabled(false);
        SORTIE_refresh_resa_password_validation_state();
    }
}


void MainWindow::SORTIE_restit_kit(bool * i_forced_by_admin, QString *i_optional_text)
{
    Kit * p_kit = this->p_popupSortirResa->getP_kit();
    Utilisateur l_user;
    g_connect_db.get_user_by_utinfo(this->ui->SORTIE_lineEdit_utinfo->text(), &l_user);

    int sortie_number = g_connect_db.select_sortie_nb_from_kit(p_kit); // get sortie_nb for logs before deactivating sortie
    g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,"L'utilisateur '"+l_user.getUtinfo()+"' a restitué le kit '"+p_kit->getNom()+"' (code: "+p_kit->getCode()+", n° de sortie: "+QString::number(sortie_number)+")");
    if (*i_forced_by_admin)
    {
        g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,"-----> La restitution du kit a été signée par l'administrateur '"+this->login_user.getUtinfo()+"' (en l'absence de "+l_user.getUtinfo()+").");
    }
    if (i_optional_text->isEmpty() !=  true)
    {
        g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,"-----> Message optionnel lié à la restitution: " + *i_optional_text);
    }
    //Calculate remaining item quantity based on quantity returned and save it to db
    SORTIE_calculate_remaining_quantity(this->p_popupSortirResa->item_list_dest, p_kit->item_list );
    QString log_str = g_connect_db.update_items_quantity_of_kit (p_kit, this->p_popupSortirResa->item_list_dest);
    g_connect_db.insert_log_by_user_and_kit(p_kit,&l_user,log_str);

    //Supprimer la sortie dans la table de reservation
    g_connect_db.delete_sortie_from_kit(p_kit);
    p_kit->setIs_out(false);
    GEN_raise_popup_info("Vous avez restitué le kit : "+ p_kit->getNom());

}


///
/// \brief MainWindow::SORTIE_calculate_remaining_quantity
/// Calculate the remaining current quantity (param "quantity_current) of every items i_items_returned, based on the quantity of each item returned compared to the quantity of each item taken out.
/// \param i_items_returned
/// \param i_items_kit
///
void MainWindow::SORTIE_calculate_remaining_quantity(std::vector<Item *> i_items_returned, std::vector<Item *> i_items_kit )
{
    uint current_quantity = 0u;
    uint new_current_quantity = 0u;
    for(const auto& elem_returned_item : i_items_returned)
    {
        for(const auto& elem_kit_item : i_items_kit)
        {
            if (elem_returned_item->getId() == elem_kit_item->getId())
            {
                current_quantity = elem_kit_item->getQuantity_current();//save current quantity of item
                if (elem_returned_item->getQuantity_out() < elem_kit_item->getQuantity_out())
                {
                    new_current_quantity = current_quantity - (elem_kit_item->getQuantity_out() - elem_returned_item->getQuantity_out() );
                    elem_returned_item->setQuantity_current(new_current_quantity );
                }
                elem_returned_item->setQuantity_out(0u); //since this function is always called when KIT is returned, then no item should be out

            }
        }
    }
}


///
/// \brief MainWindow::on_SORTIE_popupSortirResaPushAnnuler: : Callback called when "Annuler" button is pushed inside popup
///
void MainWindow::on_SORTIE_popupSortirResaPushAnnuler()
{
    delete this->p_popupSortirResa;
}


void MainWindow::on_SORTIE_pushButton_retirer_kit_from_resa_clicked()
{
    int ret = GEN_raise_popup_ask_to_continue("Voulez-vous vraiment supprimer le Kit de la réservation?");
    if (ret == QMessageBox::Yes)
    {
        Kit * l_kit = SORTIE_get_kitOfResa_selected();
        g_connect_db.deactivate_resa_from_kit(l_kit);

        //Refresh resa list from DB
        on_SORTIE_pushButton_resa_showResa_clicked();
    }
    else
    {

    }
}




void MainWindow::on_SORTIE_pushButton_endResa_clicked()
{

}
// ^^^^^^ MAIN WINDOW "Gestion SORTIES" ^^^^^^
//---------------------------------------------------------













