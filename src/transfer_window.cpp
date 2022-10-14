#include "transfer_window.h"
#include<iostream>
#include<string>
#include<sstream>
#include<cstdio>
#include<cstdlib>
#include<cstring>


TransferWindow::TransferWindow(QWidget *parent) : QMainWindow(parent) {
    setGeometry(100, 100, 640, 170);
    createMenu();
    statusBar()->showMessage(tr("Welcome to QTransfer"));
    con_window = new ConnectWindow(this);
    listen_window = new ListenWindow(this);
    con_window->setParentWindow(this);
    listen_window->setParentWindow(this);
    
    file_name = new QLabel("", this);
    file_name->setGeometry(10, 25, 200, 20);
    transfer_bar = new QProgressBar(this);
    transfer_bar->setGeometry(10, 55, 620, 20);
    
    file_cancel = new QPushButton("Cancel", this);
    file_cancel->setGeometry(520, 85, 100, 20);
    file_show = new QPushButton("Show", this);
    file_show->setGeometry(410, 85, 100, 20);
    file_show->setEnabled(false);
    connect(file_cancel, SIGNAL(clicked()), this, SLOT(onCancel()));
    connect(file_show, SIGNAL(clicked()), this, SLOT(onShowInFinder()));
    
    setWindowTitle(tr("QTransfer - "));
    setFixedSize(640, 170);
    file_sending = false;
    server_ = NULL;
    socket_ = NULL;
    file_bytes = file_len = 0;
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
    file_exit = new QAction(tr("E&xit"), this);
    file_exit->setShortcut(tr("Ctrl+E"));
    file_exit->setStatusTip(tr("Exit program"));
    file_menu->addAction(file_exit);
    connect(file_exit, SIGNAL(triggered()), this, SLOT(onExit()));
    help_menu = menuBar()->addMenu(tr("&Help"));
    help_about = new QAction(tr("&About"), this);
    help_about->setShortcut(tr("Ctrl+A"));
    help_about->setStatusTip(tr("About this program"));
    help_menu->addAction(help_about);
    connect(help_about, SIGNAL(triggered()), this, SLOT(onAbout()));
}

bool TransferWindow::connectTo(QString ip, int port) {
    
    if(socket_ != NULL) delete socket_;
    
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
    if(server_ != NULL) delete server_;
    server_ = new QTcpServer(this);
    connect(server_, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    server_->listen(QHostAddress::Any, port);
    listen_window->list_status->setText("Waiting for connection...\n");
    statusBar()->showMessage("Waiting for incoming connection....\n");
}

void TransferWindow::onExit() {
    QApplication::exit(0);
}

void TransferWindow::onConnect() {
    con_window->show();
}

void TransferWindow::onListen() {
    listen_window->show();
}

void TransferWindow::onCancel() {
    QApplication::exit(0);
}
void TransferWindow::onShowInFinder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(ex_file_path));
}

void TransferWindow::onConConnected() {
    std::cout << "Connected..\n";
    setWindowTitle(tr("QTransfer - Connect "));
    con_window->hide();
    
    con_window->con_start->setEnabled(false);
    listen_window->list_start->setEnabled(false);

    file_show->setEnabled(false);
    
    statusBar()->showMessage(tr("Connected"));
    QString value;
    QTextStream stream(&value);
    stream << con_window->tex_pass->text() << "\n";
    socket_->write(value.toUtf8().data());
    // recv file
}
void TransferWindow::onConDisconnected() {
    std::cout << "Disconnected..\n";
    
    setWindowTitle(tr("QTransfer - "));
    
    statusBar()->showMessage(tr("Disconnected"));
    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
    

}
void TransferWindow::onConError(QAbstractSocket::SocketError se) {
    if(se != QAbstractSocket::RemoteHostClosedError) std::cout << "Error occoured: " << se << ".\n";
    QString value;
    QTextStream stream(&value);
    
    stream << "An Error has occured: " << se << "\n";
    
    if(se == QAbstractSocket::ConnectionRefusedError)
        con_window->con_status->setText(tr("Connection refused..\n"));
    else if(se == QAbstractSocket::RemoteHostClosedError)
        con_window->con_status->setText(tr("Remote host closed connection..\n"));
    else
        con_window->con_status->setText(value);
    
    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
}

