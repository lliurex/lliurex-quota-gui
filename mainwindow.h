#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTableWidgetItem>

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
    void InitCheckStatus();
    void OpenEditors();
    void ApplyChanges();
    void InitPopulateTable();
    void apply_table_to_model();

    void cell_action(int row, int col);
    void cell_changed(int row,int col);

private:
    Ui::MainWindow *ui;
    QThread* local_thread;
    QString n4duser, n4dpwd;
    DataModel* model;

    void InitN4DCall(QtN4DWorker::Methods method);
    void CheckValidation(QString result); //cb InitValidation
    void CompleteGetStatus(QString result); //cb CheckStatus
    void CompletePopulate(QString result); //cb InitPopulateTable
    void InitializeTable();
};

#endif // MAINWINDOW_H
