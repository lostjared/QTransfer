#include "transfer_window.h"


TransferWindow::TransferWindow(QWidget *parent) : QMainWindow(parent) {
    setGeometry(100, 100, 800, 600);
    createMenu();
    statusBar()->showMessage(tr("Welcome to QTransfer"));
    con_window = new ConnectWindow(this);
    listen_window = new ListenWindow(this);
}

void TransferWindow::createMenu() {
    file_menu = menuBar()->addMenu(tr("&File"));
    file_connect = new QAction(tr("&Connect"), this);
    file_connect->setShortcut(tr("Ctrl+O"));
    file_connect->setStatusTip(tr("Open a Connection"));
    file_menu->addAction(file_connect);
    connect(file_connect, SIGNAL(triggered()), this, SLOT(onConnect()));
    file_listen = new QAction(tr("&Listen for Connection"), this);
    file_listen->setShortcut(tr("Ctrl+L"));
    file_listen->setStatusTip(tr("Listen for Connection"));
    file_menu->addAction(file_listen);
    connect(file_listen, SIGNAL(triggered()), this, SLOT(onListen()));
    help_menu = menuBar()->addMenu(tr("&Help"));
    help_about = new QAction(tr("&About"), this);
    help_about->setShortcut(tr("Ctrl+A"));
    help_about->setStatusTip(tr("About this program"));
    help_menu->addAction(help_about);
    connect(help_about, SIGNAL(triggered()), this, SLOT(onAbout()));
}

void TransferWindow::onConnect() {
    con_window->show();
}

void TransferWindow::onListen() {
    listen_window->show();
}

void TransferWindow::onAbout() {
    QMessageBox::information(this, "About QTransfer", "Written by Jared Bruni in C++<br>\n<a href=\"http://lostsidedead.com\">http://lostsidedead.com</a>");
}

ConnectWindow::ConnectWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100, 100, 300, 200);
}

ListenWindow::ListenWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100,100,300,200);
}