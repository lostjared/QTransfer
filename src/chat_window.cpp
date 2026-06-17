#include "chat_window.h"

#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QMessageBox>
#include <QFont>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

namespace {
constexpr int DIALOG_FONT_SIZE = 10;

QString escapeHtml(QString text) {
    return text.toHtmlEscaped();
}

QString timestamp() {
    return QDateTime::currentDateTime().toString("hh:mm:ss");
}

void applyDialogFont(QWidget *widget) {
    QFont font = widget->font();
    font.setPointSize(DIALOG_FONT_SIZE);
    widget->setFont(font);
}
} // namespace

ChatWindow::ChatWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
}

ChatWindow::~ChatWindow() {
    closeResources();
}

void ChatWindow::closeResources() {
    if (socket != nullptr) {
        socket->disconnect(this);
        socket->disconnectFromHost();
        delete socket;
        socket = nullptr;
    }

    if (server != nullptr) {
        server->close();
        delete server;
        server = nullptr;
    }
}

void ChatWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    chat_view = new QTextBrowser(central);
    chat_view->setOpenExternalLinks(true);
    chat_view->setHtml("<div style='color:#666'>Chat ready.</div>");
    layout->addWidget(chat_view, 1);

    auto *row = new QHBoxLayout();
    message_edit = new QLineEdit(central);
    message_edit->setPlaceholderText("Type a message...");
    send_button = new QPushButton(tr("Send"), central);
    row->addWidget(message_edit, 1);
    row->addWidget(send_button);
    layout->addLayout(row);

    status_label = new QLabel(tr("Disconnected"), central);
    layout->addWidget(status_label);

    setCentralWidget(central);
    setWindowTitle(tr("QTransfer Chat"));
    resize(560, 420);

    connect(send_button, &QPushButton::clicked, this, &ChatWindow::onSendClicked);
    connect(message_edit, &QLineEdit::returnPressed, this, &ChatWindow::onSendClicked);
}

void ChatWindow::connectToHost(const QString &host, quint16 port, const QString &nick, const QString &password) {
    mode = Mode::Client;
    local_nick = nick.trimmed();
    shared_password = password;
    peer_nick.clear();
    pending_buffer.clear();
    authenticated = false;

    closeResources();

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &ChatWindow::onSocketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &ChatWindow::onSocketError);
    connect(socket, &QTcpSocket::readyRead, this, &ChatWindow::onReadyRead);

    setWindowTitle(tr("Chat - Connecting"));
    status_label->setText(tr("Connecting to %1:%2").arg(host).arg(port));
    socket->connectToHost(host, port);
}

void ChatWindow::listenOn(quint16 port, const QString &nick, const QString &password) {
    mode = Mode::Server;
    local_nick = nick.trimmed();
    shared_password = password;
    peer_nick.clear();
    pending_buffer.clear();
    authenticated = false;

    closeResources();

    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &ChatWindow::onNewConnection);

    if (!server->listen(QHostAddress::Any, port)) {
        QMessageBox::warning(this, tr("Chat"), tr("Could not start chat listener."));
        return;
    }

    setWindowTitle(tr("Chat - Listening"));
    status_label->setText(tr("Listening on port %1").arg(port));
}

void ChatWindow::onSendClicked() {
    const QString text = message_edit->text().trimmed();
    if (text.isEmpty() || socket == nullptr || !authenticated) {
        return;
    }

    appendChatMessage(local_nick, text, true);
    sendLine(QStringLiteral("MSG|") + text);
    message_edit->clear();
}

void ChatWindow::onReadyRead() {
    if (socket == nullptr) {
        return;
    }

    pending_buffer += socket->readAll();

    while (true) {
        const int newline = pending_buffer.indexOf('\n');
        if (newline < 0) {
            break;
        }

        const QByteArray line = pending_buffer.left(newline);
        pending_buffer.remove(0, newline + 1);
        handleLine(QString::fromUtf8(line));
    }
}

void ChatWindow::onSocketConnected() {
    setWindowTitle(tr("Chat - Connected"));
    status_label->setText(tr("Connected"));
    sendHandshake();
}

void ChatWindow::onSocketDisconnected() {
    status_label->setText(tr("Disconnected"));
    appendSystemMessage(tr("Connection closed."));
    authenticated = false;
}

void ChatWindow::onSocketError(QAbstractSocket::SocketError) {
    if (socket != nullptr) {
        status_label->setText(socket->errorString());
    }
}

void ChatWindow::onNewConnection() {
    if (server == nullptr) {
        return;
    }

    if (socket != nullptr) {
        socket->disconnect(this);
        socket->deleteLater();
    }

    socket = server->nextPendingConnection();
    server->close();

    connect(socket, &QTcpSocket::disconnected, this, &ChatWindow::onSocketDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &ChatWindow::onSocketError);
    connect(socket, &QTcpSocket::readyRead, this, &ChatWindow::onReadyRead);

    setWindowTitle(tr("Chat - Connected"));
    status_label->setText(tr("Peer connected, waiting for handshake..."));
}

void ChatWindow::appendSystemMessage(const QString &text) {
    chat_view->append(QString("<div style='color:#666;font-style:italic'>[%1] %2</div>")
                           .arg(timestamp(), escapeHtml(text)));
}

void ChatWindow::appendChatMessage(const QString &nick, const QString &text, bool local) {
    const QString color = local ? "#0b5" : "#06c";
    chat_view->append(QString("<div style='margin:4px 0'><span style='color:#888'>[%1]</span> "
                               "<span style='color:%2;font-weight:600'>%3</span>: %4</div>")
                           .arg(timestamp(), color, escapeHtml(nick), escapeHtml(text)));
}

void ChatWindow::sendHandshake() {
    if (socket == nullptr) {
        return;
    }

    sendLine(QStringLiteral("HELLO|") + local_nick + QStringLiteral("|") + shared_password);
}

