#include "transfer_window.h"
#include<iostream>

TransferWindow::TransferWindow(QWidget *parent) : QMainWindow(parent) {
    setGeometry(100, 100, 640, 200);
    createMenu();
    statusBar()->showMessage(tr("Welcome to QTransfer"));
    con_window = new ConnectWindow(this);
    listen_window = new ListenWindow(this);
    con_window->setParentWindow(this);
    listen_window->setParentWindow(this);
    
    file_name = new QLabel(tr("File name"), this);
    file_name->setGeometry(10, 10, 200, 20);
    transfer_bar = new QProgressBar(this);
    transfer_bar->setGeometry(10, 35, 620, 20);
    
    file_cancel = new QPushButton("Cancel", this);
    file_cancel->setGeometry(520, 55, 100, 20);
    file_cancel->setEnabled(false);
    file_show = new QPushButton("Show", this);
    file_show->setGeometry(410, 55, 100, 20);
    file_show->setEnabled(false);
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

bool TransferWindow::connectTo(QString ip, int port) {
    
    socket_ = new QTcpSocket(this);
    
    connect(socket_, SIGNAL(connected()), this, SLOT(onConConnected()));
    connect(socket_, SIGNAL(disconnected()), this, SLOT(onConDisconnected()));
    connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onConError(QAbstractSocket::SocketError)));
    connect(socket_, SIGNAL(readyRead()), this, SLOT(onConReadyRead()));
    
    con_window->con_status->setText("Connecting .... ");
    std::cout << "Connecting to: " << ip.toUtf8().data() << ":" << port << "\n";
    socket_->connectToHost(ip, port);
    
    return true;
}

void TransferWindow::listenTo(int port) {
    
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

void TransferWindow::onConConnected() {
    std::cout << "Connected..\n";
    con_window->hide();
    statusBar()->showMessage("Connected");
    // recv file
}
void TransferWindow::onConDisconnected() {
    std::cout << "Disconnected..\n";
    statusBar()->showMessage("Disconnected");
}
void TransferWindow::onConError(QAbstractSocket::SocketError se) {
    std::cout << "Error occoured.\n";
    if(se == QAbstractSocket::ConnectionRefusedError)
        con_window->con_status->setText(tr("Connection refused..\n"));
    else
        con_window->con_status->setText(tr("An error occured..\n"));
}

void TransferWindow::onConReadyRead() {
    
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
    tex_port->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));
    QLabel *lbl_2 = new QLabel("Port: ", this);
    lbl_2->setGeometry(140, 10, 25, 25);
    tex_port->setGeometry(170, 10, 50, 20);
    con_start = new QPushButton("Connect", this);
    con_start->setGeometry(225, 10, 75, 20);
    con_status = new QLabel("Status..", this);
    con_status->setGeometry(10, 40, 300, 25);
    setWindowTitle("Connect to IP Address");
    connect(con_start, SIGNAL(clicked()), this, SLOT(onConnect()));
    setFixedSize(310, 75);
}

// Connect code here
void ConnectWindow::onConnect() {
    QString ip = tex_ip->text();
    QRegExp ex("(\\d{1,3}(\\.\\d{1,3}){3})");
    if(!ex.exactMatch(ip)) {
        QMessageBox::information(this, "Invalid", "Invalid IP address try again..\n");
        return;
    }
    QString port = tex_port->text();
    
    if(port.toInt() <= 0) {
        QMessageBox::information(this, "Invalid Port", "Invalid Port Number...\n");
        return;
    }
    
    if(parent_->connectTo(ip, port.toInt()) == true) {
        
    }
}

void ConnectWindow::setParentWindow(TransferWindow *win) {
    parent_ = win;
}

ListenWindow::ListenWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100,100,270,110);
    QLabel *lbl_1 = new QLabel("Port: ", this);
    lbl_1->setGeometry(10, 10, 75, 20);
    list_port = new QLineEdit("", this);
    list_port->setGeometry(75, 10, 100, 20);
    list_port->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));
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
    QString port = list_port->text();
    if(port.toInt() <= 0) {
        QMessageBox::information(this, "Invalid Port", "Invalid Port Number...\n");
        return;
    }
    parent_->listenTo(port.toInt());
}

void ListenWindow::onSelectFile() {
    QString input_file = QFileDialog::getOpenFileName(this, "Select a File", "", "");
    if(input_file != "") {
        file_name = input_file;
        list_start->setEnabled(true);
        list_file->setText(file_name);
    }
}

void ListenWindow::setParentWindow(TransferWindow *win) {
    parent_ = win;
}

