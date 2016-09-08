#ifndef __TRANSFER_QT_H__
#define __TRANSFER_QT_H__

#include<QtCore>
#include<QtGui>


class ConnectWindow : public QDialog {
    Q_OBJECT
    
public:
    ConnectWindow(QWidget *parent = 0);
};

class ListenWindow : public QDialog {
    Q_OBJECT
    
public:
    ListenWindow(QWidget *parent = 0);
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
