#include <QApplication>
#include "sysmon.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false); // Keeps process alive in tray

    SysmonV3 w;
    w.show();

    return a.exec();
}