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
#include <QUrl>
#include <QWidget>
#include <cstdint>
#include <fstream>

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
    bool file_sending = false;
    std::fstream outfile;
    QString ex_file_path;
    std::uintmax_t file_len = 0;
    std::uintmax_t file_bytes = 0;
    QPointer<ChatConnectWindow> chat_connect_window_;
};

#endif
