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

#include <QByteArray>
#include <QDir>
#include <QFileInfo>

namespace {
    constexpr qint64 READ_CHUNK_SIZE = 4096;
    constexpr qint64 SEND_BUFFER_LIMIT = 64 * 1024;

    int progressPercent(std::uintmax_t current, std::uintmax_t total) {
        if (total == 0) {
            return 0;
        }

        const auto value = static_cast<int>((current * 100) / total);
        return std::clamp(value, 0, 100);
    }

    QString safeFileName(const QString &filename) {
        return QString::fromStdString(std::filesystem::path(filename.toStdString()).filename().string());
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
    server_ = nullptr;
    socket_ = nullptr;
    file_bytes = file_len = 0;
}

void TransferWindow::resetTransferState() {
    receive_buffer_.clear();
    input_file_.close();
    output_file_.close();
    incoming_file_name_.clear();
    outgoing_file_name_.clear();
    expecting_transfer_header_ = false;
    sending_transfer_ = false;
    send_header_queued_ = false;
    send_finished_ = false;
    file_len = 0;
    file_bytes = 0;
    send_offset_ = 0;
    receive_offset_ = 0;
    transfer_bar->setValue(0);
    file_name->clear();
    file_show->setEnabled(false);
}

QString TransferWindow::sanitizedFileName(const QString &filename) {
    return safeFileName(filename);
}

bool TransferWindow::openReceivedFile(const QString &filename) {
    const QString safe_name = sanitizedFileName(filename);
    if (safe_name.isEmpty()) {
        return false;
    }

    const std::filesystem::path full_filename =
        std::filesystem::path(con_window->file_dir.toStdString()) / safe_name.toStdString();

    output_file_.setFileName(QString::fromStdString(full_filename.string()));
    if (!output_file_.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    ex_file_path = QString::fromStdString(full_filename.parent_path().string());
    incoming_file_name_ = safe_name;
    file_name->setText(safe_name);
    return true;
}

void TransferWindow::finishReceiveSuccess() {
    output_file_.flush();
    output_file_.close();
    socket_->disconnectFromHost();
    file_show->setEnabled(true);
    QMessageBox::information(this, tr("File Received"), tr("Transfer complete!"));
}

void TransferWindow::startSendingSelectedFile() {
    const QString safe_name = sanitizedFileName(listen_window->file_name);
    if (safe_name.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid file name."));
        return;
    }

    input_file_.setFileName(listen_window->file_name);
    if (!input_file_.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not find file."));
        return;
    }

    outgoing_file_name_ = safe_name;
    file_name->setText(safe_name);
    ex_file_path = QFileInfo(listen_window->file_name).absolutePath();

    file_len = input_file_.size();
    file_bytes = 0;
    send_offset_ = 0;
    sending_transfer_ = true;
    send_header_queued_ = false;
    send_finished_ = false;

    transfer_bar->setRange(0, 100);
    transfer_bar->setValue(0);

    server_->close();

    const QByteArray header = safe_name.toUtf8() + ':' + QByteArray::number(file_len) + '\n';
    list_socket->write(header);
    send_header_queued_ = true;
    pumpSendFile();
}

void TransferWindow::pumpSendFile() {
    if (!sending_transfer_ || list_socket == nullptr || !input_file_.isOpen() || send_finished_) {
        return;
    }

    while (send_offset_ < file_len && list_socket->bytesToWrite() < SEND_BUFFER_LIMIT) {
        std::array<char, READ_CHUNK_SIZE> buffer{};
        const qint64 bytes_read = input_file_.read(buffer.data(), buffer.size());
        if (bytes_read <= 0) {
            break;
        }

        qint64 accepted = 0;
        while (accepted < bytes_read) {
            const qint64 written = list_socket->write(buffer.data() + accepted, bytes_read - accepted);
            if (written <= 0) {
                input_file_.seek(input_file_.pos() - (bytes_read - accepted));
                return;
            }
            accepted += written;
            send_offset_ += written;
        }

        if (accepted <= 0) {
            break;
        }
        transfer_bar->setValue(progressPercent(static_cast<std::uintmax_t>(send_offset_),
                                               static_cast<std::uintmax_t>(file_len)));
    }

    if (send_offset_ >= file_len && list_socket->bytesToWrite() == 0) {
        finishSendSuccess();
    }
}

void TransferWindow::finishSendSuccess() {
    if (send_finished_) {
        return;
    }

    send_finished_ = true;
    sending_transfer_ = false;
    input_file_.close();
    if (list_socket != nullptr) {
        list_socket->disconnectFromHost();
    }
    file_show->setEnabled(true);
    QMessageBox::information(this, tr("File Sent"), tr("Transfer complete!"));
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

    resetTransferState();
    file_show->setEnabled(false);
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
    resetTransferState();
    file_show->setEnabled(false);
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
    expecting_transfer_header_ = true;
    const QByteArray password = con_window->tex_pass->text().toUtf8() + '\n';
    socket_->write(password);
}
void TransferWindow::onConDisconnected() {
    std::cout << "Disconnected..\n";

    setWindowTitle(tr("QTransfer - "));

    statusBar()->showMessage(tr("Disconnected"));
    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
    expecting_transfer_header_ = false;
    if (output_file_.isOpen()) {
        resetTransferState();
    }
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
    if (output_file_.isOpen() || expecting_transfer_header_) {
        resetTransferState();
    }
}

void TransferWindow::onConReadyRead() {
    receive_buffer_ += socket_->readAll();

    if (expecting_transfer_header_) {
        const int newline = receive_buffer_.indexOf('\n');
        if (newline < 0) {
            return;
        }

        const QByteArray header = receive_buffer_.left(newline);
        receive_buffer_.remove(0, newline + 1);

        if (header == "incorrect") {
            statusBar()->showMessage(tr("Incorrect Password"));
            socket_->close();
            QMessageBox::information(this, tr("Invalid Password"), tr("The password is incorrect. Try again.\n"));
            resetTransferState();
            return;
        }

        const int colon = header.indexOf(':');
        if (colon < 0) {
            socket_->close();
            QMessageBox::warning(this, tr("Invalid host"), tr("Invalid host..\n"));
            con_window->show();
            resetTransferState();
            return;
        }

        const QString filename = QString::fromUtf8(header.left(colon));
        const QByteArray size_bytes = header.mid(colon + 1);

        qint64 len = 0;
        const auto [ptr, ec] = std::from_chars(size_bytes.constData(), size_bytes.constData() + size_bytes.size(), len);
        if (ec != std::errc{} || ptr == size_bytes.constData() || len < 0) {
            QMessageBox::warning(this, tr("Invalid File Length."), tr("Invalid File Length"));
            socket_->close();
            resetTransferState();
            return;
        }

        file_len = len;
        transfer_bar->setRange(0, 100);
        if (!openReceivedFile(filename)) {
            QMessageBox::warning(this, tr("Error could not open file"), tr("Couldn't open file"));
            socket_->close();
            resetTransferState();
            return;
        }

        expecting_transfer_header_ = false;
    }

    if (!expecting_transfer_header_ && output_file_.isOpen()) {
        const qint64 bytes_needed = file_len - receive_offset_;
        const qint64 bytes_to_write = std::min(bytes_needed, static_cast<qint64>(receive_buffer_.size()));
        if (bytes_to_write > 0) {
            qint64 written_total = 0;
            while (written_total < bytes_to_write) {
                const qint64 written =
                    output_file_.write(receive_buffer_.constData() + written_total, bytes_to_write - written_total);
                if (written <= 0) {
                    break;
                }
                written_total += written;
            }

            if (written_total > 0) {
                receive_offset_ += written_total;
                receive_buffer_.remove(0, written_total);
                transfer_bar->setValue(progressPercent(static_cast<std::uintmax_t>(receive_offset_),
                                                       static_cast<std::uintmax_t>(file_len)));
            }
        }

        if (receive_offset_ >= file_len) {
            finishReceiveSuccess();
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
    sending_transfer_ = false;
    if (!send_finished_ && input_file_.isOpen()) {
        resetTransferState();
    }
}

void TransferWindow::onListError(QAbstractSocket::SocketError /*se*/) {
    std::cout << "An Error has occured.\n";
    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
    if (input_file_.isOpen() || sending_transfer_) {
        resetTransferState();
    }
}

void TransferWindow::onListReadyRead() {
    if (!sending_transfer_) {
        const QByteArray password_line = list_socket->readLine();
        const QByteArray expected = listen_window->list_pass->text().toUtf8();
        if (password_line.trimmed() != expected) {
            list_socket->write("incorrect\n");
            list_socket->close();
            statusBar()->showMessage(tr("Invalid password attempt..\n"));
            transfer_bar->setValue(0);
            return;
        }

        statusBar()->showMessage(tr("Password accepted, sending file..\n"));
        startSendingSelectedFile();
    }
}

void TransferWindow::onListBytesWritten(qint64 bytes) {
    if (!sending_transfer_ || send_finished_) {
        return;
    }

    file_bytes += bytes;
    if (file_bytes > file_len) {
        file_bytes = file_len;
    }
    transfer_bar->setValue(progressPercent(static_cast<std::uintmax_t>(file_bytes),
                                           static_cast<std::uintmax_t>(file_len)));
    pumpSendFile();

    if (send_offset_ >= file_len && list_socket->bytesToWrite() == 0) {
        finishSendSuccess();
    }
}

void TransferWindow::onNewConnection() {
    list_socket = server_->nextPendingConnection();
    listen_window->hide();
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
