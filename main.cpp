#include "mainwindow.h"
// #include "examples.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
//#include <QCoreApplication>

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);
   QTranslator translator;
   QString locale = QLocale::system().name();

   cout << "Using locale: " << locale.toStdString() << endl;
//   QString qdpath = QCoreApplication::applicationDirPath();
//   cout << qdpath.toStdString() << endl;

   translator.load("/usr/share/lliurex-quota-gui/translations/quotagui_"+locale);
   app.installTranslator(&translator);
   MainWindow w;
   w.show();

   //example_new_image_label();
   //example_new_image_scene();

   return app.exec();
}
