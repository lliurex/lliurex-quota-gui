#include "mainwindow.h"
// #include "examples.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    //example_new_image_label();
    //example_new_image_scene();

    return a.exec();
}
