#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QStyleFactory>

static QString windows11StyleSheet()
{
    return QStringLiteral(R"(
        QWidget {
            background-color: #f3f3f3;
            color: #1f1f1f;
            font-family: "Segoe UI";
            font-size: 10pt;
        }

        QMainWindow, QDialog {
            background-color: #f3f3f3;
        }

        QMenuBar {
            background-color: #f8f8f8;
            border-bottom: 1px solid #dadada;
            padding: 2px;
        }

        QMenuBar::item {
            background: transparent;
            border-radius: 5px;
            padding: 5px 10px;
        }

        QMenuBar::item:selected {
            background-color: #e9e9e9;
        }

        QMenu {
            background-color: #fbfbfb;
            border: 1px solid #dadada;
            padding: 5px;
        }

        QMenu::item {
            border-radius: 5px;
            padding: 6px 28px 6px 22px;
        }

        QMenu::item:selected {
            background-color: #e5f1fb;
            color: #0f4f82;
        }

        QStatusBar {
            background-color: #f8f8f8;
            border-top: 1px solid #dadada;
        }

        QSplitter::handle {
            background-color: #d8d8d8;
        }

        QSplitter::handle:vertical {
            height: 6px;
            margin: 2px 0;
            border-radius: 3px;
        }

        QSplitter::handle:hover {
            background-color: #0067c0;
        }

        QTabWidget::pane {
            border: 1px solid #d0d0d0;
            border-radius: 7px;
            background-color: #ffffff;
            top: -1px;
        }

        QTabBar::tab {
            background-color: transparent;
            color: #4a4a4a;
            border: 1px solid transparent;
            border-bottom: none;
            border-top-left-radius: 7px;
            border-top-right-radius: 7px;
            padding: 8px 14px;
            margin-right: 2px;
            min-height: 22px;
        }

        QTabBar::tab:selected {
            background-color: #ffffff;
            color: #111111;
            border-color: #d0d0d0;
        }

        QTabBar::tab:hover:!selected {
            background-color: #e9e9e9;
        }

        QGroupBox {
            background-color: #ffffff;
            border: 1px solid #d7d7d7;
            border-radius: 8px;
            margin-top: 20px;
            padding: 12px 10px 10px 10px;
            font-weight: 600;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 6px;
            color: #2b2b2b;
            background-color: rgba(255, 255, 255,50);
        }

        QFrame[frameShape="6"], QFrame[frameShape="StyledPanel"] {
            background-color: #ffffff;
            border: 1px solid #dedede;
            border-radius: 8px;
        }

        QPushButton {
            background-color: #ffffff;
            border: 1px solid #c8c8c8;
            border-radius: 6px;
            padding: 4px 10px;
            min-height: 20px;
        }

        QPushButton:hover {
            background-color: #f7f7f7;
            border-color: #b8b8b8;
        }

        QPushButton:pressed {
            background-color: #ededed;
            border-color: #a8a8a8;
            padding-top: 5px;
            padding-bottom: 3px;
        }

        QPushButton:disabled {
            background-color: #f1f1f1;
            border-color: #dddddd;
            color: #9a9a9a;
        }

        QPushButton:default {
            background-color: #0067c0;
            border-color: #0067c0;
            color: #ffffff;
        }

        QLineEdit,  QTextEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit {
            background-color: #ffffff;
            border: 1px solid #c9c9c9;
            border-radius: 6px;
            padding: 4px 7px;
            min-height: 20px;
            selection-background-color: #0067c0;
            selection-color: #ffffff;
        }

        QLabel {
            background-color: rgba(255, 255, 255,50);
 
        }

        QLineEdit:hover, QTextEdit:hover, QPlainTextEdit:hover, QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover, QDateEdit:hover {
            border-color: #9f9f9f;
        }

        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QDateEdit:focus {
            background-color: #ffffff;
            border: 1px solid #0067c0;
            border-bottom: 2px solid #0067c0;
        }

        QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled, QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QDateEdit:disabled {
            background-color: #f1f1f1;
            border-color: #dddddd;
            color: #8d8d8d;
        }

        QComboBox::drop-down, QDateEdit::drop-down {
            width: 28px;
            border: none;
        }

        QAbstractItemView {
            background-color: #ffffff;
            alternate-background-color: #f8f8f8;
            border: 1px solid #d7d7d7;
            border-radius: 6px;
            outline: 0;
            selection-background-color: #d8ecff;
            selection-color: #111111;
        }

        QAbstractItemView::item {
            min-height: 24px;
            padding: 4px;
        }

        QAbstractItemView::item:hover {
            background-color: #edf6ff;
        }

        QTableWidget, QListWidget, QTreeWidget {
            gridline-color: #e5e5e5;
            background-color: #ffffff;
            border: 1px solid #d7d7d7;
            border-radius: 7px;
            alternate-background-color: #fafafa;
        }

        QHeaderView::section {
            background-color: #f6f6f6;
            border: none;
            border-right: 1px solid #e2e2e2;
            border-bottom: 1px solid #d7d7d7;
            padding: 7px 8px;
            font-weight: 600;
        }

        QCheckBox {
            spacing: 8px;
            background: transparent;
        }

        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid #8f8f8f;
            border-radius: 4px;
            background-color: #ffffff;
        }

        QCheckBox::indicator:hover {
            border-color: #0067c0;
        }

        QCheckBox::indicator:checked {
            background-color: #0067c0;
            border-color: #0067c0;
        }

        QCalendarWidget QWidget {
            alternate-background-color: #f8f8f8;
        }

        QScrollBar:vertical {
            background: transparent;
            width: 14px;
            margin: 2px;
        }

        QScrollBar::handle:vertical {
            background: #c2c2c2;
            border-radius: 6px;
            min-height: 28px;
        }

        QScrollBar::handle:vertical:hover {
            background: #9f9f9f;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }

        QScrollBar:horizontal {
            background: transparent;
            height: 14px;
            margin: 2px;
        }

        QScrollBar::handle:horizontal {
            background: #c2c2c2;
            border-radius: 6px;
            min-width: 28px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #9f9f9f;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
    )");
}

int main(int argc, char *argv[])
{
    //QCoreApplication coreApp(argc, argv);
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    a.setFont(QFont("Segoe UI", 10));
    a.setStyleSheet(windows11StyleSheet());
    MainWindow w;
    w.show();

    a.exec();
    return 0;
}
