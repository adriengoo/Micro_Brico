#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "./popups/popupaddusers.h"
#include "./popups/login_connect.h"
#include "./popups/delete_user.h"
#include "./popups/poppupaddkit.h"
#include "./popups/popupsortirresa.h"
#include "./logger/log_user_kit_appli.h"
#include <list>

#include "resa.h"
#include "sortie.h"
#include <QListWidgetItem>
#include <QDate>
#include <QLabel>
#include <QPoint>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void clean_HMI(void);
    void update_connection_status(bool is_user_logged);

public slots:
    void log_stuffs(QtMsgType type, const QString &msg);

private slots:
    /*Add user popup slots */
    //-----------Actions -------------------
    // actions générales
    void on_actionNouvel_Utilisateur_triggered();
    void on_actionSe_connecter_triggered();
    void on_actionSe_d_connecter_triggered();
    void on_actionEffacer_les_reservations_pass_es_triggered();
    void on_actionAfficher_les_logs_appli_triggered();
    // actions pour GESKIT
    void on_actionafficher_les_logs_kit_triggered();
    void on_actiondupliquer_kit_triggered();
    void on_actionmodifier_kit_triggered();
    void on_actionajouter_nouveau_kit_triggered();
    // actions pour GESUSER
    void on_actionAfficher_logs_utilisateur_triggered();
    void on_actionAjouter_utilisateur_triggered();
    void on_actionModifier_l_utilisateur_triggered();

    //-----------Slots GESUSER-------------------
    void on_GESUSER_pushButton_getuser_clicked();
    void on_GESUSER_pushButton_add_user_clicked();
    void on_GESUSER_pushbutton_deleteUser_clicked();
    void on_GESUSER_pushButton_modify_user_clicked();
    void on_GESUSER_tableWidget_user_cellClicked(int row, int column);
    void on_GESUSER_tableWidget_user_cellDoubleClicked(int row, int column);

    //-----------Slots GESKIT-------------------
    void on_GESKIT_pushButton_addkit_clicked();
    void on_GESKIT_pushButton_duplicate_kit_clicked();
    void on_GESKIT_pushButton_modify_kit_clicked();
    void on_GESKIT_pushButton_getkit_clicked();
    void on_GESKIT_lineEdit_findkitbyname_returnPressed();
    void on_GESKIT_lineEdit_findkitbycode_returnPressed();
    void on_GESKIT_pushButton_logkit_clicked();
    void on_GESKIT_tableWidget_kit_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_GESKIT_tableWidget_kit_cellDoubleClicked(int row, int column);

    //-----------Slots RESA-------------------
    void on_RESA_lineEdit_findkitbyname_returnPressed();
    void on_RESA_lineEdit_findkitbycode_returnPressed();
    void on_RESA_pushButton_getkit_resa_clicked();
    void on_RESA_listWidget_resa_itemDoubleClicked(QListWidgetItem *item);
    void on_RESA_listWidget_panierResa_itemDoubleClicked(QListWidgetItem *item);
    void on_RESA_calendarWidget_clicked(const QDate &date);
    void on_RESA_pushButton_resa_showResa_clicked();
    void on_RESA_listWidget_resa_currentResa_itemClicked(QListWidgetItem *item);
    void on_RESA_pushButton_suppr_resa_clicked();
    void on_RESA_pushButton_reserver_clicked();
    void on_RESA_pushButton_reserver_customContextMenuRequested(const QPoint &pos);
    void on_RESA_lineEdit_resa_utinfo_user_returnPressed();

    //-----------Slots SORTIES-------------------
    void on_SORTIE_pushButton_resa_showResa_clicked();
    void on_SORTIE_listWidget_resa_currentResa_itemClicked(QListWidgetItem *item);
    void on_SORTIE_pushButton_sortir_clicked(); // Function called when "Sortir" Button is called inside main window
    void on_SORTIE_listWidget_resa_kitsOfResa_itemClicked(QListWidgetItem *item);
    void on_SORTIE_popupSortirResaPushSortir();//Callback called when "Sortir" button is pushed inside popup
    void on_SORTIE_popupSortirResaPushAnnuler();
    void on_SORTIE_popupSortirResaPushRestituer();//Callback called when "Restituer" button is pushed inside popup
    void on_pushButton_restituerKit_clicked();
    void on_SORTIE_listWidget_kitsOut_itemClicked(QListWidgetItem *item);
    void on_SORTIE_listWidget_kitsOut_itemDoubleClicked(QListWidgetItem *item);
    void on_SORTIE_pushButton_retirer_kit_from_resa_clicked();
    void on_SORTIE_pushButton_endResa_clicked();
    void on_SORTIE_lineEdit_utinfo_returnPressed();
    void on_SORTIE_checkBox_resa_mdpadmin_clicked(bool checked);
    void on_SORTIE_pushButton_validate_mdp_sortie_clicked();

