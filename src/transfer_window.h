#ifndef __TRANSFER_QT_H__
#define __TRANSFER_QT_H__

#include<QtCore>
#include<QtGui>
#include<QTcpSocket>
#include<QTcpServer>
#include<fstream>

class TransferWindow;

class ConnectWindow : public QDialog {
    Q_OBJECT
    
public:
    ConnectWindow(QWidget *parent = 0);
    void setParentWindow(TransferWindow *win);
    
    
    friend class TransferWindow;
    
public slots:
    void onConnect();
    void onSelectDir();
    
private:
    QLineEdit *tex_ip, *tex_port, *tex_pass;
    QLabel *con_status;
    QPushButton *con_start;
    TransferWindow *parent_;
    QPushButton *con_path;
    QLabel *con_pathf;
    QString file_dir;
};

class ListenWindow : public QDialog {
    Q_OBJECT
    
public:
    ListenWindow(QWidget *parent = 0);
    void setParentWindow(TransferWindow *win);
    
    friend class TransferWindow;
    
public slots:
    void onListen();
    void onSelectFile();
    
private:
    QLineEdit *list_port, *list_pass;
    QPushButton *list_start;
    QPushButton *list_select;
    QLabel *list_status, *list_file;
    QString file_name;
    TransferWindow *parent_;
};


class TransferWindow : public QMainWindow {
    Q_OBJECT
public:
    ConnectWindow *con_window;
    ListenWindow *listen_window;
    
    TransferWindow(QWidget *parent = 0);
    void createMenu();
    
    bool connectTo(QString ip, int port);
    void listenTo(int port);
    
    
public slots:
    void onConnect();
    void onListen();
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
    void onNewConnection();
    
private:
    QMenu *file_menu, *help_menu;
    QAction *file_connect, *file_listen;
    QAction *help_about;
    QProgressBar *transfer_bar;
    QLabel *file_name;
    QPushButton *file_cancel, *file_show;
    QTcpSocket *socket_, *list_socket;
    QTcpServer *server_;
    bool file_sending;
    std::fstream outfile;
    QString ex_file_path;
};


#endif
