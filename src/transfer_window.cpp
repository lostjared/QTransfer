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
#include <QFormLayout>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {
    constexpr qint64 READ_CHUNK_SIZE = 4096;
    constexpr qint64 SEND_BUFFER_LIMIT = 64 * 1024;
    constexpr int DIALOG_FONT_SIZE = 10;

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

    void applyDialogFont(QWidget *widget) {
        QFont font = widget->font();
        font.setPointSize(DIALOG_FONT_SIZE);
        widget->setFont(font);
    }
} // namespace

TransferWindow::TransferWindow(QWidget *parent) : QMainWindow(parent) {
    createMenu();
    statusBar()->showMessage(tr("Welcome to QTransfer"));
    con_window = new ConnectWindow(this);
    listen_window = new ListenWindow(this);
    con_window->setParentWindow(this);
    listen_window->setParentWindow(this);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("Active Transfer"), central);
    QFont title_font = title->font();
    title_font.setPointSize(title_font.pointSize() + 2);
    title_font.setBold(true);
    title->setFont(title_font);
    layout->addWidget(title);

    file_name = new QLabel(tr("No file selected"), central);
    file_name->setWordWrap(true);
    layout->addWidget(file_name);

    transfer_bar = new QProgressBar(central);
    transfer_bar->setRange(0, 100);
    layout->addWidget(transfer_bar);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch(1);
    file_show = new QPushButton(tr("Show"), central);
    file_cancel = new QPushButton(tr("Cancel"), central);
    buttonRow->addWidget(file_show);
    buttonRow->addWidget(file_cancel);
    layout->addLayout(buttonRow);

    setCentralWidget(central);
    file_show->setEnabled(false);
    connect(file_cancel, &QPushButton::clicked, this, &TransferWindow::onCancel);
    connect(file_show, &QPushButton::clicked, this, &TransferWindow::onShowInFinder);

    setWindowTitle(tr("QTransfer - "));
    setMinimumSize(560, 200);
    server = nullptr;
    socket = nullptr;
    file_bytes = file_len = 0;
}

void TransferWindow::resetTransferState() {
    receive_buffer.clear();
    input_file.close();
    output_file.close();
    incoming_file_name.clear();
    outgoing_file_name.clear();
    expecting_transfer_header = false;
    sending_transfer = false;
    send_header_queued = false;
    send_finished = false;
    file_len = 0;
    file_bytes = 0;
    send_offset = 0;
    receive_offset = 0;
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

    output_file.setFileName(QString::fromStdString(full_filename.string()));
    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    ex_file_path = QString::fromStdString(full_filename.parent_path().string());
    incoming_file_name = safe_name;
    file_name->setText(safe_name);
    return true;
}

void TransferWindow::finishReceiveSuccess() {
    output_file.flush();
    output_file.close();
    socket->disconnectFromHost();
    file_show->setEnabled(true);
    QMessageBox::information(this, tr("File Received"), tr("Transfer complete!"));
}

void TransferWindow::startSendingSelectedFile() {
    const QString safe_name = sanitizedFileName(listen_window->file_name);
    if (safe_name.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid file name."));
        return;
    }

    input_file.setFileName(listen_window->file_name);
    if (!input_file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not find file."));
        return;
    }

    outgoing_file_name = safe_name;
    file_name->setText(safe_name);
    ex_file_path = QFileInfo(listen_window->file_name).absolutePath();

    file_len = input_file.size();
    file_bytes = 0;
    send_offset = 0;
    sending_transfer = true;
    send_header_queued = false;
    send_finished = false;

    transfer_bar->setRange(0, 100);
    transfer_bar->setValue(0);

    server->close();

    const QByteArray header = safe_name.toUtf8() + ':' + QByteArray::number(file_len) + '\n';
    list_socket->write(header);
    send_header_queued = true;
    pumpSendFile();
}

void TransferWindow::pumpSendFile() {
    if (!sending_transfer || list_socket == nullptr || !input_file.isOpen() || send_finished) {
        return;
    }

    while (send_offset < file_len && list_socket->bytesToWrite() < SEND_BUFFER_LIMIT) {
        std::array<char, READ_CHUNK_SIZE> buffer{};
        const qint64 bytes_read = input_file.read(buffer.data(), buffer.size());
        if (bytes_read <= 0) {
            break;
        }

        qint64 accepted = 0;
        while (accepted < bytes_read) {
            const qint64 written = list_socket->write(buffer.data() + accepted, bytes_read - accepted);
            if (written <= 0) {
                input_file.seek(input_file.pos() - (bytes_read - accepted));
                return;
            }
            accepted += written;
            send_offset += written;
        }

        if (accepted <= 0) {
            break;
        }
        transfer_bar->setValue(progressPercent(static_cast<std::uintmax_t>(send_offset),
                                               static_cast<std::uintmax_t>(file_len)));
    }

    if (send_offset >= file_len && list_socket->bytesToWrite() == 0) {
        finishSendSuccess();
    }
}

