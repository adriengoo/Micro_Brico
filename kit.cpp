#include "kit.h"

QString Kit::getNom() const
{
    return nom;
}

QString Kit::getDescription() const
{
    return description;
}

QDate Kit::getDate_achat() const
{
    return date_achat;
}

Money Kit::getPrix_achat() const
{
    return prix_achat;
}

QString Kit::getTexte_libre() const
{
    return texte_libre;
}

bool Kit::getEn_panne() const
{
    return en_panne;
}

QString Kit::getEn_panne_str() const
{
    if (en_panne == true)
    {
        return "Oui";
    }
    else
    {
        return "Non";
    }
}
QString Kit::getCode() const
{
    return code;
}

Money Kit::getCaution() const
{
    return caution;
}

int Kit::getIdKit() const
{
    return idkit;
}

void Kit::setIdkit(int newIdkit)
{
    idkit = newIdkit;
}

void Kit::setNom(const QString &newNom)
{
    nom = newNom;
}

void Kit::setDescription(const QString &newDescription)
{
    description = newDescription;
}

void Kit::setDate_achat(const QDate &newDate_achat)
{
    date_achat = newDate_achat;
}

void Kit::setDate_achat(const QString &newDate_achat)
{
    QStringList list1 = newDate_achat.split("-");
    date_achat.setDate(list1.at(0).toInt(), list1.at(1).toInt(),list1.at(2).toInt()) ;
}

void Kit::setPrix_achat(const Money &newPrix_achat)
{
    prix_achat = newPrix_achat;
}

void Kit::setPrix_achat(const QString &newPrix_achat)
{
    prix_achat.setValue(newPrix_achat);
}

void Kit::setTexte_libre(const QString &newTexte_libre)
{
    texte_libre = newTexte_libre;
}

void Kit::setCaution(const Money &newCaution)
{
    caution = newCaution;
}

void Kit::setCaution(const QString &newCaution)
{
    caution.setValue(newCaution);
}

void Kit::setEn_panne(bool newEn_panne)
{
    en_panne = newEn_panne;
}

void Kit::setCode(const QString &newCode)
{
    code = newCode;
}

QString Kit::toString()
{
    return this->nom + " (" + this->code + ")";
}

Kit::~Kit()
{
    for(const auto& elem : this->item_list)
    {
        delete (elem);
    }
}

bool Kit::getIs_booked() const
{
    return is_booked;
}

void Kit::setIs_booked(bool newIs_booked)
{
    is_booked = newIs_booked;
}

bool Kit::getIs_out() const
{
    return is_out;
}

void Kit::setIs_out(bool newIs_out)
{
    is_out = newIs_out;
}

QString Kit::getIs_out_str() const
{
    if (is_out == true)
    {
        return "Oui";
    }
    else
    {
        return "Non";
    }
}

bool Kit::getIs_in_basket() const
{
    return is_in_basket;
}

void Kit::setIs_in_basket(bool newIs_in_basket)
{
    is_in_basket = newIs_in_basket;
}

bool Kit::getIs_in_resa_view() const
{
    return is_in_resa_view;
}

void Kit::setIs_in_resa_view(bool newIs_in_resa_view)
{
    is_in_resa_view = newIs_in_resa_view;
}


bool Kit::getTo_duplicate() const
{
    return to_duplicate;
}

void Kit::setTo_duplicate(bool newTo_duplicate)
{
    to_duplicate = newTo_duplicate;
}

int Kit::getId_user_out() const
{
    return id_user_out;
}

void Kit::setId_user_out(int newId_user_out)
{
    id_user_out = newId_user_out;
}

int Kit::getId_user_booked() const
{
    return id_user_booked;
}

void Kit::setId_user_booked(int newId_user_booked)
{
    id_user_booked = newId_user_booked;
}

Kit::Kit(QObject *parent)
    : QObject{parent}
{}


Kit::Kit(int idkit, const QString &nom, const QString &description, const QString &date_achat, const QString &prix_achat, const QString &texte_libre, const bool en_panne, const QString &code, const QString &caution)
{
    this->setIdkit(idkit);
    this->setNom(nom);
    this->setDescription(description);
    this->setDate_achat(date_achat);
    this->setPrix_achat(prix_achat);
    this->setTexte_libre(texte_libre);
    this->setEn_panne(en_panne);
    this->setCode(code);
    this->setCaution(caution);
    this->is_booked = false;
    this->is_out = false;
    this->is_in_basket = false;
    this->is_in_resa_view = false;
    this->to_duplicate = false;
}
