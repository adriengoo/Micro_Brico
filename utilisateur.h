#ifndef UTILISATEUR_H
#define UTILISATEUR_H
#include <qstring.h>
#include "types.h"
#include "QDate"
#include "money.h"

class Utilisateur
{
public:
    Utilisateur();
    Utilisateur(QString nom, QString mdp, QString prenom, QString email, QString utinfo, QString telephone, QDate date_caution, bool adhesion_payed, Money caution);
    QString ToString();
    void setUserData( QString nom, QString prenom, QString email, QString utinfo, T_user_privilege newPrivilege, QString telephone, QDate date_caution, bool adhesion_payed, Money caution);
    QString getNom(void);
    QString getMdp(void);
    QString getPrenom() const;
    QString getEmail() const;
    QString getToken() const;
    QString getUtinfo() const;
    QString getTelephone() const;
    QDate getDate_caution() const;
    Money getCaution() const;
    T_modify_or_new getCreate_type() const;

    bool getAdhesion_payed() const;
    void setId(int newId);
    void setMdp(const QString &newMdp);
    void setEmail(const QString &newEmail);
    void setUtinfo(const QString &newUtinfo);
    T_user_privilege getPrivilege() const;
    void setPrivilege(const T_user_privilege &newPrivilege);
    bool getIs_logged_on() const;
    void setIs_logged_on(bool newIs_logged_on);
    void setPrenom(const QString &newPrenom);
    void setNom(const QString &newNom);
    void setToken(const QString &newToken);
    int getId() const;
    void setCreate_type(T_modify_or_new newCreate_type);
    void setTelephone(const QString &newTelephone);
    void setDate_caution(const QDate &newDate_caution);
    void setAdhesion_payed(bool newAdhesion_payed);
    void setCaution(const Money &newCaution);




private:
    int id;
    T_modify_or_new create_type;
    QString nom;
    QString mdp;
    QString prenom;
    QString email;
    QString token;
    QString utinfo;
    QString telephone;

    QDate date_caution;
    bool adhesion_payed = false;
    Money caution;

    T_user_privilege privilege = E_basic;
    bool is_logged_on;
};

#endif // UTILISATEUR_H