void TransferWindow::finishSendSuccess() {
    if (send_finished) {
        return;
    }

    send_finished = true;
    sending_transfer = false;
    input_file.close();
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
    if (socket != nullptr) {
        socket->disconnect(this);
        socket->deleteLater();
        socket = nullptr;
    }

    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, &TransferWindow::onConConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TransferWindow::onConDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &TransferWindow::onConError);
    connect(socket, &QTcpSocket::readyRead, this, &TransferWindow::onConReadyRead);

    con_window->con_status->setText("Connecting .... ");
    std::cout << "Connecting to: " << ip.toUtf8().data() << ":" << port << "\n";

    resetTransferState();
    file_show->setEnabled(false);
    socket->connectToHost(ip, port);
    return true;
}

void TransferWindow::listenTo(int port) {
    if (server != nullptr) {
        server->close();
        server->deleteLater();
        server = nullptr;
    }
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &TransferWindow::onNewConnection);
    server->listen(QHostAddress::Any, port);
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
    if (chat_connect_window == nullptr) {
        chat_connect_window = new ChatConnectWindow(this);
    }

    chat_connect_window->show();
    chat_connect_window->raise();
    chat_connect_window->activateWindow();
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
    expecting_transfer_header = true;
    const QByteArray password = con_window->tex_pass->text().toUtf8() + '\n';
    socket->write(password);
}
void TransferWindow::onConDisconnected() {
    std::cout << "Disconnected..\n";

    setWindowTitle(tr("QTransfer - "));

    statusBar()->showMessage(tr("Disconnected"));
    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
    expecting_transfer_header = false;
    if (output_file.isOpen()) {
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
    if (output_file.isOpen() || expecting_transfer_header) {
        resetTransferState();
    }
}

void TransferWindow::onConReadyRead() {
    receive_buffer += socket->readAll();

    if (expecting_transfer_header) {
        const int newline = receive_buffer.indexOf('\n');
        if (newline < 0) {
            return;
        }

        const QByteArray header = receive_buffer.left(newline);
        receive_buffer.remove(0, newline + 1);

        if (header == "incorrect") {
            statusBar()->showMessage(tr("Incorrect Password"));
            socket->close();
            QMessageBox::information(this, tr("Invalid Password"), tr("The password is incorrect. Try again.\n"));
            resetTransferState();
            return;
        }

        const int colon = header.indexOf(':');
        if (colon < 0) {
            socket->close();
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
            socket->close();
            resetTransferState();
            return;
        }

        file_len = len;
        transfer_bar->setRange(0, 100);
        if (!openReceivedFile(filename)) {
            QMessageBox::warning(this, tr("Error could not open file"), tr("Couldn't open file"));
            socket->close();
            resetTransferState();
            return;
        }

        expecting_transfer_header = false;
    }

    if (!expecting_transfer_header && output_file.isOpen()) {
        const qint64 bytes_needed = file_len - receive_offset;
        const qint64 bytes_to_write = std::min(bytes_needed, static_cast<qint64>(receive_buffer.size()));
        if (bytes_to_write > 0) {
            qint64 written_total = 0;
            while (written_total < bytes_to_write) {
                const qint64 written =
                    output_file.write(receive_buffer.constData() + written_total, bytes_to_write - written_total);
                if (written <= 0) {
                    break;
                }
                written_total += written;
            }

            if (written_total > 0) {
                receive_offset += written_total;
                receive_buffer.remove(0, written_total);
                transfer_bar->setValue(progressPercent(static_cast<std::uintmax_t>(receive_offset),
                                                       static_cast<std::uintmax_t>(file_len)));
            }
        }

        if (receive_offset >= file_len) {
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
    sending_transfer = false;
    if (!send_finished && input_file.isOpen()) {
        resetTransferState();
    }
}

void TransferWindow::onListError(QAbstractSocket::SocketError /*se*/) {
    std::cout << "An Error has occured.\n";
    con_window->con_start->setEnabled(true);
    listen_window->list_start->setEnabled(true);
    if (input_file.isOpen() || sending_transfer) {
        resetTransferState();
    }
}

void TransferWindow::onListReadyRead() {
    if (!sending_transfer) {
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
    if (!sending_transfer || send_finished) {
        return;
    }

    file_bytes += bytes;
    if (file_bytes > file_len) {
        file_bytes = file_len;
    }
    transfer_bar->setValue(progressPercent(static_cast<std::uintmax_t>(file_bytes),
                                           static_cast<std::uintmax_t>(file_len)));
    pumpSendFile();

    if (send_offset >= file_len && list_socket->bytesToWrite() == 0) {
        finishSendSuccess();
    }
}

void TransferWindow::onNewConnection() {
    list_socket = server->nextPendingConnection();
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
    setWindowTitle(tr("Connect to IP Address"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);

    auto *lbl_1 = new QLabel(tr("IP:"), this);
    auto *lbl_2 = new QLabel(tr("Port:"), this);
    auto *lbl_3 = new QLabel(tr("Password:"), this);
    tex_ip = new QLineEdit(this);
    tex_port = new QLineEdit(this);
    tex_port->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), this));
    tex_pass = new QLineEdit(tr("password"), this);

    form->addRow(lbl_1, tex_ip);
    form->addRow(lbl_2, tex_port);
    form->addRow(lbl_3, tex_pass);
    layout->addLayout(form);

    auto *dirRow = new QHBoxLayout();
    dirRow->setSpacing(10);
    con_path = new QPushButton(tr("Select Directory"), this);
    con_pathf = new QLabel(tr("No directory selected"), this);
    con_pathf->setWordWrap(true);
    dirRow->addWidget(con_path);
    dirRow->addWidget(con_pathf, 1);
    layout->addLayout(dirRow);

    con_status = new QLabel(tr("Status.."), this);
    con_status->setWordWrap(true);
    layout->addWidget(con_status);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch(1);
    con_start = new QPushButton(tr("Connect"), this);
    buttonRow->addWidget(con_start);
    layout->addLayout(buttonRow);

    setMinimumSize(380, 220);
    connect(con_start, &QPushButton::clicked, this, &ConnectWindow::onConnect);
    connect(con_path, &QPushButton::clicked, this, &ConnectWindow::onSelectDir);

    for (auto *widget : {qobject_cast<QWidget *>(lbl_1), qobject_cast<QWidget *>(lbl_2), qobject_cast<QWidget *>(lbl_3),
                         qobject_cast<QWidget *>(tex_ip), qobject_cast<QWidget *>(tex_port), qobject_cast<QWidget *>(tex_pass),
                         qobject_cast<QWidget *>(con_start), qobject_cast<QWidget *>(con_path), qobject_cast<QWidget *>(con_status),
                         qobject_cast<QWidget *>(con_pathf)}) {
        applyDialogFont(widget);
    }
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
    parent_window->connectTo(ip, the_port);
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
    parent_window = win;
}

ListenWindow::ListenWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Listen for Connection"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);

    auto *lbl_1 = new QLabel(tr("Port:"), this);
    auto *lbl_2 = new QLabel(tr("Password:"), this);
    list_port = new QLineEdit(this);
    list_port->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), this));
    list_pass = new QLineEdit(tr("password"), this);

    form->addRow(lbl_1, list_port);
    form->addRow(lbl_2, list_pass);
    layout->addLayout(form);

    auto *fileRow = new QHBoxLayout();
    fileRow->setSpacing(10);
    list_select = new QPushButton(tr("Select File"), this);
    list_file = new QLabel(tr("Please select a file..."), this);
    list_file->setWordWrap(true);
    fileRow->addWidget(list_select);
    fileRow->addWidget(list_file, 1);
    layout->addLayout(fileRow);

    list_status = new QLabel(tr("Listen Status"), this);
    list_status->setWordWrap(true);
    layout->addWidget(list_status);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch(1);
    list_start = new QPushButton(tr("Listen"), this);
    buttonRow->addWidget(list_start);
    layout->addLayout(buttonRow);

    setMinimumSize(380, 210);
    connect(list_select, &QPushButton::clicked, this, &ListenWindow::onSelectFile);
    connect(list_start, &QPushButton::clicked, this, &ListenWindow::onListen);

    for (auto *widget : {qobject_cast<QWidget *>(lbl_1), qobject_cast<QWidget *>(lbl_2),
                         qobject_cast<QWidget *>(list_port), qobject_cast<QWidget *>(list_pass),
                         qobject_cast<QWidget *>(list_start), qobject_cast<QWidget *>(list_select),
                         qobject_cast<QWidget *>(list_status), qobject_cast<QWidget *>(list_file)}) {
        applyDialogFont(widget);
    }
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

    parent_window->listenTo(port.toInt());
}

void ListenWindow::onSelectFile() {
    const QString input_file = QFileDialog::getOpenFileName(this, tr("Select a File"), QString(), QString());
    if (!input_file.isEmpty()) {
        file_name = input_file;
        list_file->setText(file_name);
    }
}

void ListenWindow::setParentWindow(TransferWindow *win) {
    parent_window = win;
}
