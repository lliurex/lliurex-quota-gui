/*
    lliurex quota gui
    Copyright (C) 2019 M.Angel Juan <m.angel.juan@gmail.com>
    Copyright (C) 2019  Enrique Medina Gremaldos <quiqueiii@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
