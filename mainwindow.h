#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTableWidgetItem>

#include "n4d.h"
//#include "datamodel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    //void setCellData(int x, int y, QString *str);
    //QString getCellData(int x, int y);

public slots:
    void ProcessCallback(QtN4DWorker::Methods, QString returned);

    void InitValidation();
    void InitCheckStatus();
    void InitGetQuotaGroups();
    void InitGetGolemGroups();
    void PopulateGroupTableWithFilter();
    void CheckGroupTableDifferences();
    void CellChangedGroupTable(int row, int col);
    void PendingBack();
    void RestoreGroupTable();
    // IMPLEMENTING
    // void InitPopulateTable();

    /*
     * OLD CODE
     *
    void apply_table_to_model();
    void cell_action(int row, int col);
    void cell_changed(int row,int col);
    */

private:
    Ui::MainWindow *ui;
    QThread* local_thread;
    QString n4duser, n4dpwd;
    QList<QWidget*> last_page_used;
    QList<QWidget*> tablewidgets;
    QMap<QWidget*,bool> enable_watch_group_table;
    QMap<QString,QStringList>* map_group_info;

    // CHECK IF NEEDED
    // DataModel* model;

    void InitN4DCall(QtN4DWorker::Methods method);
    void CheckValidation(QString result); //cb InitValidation
    void CompleteGetStatus(QString result); //cb CheckStatus
    void CompletePopulate(QString result); //cb InitPopulateTable
    void CompleteGetConfigure(QString result); //cb InitGetQuotaGroups
    void StoreGolemGroups(QString result); //cb GetGolemGroups
    void InitializeTable(QTableWidget* item);
    void PopulateTable(QMap<QString,QStringList>* data, QTableWidget* table);
    void PopulateGroupTableWithFilter(QMap<QString,QStringList>* content);
    void PopulateTable(QMap<QString,QStringList>* data, QTableWidget* table, QStringList showfilter, QStringList filter);
    void ChangePannel(QWidget* pannel);
    QMap<QString,QStringList> readViewTable(QTableWidget* table);
    QMap<QString,QStringList> getTableDifferences(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2);
    QMap<QString,QStringList> getTableDifferencesWithHeader(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2);
    QMap<QString,QStringList> ApplyChangesToModel(QMap<QString,QStringList>* model,QMap<QString,QStringList>* changes);
    bool isModified(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2);
    void showConfirmationTable(QTableWidget* table,QMap<QString,QStringList>* model);
    QString normalizeUnits(QString value);
    bool isValidQuotaValue(QString value);
    void makeReadOnlyTable(QTableWidget* table);
};

#endif // MAINWINDOW_H
