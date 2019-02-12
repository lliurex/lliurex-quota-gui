#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

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
    void CheckValidation(QString result);
    void InitValidation();
    void cellChanged(int row, int col);

private:
    Ui::MainWindow *ui;
    QThread* local_thread;
};

#endif // MAINWINDOW_H
