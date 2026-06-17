#include "chat_window.h"

#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

namespace {
QString escapeHtml(QString text) {
    return text.toHtmlEscaped();
}

QString timestamp() {
    return QDateTime::currentDateTime().toString("hh:mm:ss");
}
} // namespace

ChatWindow::ChatWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
}

ChatWindow::~ChatWindow() {
    closeResources();
}

void ChatWindow::closeResources() {
    if (socket_ != nullptr) {
        socket_->disconnect(this);
        socket_->disconnectFromHost();
        delete socket_;
        socket_ = nullptr;
    }

    if (server_ != nullptr) {
        server_->close();
        delete server_;
        server_ = nullptr;
    }
}

void ChatWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    chat_view_ = new QTextBrowser(central);
    chat_view_->setOpenExternalLinks(true);
    chat_view_->setHtml("<div style='color:#666'>Chat ready.</div>");
    layout->addWidget(chat_view_, 1);

    auto *row = new QHBoxLayout();
    message_edit_ = new QLineEdit(central);
    message_edit_->setPlaceholderText("Type a message...");
    send_button_ = new QPushButton(tr("Send"), central);
    row->addWidget(message_edit_, 1);
    row->addWidget(send_button_);
    layout->addLayout(row);

    status_label_ = new QLabel(tr("Disconnected"), central);
    layout->addWidget(status_label_);

    setCentralWidget(central);
    setWindowTitle(tr("QTransfer Chat"));
    resize(560, 420);

    connect(send_button_, &QPushButton::clicked, this, &ChatWindow::onSendClicked);
    connect(message_edit_, &QLineEdit::returnPressed, this, &ChatWindow::onSendClicked);
}

void ChatWindow::connectToHost(const QString &host, quint16 port, const QString &nick, const QString &password) {
    mode_ = Mode::Client;
    local_nick_ = nick.trimmed();
    shared_password_ = password;
    peer_nick_.clear();
    pending_buffer_.clear();
    authenticated_ = false;

    closeResources();

    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::connected, this, &ChatWindow::onSocketConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &ChatWindow::onSocketError);
    connect(socket_, &QTcpSocket::readyRead, this, &ChatWindow::onReadyRead);

    setWindowTitle(tr("Chat - Connecting"));
    status_label_->setText(tr("Connecting to %1:%2").arg(host).arg(port));
    socket_->connectToHost(host, port);
}

void ChatWindow::listenOn(quint16 port, const QString &nick, const QString &password) {
    mode_ = Mode::Server;
    local_nick_ = nick.trimmed();
    shared_password_ = password;
    peer_nick_.clear();
    pending_buffer_.clear();
    authenticated_ = false;

    closeResources();

    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, &ChatWindow::onNewConnection);

    if (!server_->listen(QHostAddress::Any, port)) {
        QMessageBox::warning(this, tr("Chat"), tr("Could not start chat listener."));
        return;
    }

    setWindowTitle(tr("Chat - Listening"));
    status_label_->setText(tr("Listening on port %1").arg(port));
}

void ChatWindow::onSendClicked() {
    const QString text = message_edit_->text().trimmed();
    if (text.isEmpty() || socket_ == nullptr || !authenticated_) {
        return;
    }

    appendChatMessage(local_nick_, text, true);
    sendLine(QStringLiteral("MSG|") + text);
    message_edit_->clear();
}

void ChatWindow::onReadyRead() {
    if (socket_ == nullptr) {
        return;
    }

    pending_buffer_ += socket_->readAll();

    while (true) {
        const int newline = pending_buffer_.indexOf('\n');
        if (newline < 0) {
            break;
        }

        const QByteArray line = pending_buffer_.left(newline);
        pending_buffer_.remove(0, newline + 1);
        handleLine(QString::fromUtf8(line));
    }
}

void ChatWindow::onSocketConnected() {
    setWindowTitle(tr("Chat - Connected"));
    status_label_->setText(tr("Connected"));
    sendHandshake();
}

void ChatWindow::onSocketDisconnected() {
    status_label_->setText(tr("Disconnected"));
    appendSystemMessage(tr("Connection closed."));
    authenticated_ = false;
}

void ChatWindow::onSocketError(QAbstractSocket::SocketError) {
    if (socket_ != nullptr) {
        status_label_->setText(socket_->errorString());
    }
}

void ChatWindow::onNewConnection() {
    if (server_ == nullptr) {
        return;
    }

    if (socket_ != nullptr) {
        socket_->disconnect(this);
        socket_->deleteLater();
    }

    socket_ = server_->nextPendingConnection();
    server_->close();

    connect(socket_, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &ChatWindow::onSocketError);
    connect(socket_, &QTcpSocket::readyRead, this, &ChatWindow::onReadyRead);

    setWindowTitle(tr("Chat - Connected"));
    status_label_->setText(tr("Peer connected, waiting for handshake..."));
}

void ChatWindow::appendSystemMessage(const QString &text) {
    chat_view_->append(QString("<div style='color:#666;font-style:italic'>[%1] %2</div>")
                           .arg(timestamp(), escapeHtml(text)));
}

void ChatWindow::appendChatMessage(const QString &nick, const QString &text, bool local) {
    const QString color = local ? "#0b5" : "#06c";
    chat_view_->append(QString("<div style='margin:4px 0'><span style='color:#888'>[%1]</span> "
                               "<span style='color:%2;font-weight:600'>%3</span>: %4</div>")
                           .arg(timestamp(), color, escapeHtml(nick), escapeHtml(text)));
}

void ChatWindow::sendHandshake() {
    if (socket_ == nullptr) {
        return;
    }

    sendLine(QStringLiteral("HELLO|") + local_nick_ + QStringLiteral("|") + shared_password_);
}