void ChatWindow::handleLine(const QString &line) {
    const QStringList parts = line.split('|');
    if (parts.isEmpty()) {
        return;
    }

    const QString type = parts.front();
    if (type == QStringLiteral("HELLO")) {
        if (mode != Mode::Server) {
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
        if (mode != Mode::Client || parts.size() != 2) {
            return;
        }
        peer_nick = parts[1];
        authenticated = true;
        status_label->setText(tr("Connected to %1").arg(peer_nick));
        appendSystemMessage(tr("Connected with %1.").arg(peer_nick));
        return;
    }

    if (type == QStringLiteral("REJECT")) {
        const QString reason = parts.size() > 1 ? parts[1] : tr("Rejected");
        disconnectSession(reason);
        return;
    }

    if (type == QStringLiteral("MSG")) {
        if (parts.size() < 2 || !authenticated) {
            return;
        }
        appendChatMessage(peer_nick.isEmpty() ? tr("Peer") : peer_nick, parts.mid(1).join('|'), false);
    }
}

void ChatWindow::acceptHandshake(const QStringList &parts) {
    const QString peer_nick_text = parts[1];
    const QString password = parts[2];

    if (password != shared_password) {
        sendLine(QStringLiteral("REJECT|Bad password"));
        disconnectSession(tr("Bad password."));
        return;
    }

    peer_nick = peer_nick_text;
    authenticated = true;
    sendLine(QStringLiteral("WELCOME|") + local_nick);
    status_label->setText(tr("Connected to %1").arg(peer_nick_text));
    appendSystemMessage(tr("%1 joined the chat.").arg(peer_nick_text));
}

void ChatWindow::sendLine(const QString &line) {
    if (socket == nullptr) {
        return;
    }

    socket->write(line.toUtf8());
    socket->write("\n");
}

void ChatWindow::disconnectSession(const QString &reason) {
    appendSystemMessage(reason);
    if (socket != nullptr) {
        socket->disconnect(this);
        socket->disconnectFromHost();
    }
    authenticated = false;
    status_label->setText(reason);
}

ChatConnectWindow::ChatConnectWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Chat Connect"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto *form = new QFormLayout();
    form->setVerticalSpacing(8);
    form->setHorizontalSpacing(10);
    host_edit = new QLineEdit(this);
    host_edit->setPlaceholderText(tr("127.0.0.1"));
    port_edit = new QLineEdit(this);
    port_edit->setValidator(new QRegularExpressionValidator(QRegularExpression(R"(^\d{1,5}$)"), this));
    port_edit->setPlaceholderText(tr("5000"));
    nick_edit = new QLineEdit(this);
    nick_edit->setPlaceholderText(tr("nickname"));
    password_edit = new QLineEdit(this);
    password_edit->setEchoMode(QLineEdit::Password);

    form->addRow(tr("Host"), host_edit);
    form->addRow(tr("Port"), port_edit);
    form->addRow(tr("Nick"), nick_edit);
    form->addRow(tr("Password"), password_edit);
    layout->addLayout(form);

    auto *buttons = new QHBoxLayout();
    connect_button = new QPushButton(tr("Connect"), this);
    listen_button = new QPushButton(tr("Listen"), this);
    buttons->addWidget(connect_button);
    buttons->addWidget(listen_button);
    layout->addLayout(buttons);

    status_label = new QLabel(tr("Enter connection details."), this);
    layout->addWidget(status_label);

    connect(connect_button, &QPushButton::clicked, this, &ChatConnectWindow::onConnectClicked);
    connect(listen_button, &QPushButton::clicked, this, &ChatConnectWindow::onListenClicked);

    for (auto *widget : {qobject_cast<QWidget *>(host_edit), qobject_cast<QWidget *>(port_edit),
                         qobject_cast<QWidget *>(nick_edit), qobject_cast<QWidget *>(password_edit),
                         qobject_cast<QWidget *>(connect_button), qobject_cast<QWidget *>(listen_button),
                         qobject_cast<QWidget *>(status_label)}) {
        applyDialogFont(widget);
    }

    resize(360, 220);
}

void ChatConnectWindow::onConnectClicked() {
    openChat(false);
}

void ChatConnectWindow::onListenClicked() {
    openChat(true);
}

void ChatConnectWindow::openChat(bool listen_mode) {
    const QString nick = nick_edit->text().trimmed();
    const QString password = password_edit->text();
    const QString port_text = port_edit->text().trimmed();

    if (nick.isEmpty() || password.isEmpty() || port_text.isEmpty()) {
        status_label->setText(tr("Nick, password, and port are required."));
        return;
    }

    if (nick.contains('|') || password.contains('|') || nick.contains('\n') || password.contains('\n')) {
        status_label->setText(tr("Nick and password cannot contain | or newlines."));
        return;
    }

    bool ok = false;
    const int port = port_text.toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
        status_label->setText(tr("Port is invalid."));
        return;
    }

    if (!listen_mode && host_edit->text().trimmed().isEmpty()) {
        status_label->setText(tr("Host is required for connect mode."));
        return;
    }

    if (chat_window == nullptr) {
        chat_window = new ChatWindow(nullptr);
        chat_window->setAttribute(Qt::WA_DeleteOnClose);
    }

    if (listen_mode) {
        chat_window->listenOn(static_cast<quint16>(port), nick, password);
        status_label->setText(tr("Listening for chat on port %1").arg(port));
    } else {
        chat_window->connectToHost(host_edit->text().trimmed(), static_cast<quint16>(port), nick, password);
        status_label->setText(tr("Connecting to chat host."));
    }

    chat_window->show();
    chat_window->raise();
    chat_window->activateWindow();
    hide();
}
