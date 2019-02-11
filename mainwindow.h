#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "n4d.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setCellData(int x, int y, QString *str);
    QString getCellData(int x, int y);

public slots:
    void ChangeStack();
    void cellChanged(int row, int col);

private:
    Ui::MainWindow *ui;
    QString n4duser,n4dpwd;
    N4D* n4d;
};

#endif // MAINWINDOW_H
