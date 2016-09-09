#ifndef __TRANSFER_QT_H__
#define __TRANSFER_QT_H__

#include<QtCore>
#include<QtGui>

class TransferWindow;

class ConnectWindow : public QDialog {
    Q_OBJECT
    
public:
    ConnectWindow(QWidget *parent = 0);
    void setParentWindow(TransferWindow *win);
    
public slots:
    void onConnect();
    
private:
    QLineEdit *tex_ip, *tex_port;
    QLabel *con_status;
    QPushButton *con_start;
    TransferWindow *parent_;
};

class ListenWindow : public QDialog {
    Q_OBJECT
    
public:
    ListenWindow(QWidget *parent = 0);
    void setParentWindow(TransferWindow *win);
    
public slots:
    void onListen();
    void onSelectFile();
    
private:
    QLineEdit *list_port;
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
    
private:
    QMenu *file_menu, *help_menu;
    QAction *file_connect, *file_listen;
    QAction *help_about;
    QProgressBar *transfer_bar;
    QLabel *file_name;
    QPushButton *file_cancel, *file_show;
};


#endif
