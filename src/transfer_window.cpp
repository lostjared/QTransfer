#include "transfer_window.h"


TransferWindow::TransferWindow(QWidget *parent) : QMainWindow(parent) {
    setGeometry(100, 100, 640, 200);
    createMenu();
    statusBar()->showMessage(tr("Welcome to QTransfer"));
    con_window = new ConnectWindow(this);
    listen_window = new ListenWindow(this);
    
    file_name = new QLabel(tr("File name"), this);
    file_name->setGeometry(10, 10, 200, 20);
    transfer_bar = new QProgressBar(this);
    transfer_bar->setGeometry(10, 35, 620, 20);
    
    file_cancel = new QPushButton("Cancel", this);
    file_cancel->setGeometry(520, 55, 100, 20);
    file_show = new QPushButton("Show", this);
    file_show->setGeometry(410, 55, 100, 20);
    
    connect(file_cancel, SIGNAL(clicked()), this, SLOT(onCancel()));
    connect(file_show, SIGNAL(clicked()), this, SLOT(onShowInFinder()));
    
    setWindowTitle(tr("QTransfer - "));
    setFixedSize(640, 200);
}

void TransferWindow::createMenu() {
    file_menu = menuBar()->addMenu(tr("&File"));
    file_connect = new QAction(tr("&Connect [Receive File]"), this);
    file_connect->setShortcut(tr("Ctrl+O"));
    file_connect->setStatusTip(tr("Open a Connection [Receive File]"));
    file_menu->addAction(file_connect);
    connect(file_connect, SIGNAL(triggered()), this, SLOT(onConnect()));
    file_listen = new QAction(tr("&Listen for Connection [Send File]"), this);
    file_listen->setShortcut(tr("Ctrl+L"));
    file_listen->setStatusTip(tr("Listen for Connection [Send File]"));
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

void TransferWindow::onCancel() {
    
}
void TransferWindow::onShowInFinder() {
    
}

void TransferWindow::onAbout() {
    QMessageBox::information(this, "About QTransfer", "Written by Jared Bruni in C++<br>\n<a href=\"http://lostsidedead.com\">http://lostsidedead.com</a>");
}

ConnectWindow::ConnectWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100, 100, 310, 75);
    QLabel *lbl_1 = new QLabel("IP: ", this);
    lbl_1->setGeometry(10, 10, 25, 25);
    tex_ip = new QLineEdit("", this);
    tex_ip->setGeometry(35, 10, 100, 20);
    tex_port = new QLineEdit("", this);
    QLabel *lbl_2 = new QLabel("Port: ", this);
    lbl_2->setGeometry(140, 10, 25, 25);
    tex_port->setGeometry(170, 10, 50, 20);
    con_start = new QPushButton("Connect", this);
    con_start->setGeometry(225, 10, 75, 20);
    con_status = new QLabel("Status..", this);
    con_status->setGeometry(10, 35, 75, 25);
    setWindowTitle("Connect to IP Address");
    connect(con_start, SIGNAL(clicked()), this, SLOT(onConnect()));
    setFixedSize(310, 75);
}

// Connect code here
void ConnectWindow::onConnect() {
    
}

ListenWindow::ListenWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100,100,270,110);
    QLabel *lbl_1 = new QLabel("Port: ", this);
    lbl_1->setGeometry(10, 10, 75, 20);
    list_port = new QLineEdit("", this);
    list_port->setGeometry(75, 10, 100, 20);
    list_start = new QPushButton("Listen", this);
    list_start->setGeometry(185, 10, 75, 20);
    list_start->setEnabled(false);
    list_status = new QLabel("Listen Status", this);
    list_status->setGeometry(10, 35, 280, 20);
    setWindowTitle("Listen for Connection");
    list_select = new QPushButton("[File]", this);
    list_select->setGeometry(10,60,50,20);
    list_file = new QLabel("Please Select File...", this);
    list_file->setGeometry(65, 60, 200, 20);
    connect(list_select, SIGNAL(clicked()), this, SLOT(onSelectFile()));
    setFixedSize(270, 110);
    
}


void ListenWindow::onListen() {
    
}

void ListenWindow::onSelectFile() {
    QString input_file = QFileDialog::getOpenFileName(this, "Select a File", "", "");
    if(input_file != "") {
        file_name = input_file;
        list_start->setEnabled(true);
        list_file->setText(file_name);
    }
}