private:
    Ui::MainWindow *ui;
    std::vector<Utilisateur*> userList;
    std::vector<Utilisateur*> userListView;
    std::vector<Kit*> kitList; //List of kits as a mirror of Kits in DB
    std::vector<Kit*> kitListGeskit_view; //List of kits to be displayed in "GESKIT_tableWidget_kit"
    std::vector<Kit*> kitListResa_view; //List of kits to be displayed in "RESA_listWidget_resa"
    std::vector<Kit*> kitListBasket_view; //List of kits to be displayed in "RESA_listWidget_panierResa"
    std::vector<Kit*> kitListSortie_kitsOfResaView; //List of kits to be displayed in "SORTIE_listWidget_resa_kitsOfResa"
    std::vector<Kit*> kitListSortie_kitsOutView; //List of kits to be displayed in "SORTIE_listWidget_kitsOut"
    std::vector<Resa*> resaList; //List of reservation only for "RESA" view
    std::vector<Resa*> resaListSortie_byUser; //List of reservation only for "SORTIE" view
    std::vector<Sortie*> sortieList_byUser; //List of Sortie associated to the "sortie_user" (only for "SORTIE" view)

    Utilisateur login_user;
    Utilisateur sortie_user; //User that takes a kit out
    popupAddUsers *p_popupAddUser;
    Login_connect *p_loginConnect;
    PopupSortirResa *p_popupSortirResa;
    Delete_user *p_popupDeleteUser;
    PoppupAddKit *p_popupAddKit;
    log_user_kit_appli *p_log_user_kit_appli;
    bool isBasketReadyToBook; //Variable remembering if the basket contains an already booked kit
    bool GESKIT_show_broken_kits;
    QListWidgetItem * item_lastSelectedResa;//Variable remembering the last selected resa (only for "SORTIE" view)
    bool sortie_resaPasswordValidated;
    bool sortie_resaForcedByAdmin;
    int sortie_lastSelectedResaNb;
    QLabel * p_status_bar_label;

    void GEN_raise_popup_info(QString msg);
    void GEN_raise_popup_warning(QString msg);
    int GEN_raise_popup_ask_to_continue(QString msg);
    void closeEvent (QCloseEvent *event);
    void activateWidgets(void);

    //-----------Context menu-------------------
    void contextMenuEvent(QContextMenuEvent *event);

    //-----------popups-------------------
    void on_popupLogin_destroyed();
    void on_popupLogin_ok();
    void on_popupDelete_ok();
    void on_popupDelete_destroyed();
    void on_popupaddUser_destroyed();
    void on_popupaddUser_ok();
    void on_popupAddKit_destroyed();
    void on_popupAddKit_ok();

    //-----------User-------------------
    void GESUSER_get_user_by_name(std::vector<Utilisateur *> *from_user, std::vector<Utilisateur *> *to_user, QString name);
    void GESUSER_get_user_by_utinfo(std::vector<Utilisateur *> *from_user, std::vector<Utilisateur *> *to_user, QString utinfo);
    void GESUSER_add_new_user();
    void GESUSER_edit_user();
    void GESUSER_clear_display();
    void GESUSER_enable_GESUSER_buttons(bool i_enable);
    Utilisateur* GESUSER_get_user_selected();
    void GESUSER_refresh_user_list_from_server(std::vector<Utilisateur*> *i_list);
    void GESUSER_refresh_user_table();
    void GESUSER_push_back_new_user_on_table(Utilisateur *user, int row);
    void GESUSER_display_user_logs();

    //-----------Kit-------------------
    void GESKIT_get_kits_by_name(std::vector<Kit *> *from_kits, std::vector<Kit *> *to_kits, QString code);
    void GESKIT_get_kits_by_code(std::vector<Kit *> *from_kits, std::vector<Kit *> *to_kits, QString code);
    void GESKIT_clear_display();
    void GESKIT_enable_geskit_buttons(bool i_enable);
    Kit* GESKIT_get_kit_selected();
    void GESKIT_refresh_kit_list_from_server(std::vector<Kit*> *i_list);
    void GESKIT_refresh_kit_table();
    void GESKIT_push_back_new_kit_on_table(Kit *kit, int row);
    void GESKIT_refresh_item_list_table(Kit *kit);
    void GESKIT_refresh_descritption(Kit *kit);
    void GESKIT_push_back_new_item_on_table(Item *item, int row);
    Kit* GESKIT_find_kit_by_id(int id);
    void GESKIT_display_kit_logs();
    void GESKIT_duplicate_kit();
    void GESKIT_modify_kit();
    void GESKIT_add_kit();

    //-----------Resa-------------------
    void RESA_refresh_kit_list_table();
    void RESA_refresh_basket_kit_list_table();
    void RESA_get_kits_by_code(std::vector<Kit *> *from_kits, std::vector<Kit *> *to_kits, QString code);
    void RESA_get_kits_by_name(std::vector<Kit *> *from_kits, std::vector<Kit *> *to_kits, QString code);
    QDate RESA_get_next_resa_day(QDate start_date);
    void RESA_refresh_current_resa_list_table();
    int RESA_find_resa_nb_selected(QListWidgetItem *item);
    void RESA_deactivate_outdated_resa();
    void RESA_sortir_panier_immediatement();

    //-----------Sorties-------------------
    Kit *SORTIE_get_kitOfResa_selected();
    Kit *SORTIE_get_kitOut_selected();
    void SORTIE_sortir_kit(bool * i_forced_by_admin, QString *i_optional_text);
    void SORTIE_refresh_current_resa_list_table();
    void SORTIE_refresh_kits_of_resa_table(int i_resa_nb);
    void SORTIE_refresh_kitsOut_table();
    void SORTIE_restit_kit(bool * i_forced_by_admin, QString *i_optional_text);
    void SORTIE_calculate_remaining_quantity(std::vector<Item *> i_items_returned, std::vector<Item *> i_items_kit );
    void SORTIE_refresh_resa_password_validation_state();

};
#endif // MAINWINDOW_H