void TransferWindow::onConReadyRead() {
    std::cout << "Bytes ready..\n";
    
    char buf[1024];
    if(file_sending == false) {
        qint64 length = socket_->readLine(buf, sizeof(buf));
        if(length > 0) {
            std::string str = buf;
            
            if(str=="incorrect\n") {
                statusBar()->showMessage(tr("Incorrect Password"));
                socket_->close();
                QMessageBox::information(this, tr("Invalid Password"), tr("The password is incorrect. Try again.\n"));
                transfer_bar->setValue(0);
                return;
            }
    
            unsigned long col_pos = str.find(":");
            if(col_pos == std::string::npos) {
                socket_->close();
                QMessageBox::warning(this, tr("Invalid host"), tr("Invalid host..\n"));
                con_window->show();
                return;
            }
            
            std::string filename = str.substr(0, col_pos);
            
            std::string slen = str.substr(col_pos+1, str.length());
            file_name->setText(filename.c_str());
            unsigned long len = atol(slen.c_str());
            file_len = len;
            
            if(len <= 0) {
                QMessageBox::warning(this, tr("Invalid File Length."), tr("Invalid File Length"));
            }
            
            transfer_bar->setRange(0, 100);
            
            std::string full_filename = std::string(con_window->file_dir.toUtf8().data()) + "/" + filename;
            outfile.open(full_filename, std::ios::out | std::ios::binary);
            ex_file_path = con_window->file_dir;
            if(!outfile.is_open()) {
                QMessageBox::warning(this, tr("Error could not open file"), tr("Couldn't open file"));
                socket_->close();
                return;
            }
            
            unsigned long pos = 0;
            unsigned long it = 0;
            
            
            while(pos < len) {
                char buf[4096];
                it = socket_->read(buf, 4096);
                outfile.write(buf, it);
                transfer_bar->setValue(pos);
                pos += it;
                
                
                double val = pos, size = len, answer = (val/size)*100;
                transfer_bar->setValue(static_cast<int>(answer));
                
                if(it == 0 && !socket_->waitForReadyRead())
                    break;
                
                static unsigned int counter = 0;
                
                ++counter;
                if((counter%3000) == 0) {
                    QApplication::processEvents();
                }
                
            }
            outfile.close();
            socket_->close();
            file_show->setEnabled(true);
            
            if(pos >= len) {
            	QMessageBox::information(this, tr("File Sent."), tr("Transfer Complete!"));
            }
        }
    }
    
}


void TransferWindow::onListConnected() {
    std::cout << "Connected\n";
    listen_window->list_start->setEnabled(false);
    con_window->con_start->setEnabled(false);
    listen_window->hide();
    file_show->setEnabled(false);
    setWindowTitle(tr("QTransfer - Listen "));
}

void TransferWindow::onListDisconnected() {
    std::cout << "Disconnected\n";
    setWindowTitle(tr("QTransfer - "));
    statusBar()->showMessage("Disconnected");
    listen_window->list_start->setEnabled(true);
    con_window->con_start->setEnabled(true);
    
}

void TransferWindow::onListError(QAbstractSocket::SocketError /*se*/) {
    std::cout << "An Error has occured.\n";
    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
    
}

