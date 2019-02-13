#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

#include "n4d.h"
#include "datamodel.h"

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
    void ProcessCallback(QtN4DWorker::Methods, QString returned);

    void InitValidation();
    void InitPopulateTable();

    void cellChanged(int row, int col);

private:
    Ui::MainWindow *ui;
    QThread* local_thread;
    QString n4duser, n4dpwd;
    DataModel* model;

    void InitN4DCall(QtN4DWorker::Methods method);
    void CheckValidation(QString result); //cb InitValidation
    void CompletePopulate(QString result); //cb InitPopulateTable
};

#endif // MAINWINDOW_H
