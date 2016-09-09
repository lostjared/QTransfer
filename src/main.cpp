#include "transfer_window.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    TransferWindow twindow;
    twindow.show();
    return app.exec();
}