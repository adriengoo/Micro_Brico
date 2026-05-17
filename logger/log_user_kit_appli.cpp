#include "log_user_kit_appli.h"
#include "ui_log_user_kit_appli.h"
#include "../log.h"

log_user_kit_appli::log_user_kit_appli(QWidget *parent) :QWidget(parent), ui(new Ui::log_user_kit_appli)
{
    ui->setupUi(this);
}

log_user_kit_appli::~log_user_kit_appli()
{
    delete ui;
    g_utils.clearList(&this->log_list);
}

void log_user_kit_appli::setKit(Kit *newKit)
{
    kit = newKit;
}

void log_user_kit_appli::setUser(Utilisateur *newUser)
{
    user = newUser;
}


void log_user_kit_appli::update_logs_from_kit(int i_max_lines)
{
    g_utils.clearList(&this->log_list);
    g_connect_db.select_logs_by_kit(&this->log_list, kit,i_max_lines);

    for(const auto& elem : this->log_list)
    {
        this->ui->textEdit->insertPlainText(elem->getText());
        this->ui->textEdit->insertPlainText("\n");
    }
    this->ui->textEdit->moveCursor(QTextCursor::Start);
    this->ui->textEdit->ensureCursorVisible();
}

void log_user_kit_appli::update_logs_from_user(int i_max_lines)
{
    g_utils.clearList(&this->log_list);
    g_connect_db.select_logs_by_user(&this->log_list, user,i_max_lines);

    for(const auto& elem : this->log_list)
    {
        this->ui->textEdit->insertPlainText(elem->getText());
        this->ui->textEdit->insertPlainText("\n");
    }
    this->ui->textEdit->moveCursor(QTextCursor::Start);
    this->ui->textEdit->ensureCursorVisible();
}