void TransferWindow::onListReadyRead() {
    file_sending = false;
    char buf[1024];
    if(file_sending == false) {
        list_socket->readLine(buf, sizeof(buf));
        QString pw = buf;
        if(pw != listen_window->list_pass->text()+"\n") {
            list_socket->write("incorrect\n");
            list_socket->close();
            statusBar()->showMessage(tr("Invalid password attempt..\n"));
            transfer_bar->setValue(0);
            file_sending = false;
            return;
        } else {
            statusBar()->showMessage(tr("Password accepted, sending file..\n"));
            std::fstream file;
            file.open(listen_window->file_name.toUtf8().data(), std::ios::in|std::ios::binary);
            std::string _filename = listen_window->file_name.toUtf8().data();
            _filename = _filename.substr(0, _filename.rfind("/"));
            ex_file_path = _filename.c_str();
            
            if(!file.is_open()) {
                QMessageBox::warning(this, tr("Error"), tr("Could not find file."));
                return;
            }
            std::string fname = listen_window->file_name.toUtf8().data();
            std::string fn, fc;
            unsigned long offset = 0;
            offset = fname.rfind("/");
            if(offset == std::string::npos)
                offset = 0;
            else
                offset ++;
            
            
            statusBar()->showMessage(tr("Sending file..."));
            server_->close();
            
            fn = fname.substr(offset, fname.length());
            
            file.seekg(0, std::ios::end);
            unsigned long len = file.tellg();
            file_len = len;
            file.seekg(0, std::ios::beg);
            unsigned long pos = 0;
            
            std::ostringstream stream;
            stream << fn << ":" << len << "\n";
            
            file_name->setText(fn.c_str());
            
            std::cout << stream.str() << "\n";
            
            transfer_bar->setRange(0, 100);
            file_bytes = 0;
            char buffer[1024];
            snprintf(buffer, 1023, "%s", stream.str().c_str());
            
            list_socket->write(buffer, qstrlen(buffer));
            
            while(!file.eof()) {
                char buf[4096];
                file.read(buf, 4096);
                int bytes_read=file.gcount();
                if(bytes_read <= 0) break;
                pos += bytes_read;
                 list_socket->write(buf, bytes_read);
                
                static unsigned int counter = 0;
               
                ++counter;
                if((counter%3000) == 0) {
                    QApplication::processEvents();
                }
            }

            file.close();
            list_socket->close();
            file_show->setEnabled(true);
        }
    }
}

void TransferWindow::onListBytesWritten(qint64 bytes) {
    file_bytes += bytes;
    double val = file_bytes, size = file_len, answer = (val/size)*100;
    transfer_bar->setValue(static_cast<int>(answer));
    if(answer >= 100) {
        QMessageBox::information(this, tr("File Sent."), tr("Transfer Complete!"));
     }
}

void TransferWindow::onNewConnection() {
    list_socket = server_->nextPendingConnection();
    listen_window->hide();
    connect(list_socket, SIGNAL(connected()), this, SLOT(onListConnected()));
    connect(list_socket, SIGNAL(disconnected()), this, SLOT(onListDisconnected()));
    connect(list_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onListError(QAbstractSocket::SocketError)));
    connect(list_socket, SIGNAL(readyRead()), this, SLOT(onListReadyRead()));
    connect(list_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(onListBytesWritten(qint64)));
}


void TransferWindow::onAbout() {
    QMessageBox::information(this, tr("About QTransfer"), tr("Written by Jared Bruni in C++<br>\n<br>Be sure to remember to <b>forward the port</b> you choose in your routers settings if your listening for a connection.<br><br><a href=\"http://lostsidedead.com\">http://lostsidedead.com</a>"));
}

ConnectWindow::ConnectWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100, 100, 310, 150);
    QLabel *lbl_1 = new QLabel(tr("IP: "), this);
    lbl_1->setGeometry(10, 10, 25, 25);
    tex_ip = new QLineEdit("", this);
    tex_ip->setGeometry(35, 10, 100, 20);
    tex_port = new QLineEdit("", this);
    tex_port->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), this));
    QLabel *lbl_2 = new QLabel(tr("Port: "), this);
    lbl_2->setGeometry(140, 10, 25, 25);
    tex_port->setGeometry(170, 10, 50, 20);
    con_start = new QPushButton(tr("Connect"), this);
    con_start->setGeometry(225, 10, 75, 20);
    QLabel *lbl_3 = new QLabel(tr("Password: "), this);
    lbl_3->setGeometry(10, 40, 70, 20);
    tex_pass = new QLineEdit(tr("password"), this);
    tex_pass->setGeometry(75, 40, 100, 20);
    con_status = new QLabel(tr("Status.."), this);
    con_status->setGeometry(10, 75, 300, 25);
    setWindowTitle(tr("Connect to IP Address"));
    connect(con_start, SIGNAL(clicked()), this, SLOT(onConnect()));
    setFixedSize(310, 150);
    con_path = new QPushButton(tr("[Select Dir]"), this);
    con_path->setGeometry(10, 100, 120, 20);
    con_pathf = new QLabel(tr(" Directory "), this);
    con_pathf->setGeometry(135, 100, 200, 20);
    connect(con_path, SIGNAL(clicked()), this, SLOT(onSelectDir()));
}

