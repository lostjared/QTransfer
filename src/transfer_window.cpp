#include "transfer_window.h"
#include "chat_window.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include <QDir>

namespace {
    constexpr std::size_t kReadChunkSize = 4096;

    int progressPercent(std::uintmax_t current, std::uintmax_t total) {
        if (total == 0) {
            return 0;
        }

        const auto value = static_cast<int>((current * 100) / total);
        return std::clamp(value, 0, 100);
    }
} // namespace

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
    connect(file_cancel, &QPushButton::clicked, this, &TransferWindow::onCancel);
    connect(file_show, &QPushButton::clicked, this, &TransferWindow::onShowInFinder);

    setWindowTitle(tr("QTransfer - "));
    setFixedSize(640, 170);
    file_sending = false;
    server_ = nullptr;
    socket_ = nullptr;
    file_bytes = file_len = 0;
}

void TransferWindow::createMenu() {
    file_menu = menuBar()->addMenu(tr("&File"));
    file_connect = new QAction(tr("&Connect [Receive File]"), this);
    file_connect->setShortcut(tr("Ctrl+O"));
    file_connect->setStatusTip(tr("Open a Connection [Receive File]"));
    file_menu->addAction(file_connect);
    connect(file_connect, &QAction::triggered, this, &TransferWindow::onConnect);
    file_listen = new QAction(tr("&Listen for Connection [Send File]"), this);
    file_listen->setShortcut(tr("Ctrl+L"));
    file_listen->setStatusTip(tr("Listen for Connection [Send File]"));
    file_menu->addAction(file_listen);
    connect(file_listen, &QAction::triggered, this, &TransferWindow::onListen);
    file_chat = new QAction(tr("&Chat..."), this);
    file_chat->setShortcut(tr("Ctrl+T"));
    file_chat->setStatusTip(tr("Open a rich text chat session"));
    file_menu->addAction(file_chat);
    connect(file_chat, &QAction::triggered, this, &TransferWindow::onChat);
    file_exit = new QAction(tr("E&xit"), this);
    file_exit->setShortcut(tr("Ctrl+E"));
    file_exit->setStatusTip(tr("Exit program"));
    file_menu->addAction(file_exit);
    connect(file_exit, &QAction::triggered, this, &TransferWindow::onExit);
    help_menu = menuBar()->addMenu(tr("&Help"));
    help_about = new QAction(tr("&About"), this);
    help_about->setShortcut(tr("Ctrl+A"));
    help_about->setStatusTip(tr("About this program"));
    help_menu->addAction(help_about);
    connect(help_about, &QAction::triggered, this, &TransferWindow::onAbout);
}

bool TransferWindow::connectTo(const QString &ip, int port) {
    if (socket_ != nullptr) {
        socket_->disconnect(this);
        socket_->deleteLater();
        socket_ = nullptr;
    }

    socket_ = new QTcpSocket(this);

    connect(socket_, &QTcpSocket::connected, this, &TransferWindow::onConConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &TransferWindow::onConDisconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &TransferWindow::onConError);
    connect(socket_, &QTcpSocket::readyRead, this, &TransferWindow::onConReadyRead);

    con_window->con_status->setText("Connecting .... ");
    std::cout << "Connecting to: " << ip.toUtf8().data() << ":" << port << "\n";

    socket_->connectToHost(ip, port);
    return true;
}

void TransferWindow::listenTo(int port) {
    if (server_ != nullptr) {
        server_->close();
        server_->deleteLater();
        server_ = nullptr;
    }
    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, &TransferWindow::onNewConnection);
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

void TransferWindow::onChat() {
    if (chat_connect_window_ == nullptr) {
        chat_connect_window_ = new ChatConnectWindow(this);
    }

    chat_connect_window_->show();
    chat_connect_window_->raise();
    chat_connect_window_->activateWindow();
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
    if (se != QAbstractSocket::RemoteHostClosedError)
        std::cout << "Error occoured: " << se << ".\n";
    QString value;
    QTextStream stream(&value);

    stream << "An Error has occured: " << se << "\n";

    if (se == QAbstractSocket::ConnectionRefusedError)
        con_window->con_status->setText(tr("Connection refused..\n"));
    else if (se == QAbstractSocket::RemoteHostClosedError)
        con_window->con_status->setText(tr("Remote host closed connection..\n"));
    else
        con_window->con_status->setText(value);

    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
}

