#include "transfer_window.h"


TransferWindow::TransferWindow(QWidget *parent) : QMainWindow(parent) {
    setGeometry(100, 100, 800, 600);
    createMenu();
}

void TransferWindow::createMenu() {
    file_menu = menuBar()->addMenu(tr("&File"));
    file_connect = new QAction(tr("&Connect"), this);
    file_connect->setShortcut(tr("Ctrl+O"));
    file_connect->setStatusTip(tr("Open a connection"));
    file_menu->addAction(file_connect);
    connect(file_connect, SIGNAL(triggered()), this, SLOT(onConnect()));
}

void TransferWindow::onConnect() {
    
}