#ifndef __TRANSFER_QT_H__
#define __TRANSFER_QT_H__

#include<QtCore>
#include<QtGui>


class TransferWindow : public QMainWindow {
    
    Q_OBJECT
    
public:
    
    TransferWindow(QWidget *parent = 0);
    void createMenu();
    
public slots:
    void onConnect();
    
private:
    QMenu *file_menu;
    QAction *file_connect;

    
};





#endif