void TransferWindow::onConReadyRead() {
    std::cout << "Bytes ready..\n";

    if (!file_sending) {
        const QByteArray header = socket_->readLine();
        if (!header.isEmpty()) {
            const std::string_view line{header.constData(), static_cast<std::size_t>(header.size())};

            if (line == "incorrect\n") {
                statusBar()->showMessage(tr("Incorrect Password"));
                socket_->close();
                QMessageBox::information(this, tr("Invalid Password"), tr("The password is incorrect. Try again.\n"));
                transfer_bar->setValue(0);
                return;
            }

            const auto col_pos = line.find(':');
            if (col_pos == std::string_view::npos) {
                socket_->close();
                QMessageBox::warning(this, tr("Invalid host"), tr("Invalid host..\n"));
                con_window->show();
                return;
            }

            const std::string filename{line.substr(0, col_pos)};
            const std::string_view size_view = line.substr(col_pos + 1);

            std::uintmax_t len = 0;
            const auto [ptr, ec] = std::from_chars(size_view.data(), size_view.data() + size_view.size(), len);
            if (ec != std::errc{} || ptr == size_view.data()) {
                QMessageBox::warning(this, tr("Invalid File Length."), tr("Invalid File Length"));
                socket_->close();
                return;
            }

            file_name->setText(filename.c_str());
            file_len = len;

            if (len == 0) {
                QMessageBox::warning(this, tr("Invalid File Length."), tr("Invalid File Length"));
            }

            transfer_bar->setRange(0, 100);

            const std::filesystem::path full_filename = std::filesystem::path(con_window->file_dir.toStdString()) / filename;
            outfile.open(full_filename.string(), std::ios::out | std::ios::binary);
            ex_file_path = QString::fromStdString(full_filename.parent_path().string());
            if (!outfile.is_open()) {
                QMessageBox::warning(this, tr("Error could not open file"), tr("Couldn't open file"));
                socket_->close();
                return;
            }

            std::uintmax_t pos = 0;
            while (pos < len) {
                std::array<char, kReadChunkSize> buf{};
                const auto bytes_read = socket_->read(buf.data(), static_cast<qint64>(buf.size()));
                if (bytes_read <= 0) {
                    if (!socket_->waitForReadyRead()) {
                        break;
                    }
                    continue;
                }

                outfile.write(buf.data(), bytes_read);
                pos += static_cast<std::uintmax_t>(bytes_read);
                transfer_bar->setValue(progressPercent(pos, len));

                static unsigned int counter = 0;

                ++counter;
                if ((counter % 3000) == 0) {
                    QApplication::processEvents();
                }
            }
            outfile.close();
            socket_->close();
            file_show->setEnabled(true);

            if (pos >= len) {
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
    if (!file_sending) {
        const QByteArray password_line = list_socket->readLine();
        const QString expected = listen_window->list_pass->text() + '\n';
        if (password_line != expected.toUtf8()) {
            list_socket->write("incorrect\n");
            list_socket->close();
            statusBar()->showMessage(tr("Invalid password attempt..\n"));
            transfer_bar->setValue(0);
            file_sending = false;
            return;
        } else {
            statusBar()->showMessage(tr("Password accepted, sending file..\n"));
            std::fstream file;
            const std::filesystem::path input_path = listen_window->file_name.toStdString();
            file.open(input_path.string(), std::ios::in | std::ios::binary);
            ex_file_path = QString::fromStdString(input_path.parent_path().string());

            if (!file.is_open()) {
                QMessageBox::warning(this, tr("Error"), tr("Could not find file."));
                return;
            }

            const std::string fname = input_path.filename().string();

            statusBar()->showMessage(tr("Sending file..."));
            server_->close();

            file.seekg(0, std::ios::end);
            const std::uintmax_t len = static_cast<std::uintmax_t>(file.tellg());
            file_len = len;
            file.seekg(0, std::ios::beg);

            std::string header = fname;
            header.push_back(':');
            header += std::to_string(len);
            header.push_back('\n');

            file_name->setText(fname.c_str());
            std::cout << header << "\n";

            transfer_bar->setRange(0, 100);
            file_bytes = 0;

            list_socket->write(header.data(), static_cast<qint64>(header.size()));

            while (!file.eof()) {
                std::array<char, kReadChunkSize> buf{};
                file.read(buf.data(), static_cast<std::streamsize>(buf.size()));
                const auto bytes_read = file.gcount();
                if (bytes_read <= 0) {
                    break;
                }

                list_socket->write(buf.data(), bytes_read);

                static unsigned int counter = 0;

                ++counter;
                if ((counter % 3000) == 0) {
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
    transfer_bar->setValue(progressPercent(file_bytes, file_len));
    if (file_bytes >= file_len) {
        QMessageBox::information(this, tr("File Sent."), tr("Transfer Complete!"));
    }
}

void TransferWindow::onNewConnection() {
    list_socket = server_->nextPendingConnection();
    listen_window->hide();
    connect(list_socket, &QTcpSocket::connected, this, &TransferWindow::onListConnected);
    connect(list_socket, &QTcpSocket::disconnected, this, &TransferWindow::onListDisconnected);
    connect(list_socket, &QTcpSocket::errorOccurred, this, &TransferWindow::onListError);
    connect(list_socket, &QTcpSocket::readyRead, this, &TransferWindow::onListReadyRead);
    connect(list_socket, &QTcpSocket::bytesWritten, this, &TransferWindow::onListBytesWritten);
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
    connect(con_start, &QPushButton::clicked, this, &ConnectWindow::onConnect);
    setFixedSize(310, 150);
    con_path = new QPushButton(tr("[Select Dir]"), this);
    con_path->setGeometry(10, 100, 120, 20);
    con_pathf = new QLabel(tr(" Directory "), this);
    con_pathf->setGeometry(135, 100, 200, 20);
    connect(con_path, &QPushButton::clicked, this, &ConnectWindow::onSelectDir);
}

// Connect code here
void ConnectWindow::onConnect() {
    const QString ip = tex_ip->text();
    const QRegularExpression ex(R"(^(?:\d{1,3}\.){3}\d{1,3}$)");
    if (!ex.match(ip).hasMatch()) {
        QMessageBox::information(this, tr("Invalid"), tr("Invalid IP address try again..\n"));
        return;
    }
    const QString port = tex_port->text();
    const int the_port = port.toInt();

    if (the_port <= 0) {
        QMessageBox::information(this, tr("Invalid Port"), tr("Invalid Port Number...\n"));
        return;
    }

    if (tex_pass->text().isEmpty()) {
        QMessageBox::information(this, tr("password required"), tr("Password must be at least 1 character..\n"));
        return;
    }

    if (file_dir.isEmpty()) {
        QMessageBox::information(this, tr("Requires dir path"), tr("You need to provide the directory to save to.."));
        return;
    }
    parent_->connectTo(ip, the_port);
}

void ConnectWindow::onSelectDir() {
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Open Directory"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        file_dir = dir;
        con_pathf->setText(dir);
    }
}

void ConnectWindow::setParentWindow(TransferWindow *win) {
    parent_ = win;
}

ListenWindow::ListenWindow(QWidget *parent) : QDialog(parent) {
    setGeometry(100, 100, 270, 130);
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
    list_select->setGeometry(10, 60, 50, 20);
    list_file = new QLabel(tr("Please Select File..."), this);
    list_file->setGeometry(65, 60, 200, 20);
    QLabel *lbl_2 = new QLabel(tr("Password: "), this);
    lbl_2->setGeometry(10, 85, 75, 20);
    list_pass = new QLineEdit(tr("password"), this);
    list_pass->setGeometry(85, 85, 100, 20);
    connect(list_select, &QPushButton::clicked, this, &ListenWindow::onSelectFile);
    connect(list_start, &QPushButton::clicked, this, &ListenWindow::onListen);
    setFixedSize(270, 130);
}

void ListenWindow::onListen() {
    const QString port = list_port->text();
    if (port.toInt() <= 0) {
        QMessageBox::information(this, tr("Invalid Port"), tr("Invalid Port Number...\n"));
        return;
    }

    if (list_pass->text().isEmpty()) {
        QMessageBox::information(this, tr("Required Pass"), tr("Password must be atleast 1 character..\n"));
        return;
    }

    if (file_name.isEmpty()) {
        QMessageBox::information(this, tr("Required File"), tr("Please Select a File to Transfer"));
        return;
    }

    parent_->listenTo(port.toInt());
}

void ListenWindow::onSelectFile() {
    const QString input_file = QFileDialog::getOpenFileName(this, tr("Select a File"), QString(), QString());
    if (!input_file.isEmpty()) {
        file_name = input_file;
        list_file->setText(file_name);
    }
}

void ListenWindow::setParentWindow(TransferWindow *win) {
    parent_ = win;
}
