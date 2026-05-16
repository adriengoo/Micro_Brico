#ifndef POPUPSORTIRRESA_H
#define POPUPSORTIRRESA_H

#include <QWidget>

#include <QPushButton>
#include <QListWidget>
#include "../utilisateur.h"
#include "../kit.h"
#include "../types.h"

namespace Ui {
class PopupSortirResa;
}

class PopupSortirResa : public QWidget
{
    Q_OBJECT

public:
    std::vector<Item*> item_list_dest;
    explicit PopupSortirResa(QWidget *parent = nullptr);
    ~PopupSortirResa();

    QPushButton * getSortirButton();
    QPushButton * getAnnulerButton();
    bool checkIfOk (bool *o_forced_by_admin, QString *o_optional_text);
    Kit *getP_kit() const;
    void setP_kit(Kit *newP_kit);
    void refresh_source_item_list();
    void refresh_dest_item_list();
    void clean_kit();
    void setButtonText(const QString &newButtonText);

    void closeEvent(QCloseEvent *event);
    T_SORTIE_RESTIT getMode() const;
    void setMode(T_SORTIE_RESTIT newMode);

    void setUser (Utilisateur *newUser); // set active user in popup

private slots:
    void on_listWidget_popupSortie_ItemSource_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_popupSortie_ItemDest_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_popupSortie_ItemSource_itemClicked(QListWidgetItem *item);
    void on_pushButton_popupSortie_pushDest_clicked();

private:
    Ui::PopupSortirResa *ui;
    Utilisateur *user; // Active user in popup
    QString buttonText;
    Kit *p_kit;
    T_SORTIE_RESTIT mode;
    Item *get_sel_item_from_QlistWidget(QListWidget *i_QlistWidget, std::vector<Item *> i_item_list);
    void GEN_raise_popup_warning(QString msg);
    void push_item_to_dest_list(int new_quantity);
    Item *get_item_from_id(std::vector<Item *> i_item_list, uint i_id);
    void select_next_item_on_QlistWidget(QListWidget *i_QlistWidget, int i_currentRow);
    void set_spin_box_qty_from_selected_item (void);
signals:
    void delete_popup(void);
};

#endif // POPUPSORTIRRESA_H