void ChatWindow::handleLine(const QString &line) {
    const QStringList parts = line.split('|');
    if (parts.isEmpty()) {
        return;
    }

    const QString type = parts.front();
    if (type == QStringLiteral("HELLO")) {
        if (mode_ != Mode::Server) {
            return;
        }
        if (parts.size() != 3) {
            disconnectSession(tr("Invalid handshake."));
            return;
        }
        acceptHandshake(parts);
        return;
    }

    if (type == QStringLiteral("WELCOME")) {
        if (mode_ != Mode::Client || parts.size() != 2) {
            return;
        }
        peer_nick_ = parts[1];
        authenticated_ = true;
        status_label_->setText(tr("Connected to %1").arg(peer_nick_));
        appendSystemMessage(tr("Connected with %1.").arg(peer_nick_));
        return;
    }

    if (type == QStringLiteral("REJECT")) {
        const QString reason = parts.size() > 1 ? parts[1] : tr("Rejected");
        disconnectSession(reason);
        return;
    }

    if (type == QStringLiteral("MSG")) {
        if (parts.size() < 2 || !authenticated_) {
            return;
        }
        appendChatMessage(peer_nick_.isEmpty() ? tr("Peer") : peer_nick_, parts.mid(1).join('|'), false);
    }
}

void ChatWindow::acceptHandshake(const QStringList &parts) {
    const QString peer_nick = parts[1];
    const QString password = parts[2];

    if (password != shared_password_) {
        sendLine(QStringLiteral("REJECT|Bad password"));
        disconnectSession(tr("Bad password."));
        return;
    }

    peer_nick_ = peer_nick;
    authenticated_ = true;
    sendLine(QStringLiteral("WELCOME|") + local_nick_);
    status_label_->setText(tr("Connected to %1").arg(peer_nick_));
    appendSystemMessage(tr("%1 joined the chat.").arg(peer_nick_));
}

void ChatWindow::sendLine(const QString &line) {
    if (socket_ == nullptr) {
        return;
    }

    socket_->write(line.toUtf8());
    socket_->write("\n");
}

void ChatWindow::disconnectSession(const QString &reason) {
    appendSystemMessage(reason);
    if (socket_ != nullptr) {
        socket_->disconnect(this);
        socket_->disconnectFromHost();
    }
    authenticated_ = false;
    status_label_->setText(reason);
}

ChatConnectWindow::ChatConnectWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Chat Connect"));
    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout();
    host_edit_ = new QLineEdit(this);
    host_edit_->setPlaceholderText(tr("127.0.0.1"));
    port_edit_ = new QLineEdit(this);
    port_edit_->setValidator(new QRegularExpressionValidator(QRegularExpression(R"(^\d{1,5}$)"), this));
    port_edit_->setPlaceholderText(tr("5000"));
    nick_edit_ = new QLineEdit(this);
    nick_edit_->setPlaceholderText(tr("nickname"));
    password_edit_ = new QLineEdit(this);
    password_edit_->setEchoMode(QLineEdit::Password);

    form->addRow(tr("Host"), host_edit_);
    form->addRow(tr("Port"), port_edit_);
    form->addRow(tr("Nick"), nick_edit_);
    form->addRow(tr("Password"), password_edit_);
    layout->addLayout(form);

    auto *buttons = new QHBoxLayout();
    connect_button_ = new QPushButton(tr("Connect"), this);
    listen_button_ = new QPushButton(tr("Listen"), this);
    buttons->addWidget(connect_button_);
    buttons->addWidget(listen_button_);
    layout->addLayout(buttons);

    status_label_ = new QLabel(tr("Enter connection details."), this);
    layout->addWidget(status_label_);

    connect(connect_button_, &QPushButton::clicked, this, &ChatConnectWindow::onConnectClicked);
    connect(listen_button_, &QPushButton::clicked, this, &ChatConnectWindow::onListenClicked);

    resize(360, 220);
}

void ChatConnectWindow::onConnectClicked() {
    openChat(false);
}

void ChatConnectWindow::onListenClicked() {
    openChat(true);
}

void ChatConnectWindow::openChat(bool listen_mode) {
    const QString nick = nick_edit_->text().trimmed();
    const QString password = password_edit_->text();
    const QString port_text = port_edit_->text().trimmed();

    if (nick.isEmpty() || password.isEmpty() || port_text.isEmpty()) {
        status_label_->setText(tr("Nick, password, and port are required."));
        return;
    }

    if (nick.contains('|') || password.contains('|') || nick.contains('\n') || password.contains('\n')) {
        status_label_->setText(tr("Nick and password cannot contain | or newlines."));
        return;
    }

    bool ok = false;
    const int port = port_text.toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
        status_label_->setText(tr("Port is invalid."));
        return;
    }

    if (!listen_mode && host_edit_->text().trimmed().isEmpty()) {
        status_label_->setText(tr("Host is required for connect mode."));
        return;
    }

    if (chat_window_ == nullptr) {
        chat_window_ = new ChatWindow(nullptr);
        chat_window_->setAttribute(Qt::WA_DeleteOnClose);
    }

    if (listen_mode) {
        chat_window_->listenOn(static_cast<quint16>(port), nick, password);
        status_label_->setText(tr("Listening for chat on port %1").arg(port));
    } else {
        chat_window_->connectToHost(host_edit_->text().trimmed(), static_cast<quint16>(port), nick, password);
        status_label_->setText(tr("Connecting to chat host."));
    }

    chat_window_->show();
    chat_window_->raise();
    chat_window_->activateWindow();
    hide();
}
