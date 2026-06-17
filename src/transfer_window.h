#ifndef __TRANSFER_QT_H__
#define __TRANSFER_QT_H__

#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QPointer>
#include <QStatusBar>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QUrl>
#include <QWidget>
#include <cstdint>

class TransferWindow;
class ChatConnectWindow;

class ConnectWindow : public QDialog {
    Q_OBJECT

  public:
    explicit ConnectWindow(QWidget *parent = nullptr);
    void setParentWindow(TransferWindow *win);

    friend class TransferWindow;

  public slots:
    void onConnect();
    void onSelectDir();

  private:
    QLineEdit *tex_ip = nullptr, *tex_port = nullptr, *tex_pass = nullptr;
    QLabel *con_status = nullptr;
    QPushButton *con_start = nullptr;
    TransferWindow *parent_ = nullptr;
    QPushButton *con_path = nullptr;
    QLabel *con_pathf = nullptr;
    QString file_dir;
};

class ListenWindow : public QDialog {
    Q_OBJECT

  public:
    explicit ListenWindow(QWidget *parent = nullptr);
    void setParentWindow(TransferWindow *win);

    friend class TransferWindow;

  public slots:
    void onListen();
    void onSelectFile();

  private:
    QLineEdit *list_port = nullptr, *list_pass = nullptr;
    QPushButton *list_start = nullptr;
    QPushButton *list_select = nullptr;
    QLabel *list_status = nullptr, *list_file = nullptr;
    QString file_name;
    TransferWindow *parent_ = nullptr;
};

class TransferWindow : public QMainWindow {
    Q_OBJECT
  public:
    ConnectWindow *con_window;
    ListenWindow *listen_window;

    explicit TransferWindow(QWidget *parent = nullptr);
    void createMenu();

    bool connectTo(const QString &ip, int port);
    void listenTo(int port);

  public slots:
    void onExit();
    void onConnect();
    void onListen();
    void onChat();
    void onAbout();
    void onCancel();
    void onShowInFinder();
    void onConConnected();
    void onConDisconnected();
    void onConError(QAbstractSocket::SocketError se);
    void onConReadyRead();
    void onListConnected();
    void onListDisconnected();
    void onListError(QAbstractSocket::SocketError se);
    void onListReadyRead();
    void onListBytesWritten(qint64 bytes);
    void onNewConnection();

  private:
    QMenu *file_menu, *help_menu;
    QAction *file_connect, *file_listen, *file_chat, *file_exit;
    QAction *help_about;
    QProgressBar *transfer_bar = nullptr;
    QLabel *file_name = nullptr;
    QPushButton *file_cancel = nullptr, *file_show = nullptr;
    QTcpSocket *socket_ = nullptr, *list_socket = nullptr;
    QTcpServer *server_ = nullptr;
    QFile input_file_;
    QFile output_file_;
    QByteArray receive_buffer_;
    QString ex_file_path;
    QString incoming_file_name_;
    QString outgoing_file_name_;
    bool expecting_transfer_header_ = false;
    bool sending_transfer_ = false;
    bool send_header_queued_ = false;
    bool send_finished_ = false;
    qint64 file_len = 0;
    qint64 file_bytes = 0;
    qint64 send_offset_ = 0;
    qint64 receive_offset_ = 0;
    QPointer<ChatConnectWindow> chat_connect_window_;

    void resetTransferState();
    void startSendingSelectedFile();
    void pumpSendFile();
    bool openReceivedFile(const QString &filename);
    void finishReceiveSuccess();
    void finishSendSuccess();
    static QString sanitizedFileName(const QString &filename);
};

#endif
