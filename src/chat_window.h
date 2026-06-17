#ifndef CHAT_WINDOW_H
#define CHAT_WINDOW_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QTextBrowser>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>

class ChatWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow() override;

    void connectToHost(const QString &host, quint16 port, const QString &nick, const QString &password);
    void listenOn(quint16 port, const QString &nick, const QString &password);

  private slots:
    void onSendClicked();
    void onReadyRead();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onNewConnection();

  private:
    enum class Mode {
        Idle,
        Client,
        Server
    };

    void setupUi();
    void appendSystemMessage(const QString &text);
    void appendChatMessage(const QString &nick, const QString &text, bool local);
    void sendHandshake();
    void handleLine(const QString &line);
    void acceptHandshake(const QStringList &parts);
    void sendLine(const QString &line);
    void disconnectSession(const QString &reason);
    void closeResources();

    QTextBrowser *chat_view_ = nullptr;
    QLineEdit *message_edit_ = nullptr;
    QPushButton *send_button_ = nullptr;
    QLabel *status_label_ = nullptr;

    QTcpSocket *socket_ = nullptr;
    QTcpServer *server_ = nullptr;
    Mode mode_ = Mode::Idle;
    QString local_nick_;
    QString shared_password_;
    QString peer_nick_;
    QByteArray pending_buffer_;
    bool authenticated_ = false;
};

class ChatConnectWindow : public QDialog {
    Q_OBJECT

  public:
    explicit ChatConnectWindow(QWidget *parent = nullptr);

  private slots:
    void onConnectClicked();
    void onListenClicked();

  private:
    void openChat(bool listen_mode);

    QLineEdit *host_edit_ = nullptr;
    QLineEdit *port_edit_ = nullptr;
    QLineEdit *nick_edit_ = nullptr;
    QLineEdit *password_edit_ = nullptr;
    QPushButton *connect_button_ = nullptr;
    QPushButton *listen_button_ = nullptr;
    QLabel *status_label_ = nullptr;
    QPointer<ChatWindow> chat_window_;
};

#endif
