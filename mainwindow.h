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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "n4d.h"

#include <QDebug>
#include <QThread>
#include <QStandardItemModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTableWidget>
#include <QPushButton>
#include <QMovie>
#include <QSystemTrayIcon>

#include <algorithm>
#include <functional>

namespace Ui {
class MainWindow;
}

struct spin_obj_data {
    QPushButton* obj;
    QString text;
    QMovie* animation;
    time_t start_time;
    std::function<void (void)> cb;
    int timeout;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void EndApplication();
    void ProcessCallback(QtN4DWorker::Methods, QString returned, int serial);
    void InitValidation();
    void InitCheckStatus();
    void InitGetQuotaGroups();
    void InitGetCurrentQuotas();
    void InitGetGolemGroups();
    void PopulateGroupTableWithFilter();
    void PopulateUserTableWithFilter();
    void ResetGroupTable();
    void ResetUserTable();
    void GoToResume();
    void CellChangedUserTable(int row,int col);
    void CellChangedGroupTable(int row, int col);
    void PendingBack();
    void SwitchUserEdition();
    void SwitchGroupEdition();
    void Enable();
    void Disable();
    void removeThread(int i);
    void PendingApply();
    void update_animations(int frame);
    void EndApply();
    void ShowAPP();
    void HideAPP();
private:
    void init_spin_wait(QPushButton* obj,int timeout,std::function<void (void)> f);
    Ui::MainWindow *ui;

    // System structures
    QSystemTrayIcon* tray;
    QMap<int,QThread*> local_thread;
    QString n4duser, n4dpwd;
    QMap<QtN4DWorker::Methods,bool> completedTasks;
    QList<QWidget*> last_page_used;
    QList<QTableWidget*> tablewidgets;
    QStringList golem_groups;
    QMap<QTableWidget*,bool> enable_watch_table;
    QMap<QTableWidget*,QStringList> non_editable_columns;
    QMap<QTableWidget*,QMap<QString,QStringList>*> modelmap;
    QMap<QTableWidget*,bool> pending_changes;
    QList<QStringList> changes_to_apply;
    QList<spin_obj_data> spin_obj_data_list;

    void init_tray(QObject *parent);
    void init_structures(bool init_threads);
    void destroy_structures(bool init_threads);

    void InitN4DCall(QtN4DWorker::Methods method);
    void InitN4DCall(QtN4DWorker::Methods method, QStringList params);
    void runWhenCompletedTask();

    void CheckValidation(QString result); //cb InitValidation
    void CompleteGetData(QString result); //cb GET_DATA
    void CompleteGetStatus(QString result); //cb CheckStatus
    void CompleteGetConfigure(QString result); //cb InitGetQuotaGroups
    void StoreGolemGroups(QString result); //cb GetGolemGroups
    void SystemIsEnabled(QString result); //cb Enable
    void SystemIsDisabled(QString result); //cb Disable
    void AddedUserQuota(QString result); //cb set userquota
    void AddedGroupQuota(QString result); //cb set groupquota

    void PrepareTableMaps();
    void ChangePannel(QWidget* pannel);
    void InitializeTable(QTableWidget* item);
    void makeReadOnlyTable(QTableWidget* table);
    void PopulateTableWithFilters(QTableWidget* table, QStringList showfilter, QStringList filter, QLineEdit* txtfilter, bool overwrite);
    void PopulateTable(QMap<QString,QStringList>* data, QTableWidget* table);
    void PopulateTable(QMap<QString,QStringList>* data, QTableWidget* table, QStringList showfilter, QStringList filter);
    QMap<QString,QStringList> readViewTable(QTableWidget* table);
    QMap<QString,QStringList> getTableDifferences(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2);
    QMap<QString,QStringList> getTableDifferencesWithHeader(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2);
    QMap<QString,QStringList> ApplyChangesToModel(QMap<QString,QStringList>* model,QMap<QString,QStringList>* changes);
    bool MakeSameCols(QMap<QString,QStringList> &model,QMap<QString,QStringList> &changes);
    bool isModified(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2);
    void showConfirmationTable();

    QString normalizeUnits(QString value);
    QString denormalizeUnits(QString value);
    bool isValidQuotaValue(QString value);
    void CellChanged(int row, int col, QTableWidget* table);
    bool checkApplyButtons();
};

#endif // MAINWINDOW_H
