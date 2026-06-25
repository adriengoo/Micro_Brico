#ifndef KIT_H
#define KIT_H

#include <QObject>
#include <QString>
#include <QDate>
#include "money.h"
#include "item.h"
#include <vector>
class Kit : public QObject
{
    Q_OBJECT
private:
    int idkit;
    QString nom;
    QString description;
    QDate date_achat;
    Money prix_achat;
    QString texte_libre;
    bool en_panne;
    QString code;
    Money caution;

    //GES_KIT related variables
    bool to_duplicate;

    //RESA related variables
    bool is_booked;
    bool is_in_basket;
    bool is_in_resa_view;
    int id_user_booked;

    //SORTIE related variables
    bool is_out;
    int id_user_out;

public:
    explicit Kit(QObject *parent = nullptr);
    Kit(int idkit, const QString &nom, const QString &description, const QString &date_achat, const QString &prix_achat, const QString &texte_libre, const bool en_panne, const QString &code, const QString &caution);

    std::vector<Item*> item_list;

    QString getNom() const;
    QString getDescription() const;
    QDate getDate_achat() const;
    Money getPrix_achat() const;
    QString getTexte_libre() const;
    bool getEn_panne() const;
    QString getCode() const;
    Money getCaution() const;
    int getIdKit() const;

    void setIdkit(int newIdkit);
    void setNom(const QString &newNom);
    void setDescription(const QString &newDescription);
    void setDate_achat(const QDate &newDate_achat);
    void setPrix_achat(const Money &newPrix_achat);
    void setTexte_libre(const QString &newTexte_libre);
    void setCaution(const Money &newCaution);
    void setEn_panne(bool newEn_panne);
    void setCode(const QString &newCode);
    void setDate_achat(const QString &newDate_achat);
    void setPrix_achat(const QString &newPrix_achat);
    void setCaution(const QString &newCaution);
    QString getEn_panne_str() const;
    ~Kit();
    QString toString();
    bool getIs_booked() const;
    void setIs_booked(bool newIs_booked);

    bool getIs_out() const;
    void setIs_out(bool newIs_out);
    QString getIs_out_str() const;
    bool getIs_in_basket() const;
    void setIs_in_basket(bool newIs_in_basket);

    bool getIs_in_resa_view() const;
    void setIs_in_resa_view(bool newIs_in_resa_view);

    void setIs_already_in_DB(bool newIs_already_in_DB);
    bool getIs_already_in_DB() const;

    bool getTo_duplicate() const;

    void setTo_duplicate(bool newTo_duplicate);

    int getId_user_out() const;
    void setId_user_out(int newId_user_out);
    
    int getId_user_booked() const;
    void setId_user_booked(int newId_user_booked);

signals:
};

#endif // KIT_H