// Connect code here
void ConnectWindow::onConnect() {
    QString ip = tex_ip->text();
    QRegularExpression ex("(\\d{1,3}(\\.\\d{1,3}){3})");
    if(!ex.match(ip).hasMatch()) {
        QMessageBox::information(this, tr("Invalid"), tr("Invalid IP address try again..\n"));
        return;
    }
    QString port = tex_port->text();
    int the_port = port.toInt();
    
    if(the_port <= 0) {
        QMessageBox::information(this, tr("Invalid Port"), tr("Invalid Port Number...\n"));
        return;
    }
    
    if(tex_pass->text().length() == 0) {
        QMessageBox::information(this, tr("password required"), tr("Password must be at least 1 character..\n"));
        return;
    }
    
    if(file_dir.length() == 0) {
        QMessageBox::information(this, tr("Requires dir path"), tr("You need to provide the directory to save to.."));
        return;
    }
    
    if(parent_->connectTo(ip, the_port) == true) {
        
    }
}

void ConnectWindow::onSelectDir() {
 
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/home",                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if(dir != "") {
        file_dir = dir;
        con_pathf->setText(dir);
    }
}

void ConnectWindow::setParentWindow(TransferWindow *win) {
    parent_ = win;
}

ListenWindow::ListenWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100,100,270,130);
    QLabel *lbl_1 = new QLabel(tr("Port: "), this);
    lbl_1->setGeometry(10, 10, 75, 20);
    list_port = new QLineEdit("", this);
    list_port->setGeometry(75, 10, 100, 20);
    list_port->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), this));
    list_start = new QPushButton(tr("Listen"), this);
    list_start->setGeometry(185, 10, 75, 20);
    list_status = new QLabel(tr("Listen Status"), this);
    list_status->setGeometry(10, 35, 280, 20);
    setWindowTitle(tr("Listen for Connection"));
    list_select = new QPushButton("[File]", this);
    list_select->setGeometry(10,60,50,20);
    list_file = new QLabel(tr("Please Select File..."), this);
    list_file->setGeometry(65, 60, 200, 20);
    QLabel *lbl_2 = new QLabel(tr("Password: "), this);
    lbl_2->setGeometry(10, 85, 75, 20);
    list_pass = new QLineEdit(tr("password"), this);
    list_pass->setGeometry(85, 85, 100, 20);
    connect(list_select, SIGNAL(clicked()), this, SLOT(onSelectFile()));
    connect(list_start, SIGNAL(clicked()), this, SLOT(onListen()));
    setFixedSize(270, 130);
}


void ListenWindow::onListen() {
    QString port = list_port->text();
    if(port.toInt() <= 0) {
        QMessageBox::information(this, tr("Invalid Port"), tr("Invalid Port Number...\n"));
        return;
    }
    
    if(list_pass->text().length() == 0) {
        QMessageBox::information(this, tr("Required Pass"), tr("Password must be atleast 1 character..\n"));
        return;
    }
    
    if(file_name.length() == 0) {
        QMessageBox::information(this, tr("Required File"), tr("Please Select a File to Transfer"));
        return;
    }
    
    parent_->listenTo(port.toInt());
}

void ListenWindow::onSelectFile() {
    QString input_file = QFileDialog::getOpenFileName(this, tr("Select a File"), "", "");
    if(input_file != "") {
        file_name = input_file;
        list_file->setText(file_name);
    }
}

void ListenWindow::setParentWindow(TransferWindow *win) {
    parent_ = win;
}

