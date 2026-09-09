#ifndef CONNECT_DB_H
#define CONNECT_DB_H

#include <Qsql>
#include <QSqlDatabase>
#include <QObject>
#include "utilisateur.h"
#include "kit.h"
#include "item.h"
#include "resa.h"
#include "sortie.h"
#include "utils.h"

#include <list>
#include <iterator>


class Connect_db : public QObject
{
    Q_OBJECT

private:
    Utilisateur * active_user;

    //-----------General----------------
    bool get_querry_errors(QSqlQuery &query);
    bool runQuery(QSqlQuery &query, QString query_string);
    //-----------User-------------------
    void update_user_infos_from_db(Utilisateur *login_user);
    bool is_user_connected(void);
    //-----------Kit--------------------
    void populate_kit_list_from_query(std::vector<Kit *> *kits, QSqlQuery query);
    //-----------Resa-------------------
    void populate_resa_list_from_query(std::vector<Resa *> *i_resa, QSqlQuery query);
    //-----------sortie-------------------
    void populate_sortie_list_from_query(std::vector<Sortie *> *i_sortie, QSqlQuery query);

public:
    //-----------General----------------
    QSqlDatabase db;
    Connect_db();

    void insert_log_by_user_and_kit(Kit * i_kit, Utilisateur *i_user, QString text);

    void set_db(const QString &connectionName);

    //-----------User-------------------
    void select_all_users(std::vector<Utilisateur*> *list);
    bool add_user (Utilisateur *user);
    bool update_user(Utilisateur *i_user);
    void setActiveUser(Utilisateur * p_user);
    bool delete_user(Utilisateur *user_to_delete);
    bool connect_user();
    QString get_sha1_from_Qstring(QString mdp);
    QString get_unique_token(void);
    void update_user_token_on_db(void);
    bool get_user_by_mail(QString i_mail, Utilisateur * o_user);
    bool get_user_by_utinfo(QString i_utinfo, Utilisateur *o_user);
    bool get_user_by_id(int id, Utilisateur *o_user);
    void select_logs_by_user(std::vector<Log *> *o_log, Utilisateur *i_user, int i_max_lines);
    bool get_user_who_has_kit_out(Kit * i_kit, Utilisateur * o_user);

    //-----------Kit-------------------
    void select_kits_by_code(std::vector<Kit *> *kits, QString code);
    void select_kits_by_name(std::vector<Kit *> *kits, QString code);
    void add_kit (Kit *);
    void update_kit (Kit *);
    QString update_items_quantity_of_kit (Kit *, std::vector<Item*> i_new_items);
    void add_item(Item *p_item, Kit *p_kit);
    void select_all_kits(std::vector<Kit*> *kits);
    void select_items_by_kit (Kit * );
    void populate_item_list_from_query(Kit * kit, QSqlQuery query);
    void set_kit_booked_status(std::vector<Kit*> *i_kits, QDate i_date);
    void select_logs_by_kit(std::vector<Log *> * o_log, Kit * i_kit, int i_max_lines);

    //-----------Resa-------------------
    void select_all_active_resa(std::vector<Resa *> *i_resa);
    void select_active_resa_by_user(std::vector<Resa *> *o_resa, uint user_id);
    uint32_t guess_next_resa_nb(void);
    void add_resa_from_kit(Kit *i_p_kit, uint user_id, QDate i_start_date, int i_resa_nb);
    void start_resa();
    void end_resa();
    void deactivate_resa_from_id(int i_resa_nb);
    void deactivate_resa_from_kit(Kit *i_kit);
    void deactivate_resa_prior_to_date(QDate i_current_date);

    //-----------Sortie-------------------
    void set_kit_out_status(std::vector<Kit *> *i_kits);
    void end_sortie();
    void start_sortie();
    uint32_t guess_next_sortie_nb();
    void add_sortie_from_kit(Kit *i_p_kit, uint user_id, QDate i_start_date, int i_sortie_nb);
    void select_all_active_sortie(std::vector<Sortie *> *i_sortie);
    void select_active_sortie_by_user(std::vector<Sortie *> *o_sortie, uint user_id);
    void delete_sortie_from_kit(Kit *i_p_kit);
    int select_sortie_nb_from_kit(Kit *i_p_kit);

public slots:
signals:
    void log_value_changed(bool is_user_logged);
};

#endif // CONNECT_DB_H
