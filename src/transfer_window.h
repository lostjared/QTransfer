#ifndef __TRANSFER_QT_H__
#define __TRANSFER_QT_H__

#include<QtCore>
#include<QtGui>


class ConnectWindow : public QDialog {
    Q_OBJECT
    
public:
    ConnectWindow(QWidget *parent = 0);
    
public slots:
    void onConnect();
    
private:
    QLineEdit *tex_ip, *tex_port;
    QLabel *con_status;
    QPushButton *con_start;
};

class ListenWindow : public QDialog {
    Q_OBJECT
    
public:
    ListenWindow(QWidget *parent = 0);
public slots:
    void onListen();
    
private:
    QLineEdit *list_port;
    QPushButton *list_start;
    QLabel *list_status;
};


class TransferWindow : public QMainWindow {
    Q_OBJECT
public:
    ConnectWindow *con_window;
    ListenWindow *listen_window;
    
    TransferWindow(QWidget *parent = 0);
    void createMenu();
    
public slots:
    void onConnect();
    void onListen();
    void onAbout();
    
private:
    QMenu *file_menu, *help_menu;
    QAction *file_connect, *file_listen;
    QAction *help_about;
};


#endif
