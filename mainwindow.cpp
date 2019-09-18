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
#include "ui_mainwindow.h"
#include "n4d.h"

#include <QDebug>
#include <QThread>
#include <QStandardItemModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTableWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QList>
#include <QMap>

#include <algorithm>
#include <thread>
#include <chrono>
#include <time.h>

/*
 * Constructors
 * */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init_tray(this);
    init_structures(true);
    ChangePannel(ui->page_login);
}

/*
 * Initialize tray icon
 * */
void MainWindow::init_tray(QObject *parent){
    if (QSystemTrayIcon::isSystemTrayAvailable()){
        tray = new QSystemTrayIcon(QIcon(QPixmap("://rsrc/lliurex-quota.png")),parent);
        const QString msg = QString("Lliurex-quota menu");
        QMenu *menu = new QMenu(msg);
        tray->setContextMenu(menu);
        tray->show();
        ShowAPP();
    }
}


void MainWindow::closeEvent(QCloseEvent *event){
    Q_UNUSED(event);
    EndApplication();
}

/*
 * End application slot doing ending actions
 * */
void MainWindow::EndApplication(){
    tray->hide();
    delete(tray->contextMenu());
    delete(tray);
    exit(0);
}

/*
 * Initializes all private vars & structures
 * */
void MainWindow::init_structures(bool init_threads=true){
    if (init_threads){
        local_thread.clear();
    }
    n4duser = "";
    n4dpwd = "";
    completedTasks.clear();
    changes_to_apply.clear();
    last_page_used.clear();
    tablewidgets.clear();
    golem_groups.clear();
    enable_watch_table.clear();
    non_editable_columns.clear();
    modelmap.clear();
    pending_changes.clear();

    tablewidgets = {ui->tableGroupEdition,ui->table_pending,ui->tableUserEdition};
    for (auto const& k: tablewidgets){
        enable_watch_table.insert(k,false);
        pending_changes.insert(k,false);
    }
    non_editable_columns.insert(ui->tableUserEdition,QStringList({tr("Used"),tr("QuotaApplied")}));
    PrepareTableMaps();
}

/*
 * Destructors
 * */
MainWindow::~MainWindow()
{
    destroy_structures(true);
    delete ui;
}

/*
 * Destroy all private vars & structures
 * */
void MainWindow::destroy_structures(bool init_threads=true){
    if (init_threads){
        for (auto const& k: local_thread.keys()){
            removeThread(k);
        }
    }
    for (auto const& k: tablewidgets){
        if (k != ui->table_pending){
            delete modelmap.value(k);
        }
    }
}

void MainWindow::PrepareTableMaps(){
    QMap<QString,QStringList>* map = nullptr;
    QStringList headers;

    for (auto const& t: tablewidgets){
        if (t == ui->table_pending){
            continue;
        }else{
            map = new QMap<QString,QStringList>;
            if (t == ui->tableGroupEdition){
                headers = QStringList::fromStdList({ tr("Quota") });
            }else{
                headers = QStringList::fromStdList({ tr("Quota") , tr("Used") , tr("QuotaApplied") });
            }
            map->insert("__HEADER__",headers);
            if (!modelmap.contains(t)){
                modelmap.insert(t,map);
            }else{
                delete modelmap.value(t);
                modelmap.insert(t,map);
            }
        }
    }
}
/*
 * COMMON CALLBACK FROM N4D CALL
 * */
void MainWindow::ProcessCallback(QtN4DWorker::Methods from, QString returned, int serial){
    Q_UNUSED(serial);
    if (!completedTasks.keys().contains(from)){
        completedTasks.insert(from,false);
    }
    switch (from){
        case QtN4DWorker::Methods::LOGIN:
            CheckValidation(returned);
            break;
        case QtN4DWorker::Methods::GET_DATA:
            CompleteGetData(returned);
            break;
        case QtN4DWorker::Methods::GET_STATUS:
            CompleteGetStatus(returned);
            break;
        case QtN4DWorker::Methods::GET_CONFIGURED:
            CompleteGetConfigure(returned);
            break;
        case QtN4DWorker::Methods::GET_GOLEM_GROUPS:
            StoreGolemGroups(returned);
            break;
        case QtN4DWorker::Methods::ENABLE_SYSTEM:
            SystemIsEnabled(returned);
            break;
        case QtN4DWorker::Methods::DISABLE_SYSTEM:
            SystemIsDisabled(returned);
            break;
        case QtN4DWorker::Methods::SET_GROUP_QUOTA:
            AddedGroupQuota(returned);
            break;
        case QtN4DWorker::Methods::SET_USER_QUOTA:
            AddedUserQuota(returned);
            break;
    };
    completedTasks[from] = true;
    runWhenCompletedTask();
#ifdef N4D_SHOW_RESULT
    qDebug() << "Result from N4D:" << from;
    qDebug() << returned;
#endif
}

/*
 * RUN ACTIONS MULTIPLE DEPENDENT FROM CALLBACK
 * */
void MainWindow::runWhenCompletedTask(){
    if (    completedTasks.contains(QtN4DWorker::Methods::GET_DATA) &&
            completedTasks.contains(QtN4DWorker::Methods::GET_STATUS) &&
            completedTasks.value(QtN4DWorker::Methods::GET_DATA) &&
            completedTasks.value(QtN4DWorker::Methods::GET_STATUS)){
        ResetGroupTable();
        ResetUserTable();
    }
}

/*
 * SLOT TO START VALIDATION
 * */
void MainWindow::InitValidation(){
    n4duser = ui->txt_usu->text();
    n4dpwd = ui->txt_pwd->text();
    if (n4duser.trimmed() == "" or n4dpwd.trimmed() == ""){
        ui->txt_usu->setText("");
        ui->txt_pwd->setText("");
        return;
    }
    InitN4DCall(QtN4DWorker::Methods::LOGIN);
}

/*
 * SLOT TO START TO GET CURRENT SYSTEM APPLIED QUOTAS
 * */
void MainWindow::InitGetCurrentQuotas(){
    InitN4DCall(QtN4DWorker::Methods::GET_DATA);
}

/*
 * SLOT THAT BEGIN TO GET SYSTEM STATUS
 * */
void MainWindow::InitCheckStatus(){
    QString u = n4duser;
    QString p = n4dpwd;
    QStringList golem = golem_groups;
    QWidget* last;
    if (last_page_used.front() == ui->page_login){
        last = ui->page_group_edit;
    }else if (last_page_used.front() == ui->page_need_configuration){
	last = ui->page_group_edit;
    }else if(last_page_used.front() == ui->page_write_changes){
            last_page_used.pop_front();
	    last = last_page_used.front();
    }else{
        last = last_page_used.front();
    }
    
    destroy_structures(false);
    init_structures(false);
    last_page_used.push_front(last);
    golem_groups = golem;
    n4duser = u;
    n4dpwd = p;
    ui->statusBar->showMessage(tr("Refreshing information"),3000);
    InitN4DCall(QtN4DWorker::Methods::GET_STATUS);
}

/*
 * SLOT THAT START TO GET GROUP LIST & STATUS
 * */
void MainWindow::InitGetQuotaGroups(){
    InitN4DCall(QtN4DWorker::Methods::GET_CONFIGURED);
}

/*
 * SLOT THAT START TO GET CLASSROOM GROUPS
 * */
void MainWindow::InitGetGolemGroups(){
    InitN4DCall(QtN4DWorker::Methods::GET_GOLEM_GROUPS);
}

/*
 * SLOT TO SETUP GROUP FILTER FROM CHECKBOX
 * */
void MainWindow::PopulateGroupTableWithFilter(){
    QStringList filter_show;

    if (ui->check_show_all_groups->isChecked()){
        filter_show.clear();
    }else{
        filter_show = QStringList::fromStdList({"students","teachers","admins"});
        filter_show += golem_groups;
    }
    PopulateTableWithFilters(ui->tableGroupEdition,filter_show,QStringList(),ui->txt_filter_group_table,false);
}

/*
 * SLOT TO SETUP USER FILTER FROM CHECKBOX
 * */
void MainWindow::PopulateUserTableWithFilter(){
    PopulateTableWithFilters(ui->tableUserEdition,QStringList(),QStringList(),ui->txt_filter_user_table,false);
}

/*
 * SLOT UNDOING CHANGES INTO GROUP TABLE
 * */
void MainWindow::ResetGroupTable(){
    QStringList filter_remove;
    QStringList filter_show;

    filter_show = QStringList::fromStdList({"students","teachers","admins"});
    filter_show += golem_groups;
    PopulateTableWithFilters(ui->tableGroupEdition,filter_show,filter_remove,ui->txt_filter_group_table,true);

}

/*
 * SLOT UNDOING CHANGES INTO USERS TABLE
 * */
void MainWindow::ResetUserTable(){
    PopulateTableWithFilters(ui->tableUserEdition,QStringList(),QStringList(),ui->txt_filter_user_table,true);
}

/*
 * SLOT FOR CHECKING TABLE DIFFERENCES & GO TO RESUME PAGE
 * */
void MainWindow::GoToResume(){
    if (checkApplyButtons()){
        ChangePannel(ui->page_write_changes);
    }
}

/*
 * SLOT THAT NORMALIZE, CHECK & ENABLE/DISABLE APPLY BUTTON ON USER TABLE
 * */
void MainWindow::CellChangedUserTable(int row,int col){
    CellChanged(row,col,ui->tableUserEdition);
    checkApplyButtons();
}

/*
 * SLOT THAT NORMALIZE, CHECK & ENABLE/DISABLE APPLY BUTTON ON GROUP TABLE
 * */
void MainWindow::CellChangedGroupTable(int row,int col){
    CellChanged(row,col,ui->tableGroupEdition);
    checkApplyButtons();
}

/*
 * SLOT TO RETURN TO EDITION TABLE
 * */
void MainWindow::PendingBack(){
    ChangePannel(last_page_used.at(1));
}

/*
 * ACTION SLOT TO SWITCH TO USER QUOTA EDITION
 * */
void MainWindow::SwitchUserEdition(){
    ChangePannel(ui->page_edit_simple);
}

/*
 * ACTION SLOT TO SWITCH TO GROUP QUOTA EDITION
 * */
void MainWindow::SwitchGroupEdition(){
    ChangePannel(ui->page_group_edit);
}

/*
 * ACTION SLOT TO ENABLE SYSTEM QUOTAS
 * */
void MainWindow::Enable(){
    InitN4DCall(QtN4DWorker::Methods::ENABLE_SYSTEM);
}

/*
 * ACTION SLOT TO DISABLE SYSTEM QUOTAS
 * */
void MainWindow::Disable(){
    InitN4DCall(QtN4DWorker::Methods::DISABLE_SYSTEM);
}
/*
 * Callback for animated icon
 * */
void MainWindow::update_animations(int frame){
    Q_UNUSED(frame);
    time_t curtime;
    for (int i=0; i<spin_obj_data_list.size(); i++){
        spin_obj_data o = spin_obj_data_list.at(i);
        time(&curtime);
        if (o.timeout > difftime(curtime,o.start_time)){
            o.obj->setIcon(QIcon(o.animation->currentPixmap()));
        }else{
            o.obj->setText(o.text);
            o.obj->setIcon(QIcon());
            disconnect(o.animation,SIGNAL(frameChanged(int)),this,SLOT(update_animations(int)));
            disconnect(o.animation,SIGNAL(finished()),o.animation,SLOT(start()));
            o.animation->deleteLater();
            spin_obj_data_list.removeAt(i);
            o.cb();
        }
    }
}
/*
 * SLOT TO SEND CHANGES TO N4D
 * */

void MainWindow::PendingApply(){
    //qDebug() << "Applying changes:";
    ui->btn_pending_apply->setDisabled(true);
    std::function<void (void)> f = std::bind(&MainWindow::EndApply,this);
    if (! changes_to_apply.empty() ){
        QStringList sl = changes_to_apply.takeFirst();
        // List of changes contains tuples (QStringList) whose items are:
        // QStringList(name, user/group, quotaoldvalue, quotavalue)
        QString msg;
        msg = QString("I will modify: " + sl.at(1) + " " + sl.at(0) + " from " + sl.at(2) + " to " + sl.at(3));
        //qDebug() << msg;
        ui->statusBar->showMessage(msg,5000);
        QStringList params;
        params << "string/" + sl.at(0);
        params << "string/" + denormalizeUnits(sl.at(3));
        params << "string/0";
        if (sl.at(1).toLower() == "group"){
            InitN4DCall(QtN4DWorker::Methods::SET_GROUP_QUOTA,params);
        }
        if (sl.at(1).toLower() == "user"){
            InitN4DCall(QtN4DWorker::Methods::SET_USER_QUOTA,params);
        }
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        if (changes_to_apply.empty()){
            // sleep 5 seconds to sync changes before update current quotas
            ui->statusBar->showMessage(tr("Wait 5 seconds while syncing changes..."),5000);
            init_spin_wait(ui->btn_pending_apply,5,f);
        }
    }
}

void MainWindow::init_spin_wait(QPushButton* obj,int timeout,std::function<void (void)> cb){
    spin_obj_data data;
    data.obj = obj;
    data.animation = new QMovie("://rsrc/loader.gif");
    data.text = obj->text();
    data.timeout = timeout;
    data.cb = cb;

    data.obj->setText(QString(""));
    data.obj->setIcon(QIcon(data.animation->currentPixmap()));
    connect(data.animation,SIGNAL(frameChanged(int)),this,SLOT(update_animations(int)));
    if (data.animation->loopCount() != -1){
        connect(data.animation,SIGNAL(finished()),data.animation,SLOT(start()));
    }
    time(&data.start_time);
    data.animation->start();
    spin_obj_data_list.append(data);
}

void MainWindow::EndApply(){
    qDebug() << "Changes Applied";
    ui->btn_pending_apply->setEnabled(true);
    InitCheckStatus();
}

void MainWindow::ShowAPP(){
    this->show();
    QMenu *m = this->tray->contextMenu();
    m->clear();
    m->addAction("Hide Application",this,SLOT(HideAPP()));
    m->addAction("Exit Application",this,SLOT(EndApplication()));
}

void MainWindow::HideAPP(){
    this->hide();
    QMenu *m = this->tray->contextMenu();
    m->clear();
    m->addAction("Show Application",this,SLOT(ShowAPP()));
    m->addAction("Exit Application",this,SLOT(EndApplication()));
}
/*************************************
 *  END SLOTS
 ************************************/

void MainWindow::removeThread(int i){
#ifdef RUNNING_THREADS
    qDebug() << "RemoveThread: Finishing thread " << i;
#endif
    if (local_thread.contains(i)){
        while (local_thread.value(i)->isRunning()){
#ifdef RUNNING_THREADS
            qDebug() << "RemoveThread: Waiting to finish " << i;
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
#ifdef RUNNING_THREADS
        qDebug() << "RemoveThread: Ended " << i;
#endif
        delete local_thread.value(i);
        local_thread.remove(i);
#ifdef RUNNING_THREADS
        qDebug() << "RemoveThread: Deleted thread " << i;
#endif
    }
}

/*
 * CONFIGURATION METHOD TO RUN THREADED N4D CALL
 * */
void MainWindow::InitN4DCall(QtN4DWorker::Methods method){
    InitN4DCall(method,QStringList());
}

void MainWindow::InitN4DCall(QtN4DWorker::Methods method, QStringList params){
    int i=0;
    QList serials = local_thread.keys();
    for (i=0; i<100; i++){
       if (!serials.contains(i)){
           break;
       }
    }
    local_thread.insert(i,new QThread);
    QtN4DWorker* worker = new QtN4DWorker(i);
    worker->set_auth(n4duser,n4dpwd);
    if (!params.empty()){
        for(auto const& p: params){
            worker->add_param(p.toStdString());
        }
    }
    worker->moveToThread(local_thread.value(i));

    switch(method){
        case QtN4DWorker::Methods::LOGIN:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(validate_user()));
            break;
        case QtN4DWorker::Methods::GET_DATA:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(get_table_data()));
            break;
        case QtN4DWorker::Methods::GET_STATUS:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(get_system_status()));
            break;
        case QtN4DWorker::Methods::GET_CONFIGURED:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(get_configured_status()));
            break;
        case QtN4DWorker::Methods::GET_GOLEM_GROUPS:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(get_golem_groups()));
            break;
        case QtN4DWorker::Methods::ENABLE_SYSTEM:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(enable_system()));
            break;
        case QtN4DWorker::Methods::DISABLE_SYSTEM:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(disable_system()));
            break;
        case QtN4DWorker::Methods::SET_GROUP_QUOTA:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(set_groupquota()));
            break;
        case QtN4DWorker::Methods::SET_USER_QUOTA:
            connect(local_thread.value(i), SIGNAL(started()), worker, SLOT(set_userquota()));
            break;
    };
#ifdef RUNNING_THREADS
    qDebug() << "Connecting signals " << i;
#endif
    connect(worker, SIGNAL(n4d_call_completed(QtN4DWorker::Methods,QString,int)),this, SLOT(ProcessCallback(QtN4DWorker::Methods,QString,int)));
    connect(worker, SIGNAL(finished_thread(int)),local_thread.value(i), SLOT(quit()));
    connect(worker, SIGNAL(finished_thread(int)),worker, SLOT(deleteLater()));
    connect(local_thread.value(i),SIGNAL(finished()),local_thread.value(i),SLOT(deleteLater()));
    connect(worker, SIGNAL(finished_thread(int)),this, SLOT(removeThread(int)));
    local_thread.value(i)->start();
    qDebug() << "Running N4D Call #" << i << " " << method;
    //std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

/*
 * Method to customize pannel styles and related widgets globally
 * */
void MainWindow::ChangePannel(QWidget* pannel){
    static int MAX_HISTORY_ELEMENTS = 3;

    if (pannel == ui->page_login){
        ui->statusBar->showMessage(tr("Ready"),2000);
        ui->logo->setPixmap(QPixmap("://rsrc/banner.png"));
        ui->logo->setAlignment(Qt::AlignCenter);
        ui->actionGroupEditor->setDisabled(true);
        ui->actionQuotaEditor->setDisabled(true);
        ui->logo->show();
        ui->toolBar->hide();
    }
    if (pannel == ui->page_need_configuration){
        ui->logo->hide();
        ui->actionEnable->setEnabled(true);
        ui->actionDisable->setDisabled(true);
        ui->actionGroupEditor->setDisabled(true);
        ui->actionQuotaEditor->setDisabled(true);
        ui->toolBar->show();
    }
    if (pannel == ui->page_group_edit){
        ui->logo->hide();
        ui->actionEnable->setDisabled(true);
        ui->actionDisable->setEnabled(true);
        ui->actionQuotaEditor->setEnabled(true);
        ui->actionGroupEditor->setDisabled(true);
        ui->toolBar->show();
        enable_watch_table[ui->tableGroupEdition] = true;
    }
    if (pannel == ui->page_edit_simple){
        ui->logo->hide();
        ui->actionEnable->setDisabled(true);
        ui->actionDisable->setEnabled(true);
        ui->actionGroupEditor->setEnabled(true);
        ui->actionQuotaEditor->setDisabled(true);
        ui->toolBar->show();
        enable_watch_table[ui->tableUserEdition] = true;
    }
    if (pannel == ui->page_write_changes){
        showConfirmationTable();
        ui->btn_pending_apply->setEnabled(true);
    }

    // Page history
    if (last_page_used.size() == 0){
        last_page_used.push_front(pannel);
    }else{
        if (last_page_used.front() != pannel){
            last_page_used.push_front(pannel);
        }
    }
    if (last_page_used.size() > MAX_HISTORY_ELEMENTS){
        last_page_used.pop_back();
    }
    ui->stackedWidget->setCurrentWidget(pannel);
}

/*
 * CALLBACK FROM VALIDATION N4D CALL
 * */
void MainWindow::CheckValidation(QString result){
    ui->statusBar->showMessage(tr("Checking validation"),5000);
    QString res = result.toLower();
    if (n4dvalidator(result.toStdString(),"array/[bool/true]") && n4dvalidator(result.toStdString(),"array/[array/[string/admins]]")){
        ui->statusBar->showMessage(tr("Validation successful"),5000);
        InitGetGolemGroups();
    }else{
        ui->statusBar->showMessage(tr("Validation failed: ")+QString::fromStdString(n4dresult2json(result.toStdString())),5000);
    }
    ui->txt_usu->setText("");
    ui->txt_pwd->setText("");
}

/*
 * CALLBACK FROM CHECK SYSTEM STATUS
 * */
void MainWindow::CompleteGetStatus(QString result){
    ui->statusBar->showMessage(tr("Refreshed information"),3000);
    if (n4dvalidator(result.toStdString(),"struct/{string/remote:{string/status_serversync:bool/true}}")){
        ui->statusBar->showMessage(tr("System is currently configured"),5000);
        ChangePannel(last_page_used.front());
        InitGetQuotaGroups();
    }else{
        ui->statusBar->showMessage(tr("System is currently unconfigured: ")+QString::fromStdString(n4dresult2json(result.toStdString())),5000);
        ChangePannel(ui->page_need_configuration);
    }
}

/*
 * CALLBACK FROM GET THE GROUP LIST & QUOTAS
 * */
void MainWindow::CompleteGetConfigure(QString result){
    QJsonDocument res = QJsonDocument::fromJson(n4dresult2json(result.toStdString()).data());
    if (!res.isObject()){
        //ui->statusBar->showMessage(tr("Something goes wrong: ")+QString::fromStdString(n4dresult2json(result.toStdString())),5000);
        ui->statusBar->showMessage(tr("Something goes wrong: "),5000);
    }else{
        QJsonObject obj = res.object();
        QMap result = obj.toVariantMap();
        QMap<QString,QStringList>* map;

        // Groups information
        QMap<QString,QVariant> result_groups = result["groups"].toMap();
        QStringList groups = result_groups.keys();
        QRegularExpression regexp1 = QRegularExpression(tr("Quota"),QRegularExpression::CaseInsensitiveOption);
        int header_size1 = modelmap[ui->tableGroupEdition]->value("__HEADER__").size();
        int position1 = modelmap[ui->tableGroupEdition]->value("__HEADER__").indexOf(regexp1);
        map = modelmap.value(ui->tableGroupEdition);
        for (int i=0; i < groups.size(); i++){
            QString value = normalizeUnits(result_groups[groups[i]].toMap()["quota"].toString());
            QStringList tableitem;
            if (map->contains(groups[i])){
                tableitem = map->value(groups[i]);
            }else{
                for (int i=0; i<header_size1; i++){
                    tableitem << "";
                }
            }
            tableitem.replace(position1,value);
            map->insert(groups[i],tableitem);
        }

        // Users information
        QMap<QString,QVariant> result_users = result["users"].toMap();
        QStringList users = result_users.keys();
        QRegularExpression regexp2 = QRegularExpression(tr("Quota"),QRegularExpression::CaseInsensitiveOption);
        int header_size2 = modelmap[ui->tableUserEdition]->value("__HEADER__").size();
        int position2 = modelmap[ui->tableUserEdition]->value("__HEADER__").indexOf(regexp2);
        map = modelmap.value(ui->tableUserEdition);
        for (int i=0; i < users.size(); i++){
            QString value = normalizeUnits(result_users[users[i]].toMap()["quota"].toString());
            QStringList tableitem;
            if (map->contains(users[i])){
                tableitem = map->value(users[i]);
            }else{
                for (int i=0; i<header_size2; i++){
                    tableitem << "0G";
                }
            }
            tableitem.replace(position2,value);
            map->insert(users[i],tableitem);
        }
        InitGetCurrentQuotas();
    }
}

/*
 * CALLBACK FROM GET SYSTEM APPLIED QUOTAS & SPACE USED
 * */
void MainWindow::CompleteGetData(QString result){
    QJsonDocument res = QJsonDocument::fromJson(n4dresult2json(result.toStdString()).data());
    if (!res.isObject()){
        //ui->statusBar->showMessage(tr("Something goes wrong: ")+QString::fromStdString(n4dresult2json(result.toStdString())),5000);
        ui->statusBar->showMessage(tr("Something goes wrong: "),5000);
    }else{
        QJsonObject obj = res.object();
        QMap result = obj.toVariantMap();
        QMap<QString,QStringList>* map;

        map = modelmap[ui->tableUserEdition];
        QRegularExpression regexp_spaceused = QRegularExpression(tr("Used"),QRegularExpression::CaseInsensitiveOption);
        QRegularExpression regexp_hardquota = QRegularExpression(tr("QuotaApplied"),QRegularExpression::CaseInsensitiveOption);
        int position_spaceused = map->value("__HEADER__").indexOf(regexp_spaceused);
        int position_hardquota = map->value("__HEADER__").indexOf(regexp_hardquota);
        int header_size = map->value("__HEADER__").size();
        QStringList keys = result.keys();
        for (auto const& k: keys){
            QMap<QString,QVariant> user_data_quotas = result[k].toMap();
            QString spaceused = user_data_quotas.value("spaceused").toString();
            QString hardlimit = normalizeUnits(user_data_quotas.value("spacehardlimit").toString(),false,true);
            QStringList tableitem;
            if (map->contains(k)){
                tableitem = map->value(k);
            }else{
                for (int i=0; i<header_size; i++){
                    tableitem << "0G";
                }
            }
            tableitem.replace(position_spaceused,spaceused);
            tableitem.replace(position_hardquota,hardlimit);
            map->insert(k,tableitem);
        }
    }
}

/*
 * Callback to store classroom groups
 * */
void MainWindow::StoreGolemGroups(QString returned){
    golem_groups.clear();
    string json = n4dresult2json(returned.toStdString());
    QJsonDocument res = QJsonDocument::fromJson(QString(json.data()).toUtf8());
    if (!res.isArray()){
        ui->statusBar->showMessage(tr("Something goes wrong: ")+QString::fromStdString(n4dresult2json(returned.toStdString())),5000);
    }else{
        QJsonArray arr = res.array();
        for (auto e: arr){
            QJsonObject o = e.toObject();
            if (o["cn"].isArray()){
                golem_groups << o["cn"].toArray().at(0).toString();
            }
        }
        InitCheckStatus();
    }
}

/*
 * CALLBACK TO CHECK THE RESULT WHEN ENABLE SYSTEM QUOTAS
 * */
void MainWindow::SystemIsEnabled(QString result){
    if (n4dvalidator(result.toStdString(),"bool/true")){
        ui->statusBar->showMessage(tr("System quotas are enabled now"),5000);
        InitCheckStatus();
    }else{
        ui->statusBar->showMessage(tr("Something goes wrong"),5000);
    }
}

/*
 * CALLBACK TO CHECK THE RESULT WHEN DISABLE SYSTEM QUOTAS
 * */
void MainWindow::SystemIsDisabled(QString result){
    if (n4dvalidator(result.toStdString(),"bool/true")){
        ui->statusBar->showMessage(tr("System quotas are disabled now"),5000);
        InitCheckStatus();
    }else{
        ui->statusBar->showMessage(tr("Something goes wrong"),5000);
    }
}

/*
 * CALLBACK FROM USER QUOTA MODIFICATION
 * */
void MainWindow::AddedUserQuota(QString result){
    if (result != "bool/true"){
        ui->statusBar->showMessage(tr("Failed to set user quota")+", "+QString::fromStdString(n4dresult2json(result.toStdString())),10000);
    }
    if (!changes_to_apply.empty()){
        PendingApply();
    }
    //qDebug() << "Added user quota with result " << result;
}

/*
 * CALLBACK FROM GROUP QUOTA MODIFICATION
 * */
void MainWindow::AddedGroupQuota(QString result){
    if (result != "bool/true"){
        ui->statusBar->showMessage(tr("Failed to set group quota")+", "+QString::fromStdString(n4dresult2json(result.toStdString())),10000);
    }
    if (!changes_to_apply.empty()){
        PendingApply();
    }
    //qDebug() << "Added group quota with result " << result;
}

/*
 * GENERIC METHOD FOR TABLE INITIALIZATION
 * */
void MainWindow::InitializeTable(QTableWidget* item){
    // remove all items
    QAbstractItemModel* m = item->model();

    m->removeRows(0,m->rowCount());
    m->removeColumns(0,m->columnCount());
}

/*
 * MAKE READ ONLY TABLE
 * */
void MainWindow::makeReadOnlyTable(QTableWidget* table){
    // Make read only
    for (int col=0; col < table->columnCount(); col++){
        for (int row=0; row < table->rowCount(); row++){
            QTableWidgetItem* item = table->item(row,col);
            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        }
    }
}

/*
 * METHOD TO WRITE CONTENT INTO TABLES
 * */
void MainWindow::PopulateTableWithFilters(QTableWidget* table, QStringList showfilter, QStringList filter, QLineEdit* txtfilter, bool overwrite=false){
    QStringList filter_remove;
    QStringList filter_show;

    filter_show = showfilter;
    filter_remove = filter;

    QMap<QString,QStringList> content;
    if (overwrite){
        content = *(modelmap.value(table));
    }else{
        content = readViewTable(table);
        if (content.isEmpty()){
            return;
        }
    }

    if (content.isEmpty()){
        for (auto const& k: modelmap[table]->keys()){
            if (! k.contains(txtfilter->text(),Qt::CaseInsensitive)){
                filter_remove << k;
            }
        }
        pending_changes[table]=false;
        PopulateTable(modelmap.value(table),table,filter_show,filter_remove);
        checkApplyButtons();
    }else{
        if (overwrite){
            pending_changes[table]=false;
            txtfilter->setText("");
            PopulateTable(&content,table,filter_show,filter_remove);
        }else{
            QMap<QString,QStringList> diff = getTableDifferences(modelmap.value(table),&content);
            if (diff.isEmpty()){
                pending_changes[table] = false;
            }else{
                pending_changes[table] = true;
            }
            QMap<QString,QStringList> merged = ApplyChangesToModel(modelmap.value(table),&diff);
            for (auto const& k: merged.keys()){
                if (! k.contains(txtfilter->text(),Qt::CaseInsensitive)){
                    filter_remove << k;
                }
            }
            PopulateTable(&merged,table,filter_show,filter_remove);
        }
        checkApplyButtons();
    }
}

/*
 * METHOD TO COMPLETE TABLE WITH MAP DATA
 * */
void MainWindow::PopulateTable(QMap<QString,QStringList>* data, QTableWidget* table){
    PopulateTable(data,table,QStringList(),QStringList());
}

/*
 * METHOD TO COMPLETE TABLE WITH MAP DATA
 * */
void MainWindow::PopulateTable(QMap<QString,QStringList>* data, QTableWidget* table, QStringList showfilter, QStringList filter){
    InitializeTable(table);
    enable_watch_table[table] = false;
    table->setColumnCount(data->value("__HEADER__").size());
    table->setHorizontalHeaderLabels(data->value("__HEADER__"));
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QAbstractItemModel* m;
    m = table->model();
    QStringList keys = QStringList(data->keys());
    QSet<QString> keys_to_hide;

    keys.sort(Qt::CaseInsensitive);

    if (! filter.contains("__HEADER__")){
        filter << "__HEADER__";
    }
    if (!filter.isEmpty()){
        for (auto const& i: keys){
            if (filter.contains(i,Qt::CaseInsensitive)){
                keys_to_hide.insert(i);
            }
        }
    }
    if (!showfilter.isEmpty()){
        for (auto const& i: keys){
            if (! showfilter.contains(i,Qt::CaseInsensitive)){
                keys_to_hide.insert(i);
            }
        }
    }
    while (! keys.isEmpty()){
        QString k = keys.takeLast();
        m->insertRow(0);
        table->setVerticalHeaderItem(0,new QTableWidgetItem(k));
        unsigned int i = 0;
        for (auto const& j: data->value(k)){
            table->setItem(0,i,new QTableWidgetItem(j));
            table->item(0,i)->setTextAlignment(Qt::AlignCenter);
            i++;
        }
        if (keys_to_hide.contains(k)){
            table->hideRow(0);
        }      
    }

    // Make non editable columns
    for (int col=0; col < table->columnCount(); col++){
        if (non_editable_columns[table].contains(table->horizontalHeaderItem(col)->text())){
            for (int row=0 ; row < table->rowCount(); row++){
                QTableWidgetItem *item = table->item(row,col);
                item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                item->setBackground(QBrush(ui->centralWidget->palette().color(QWidget::backgroundRole())));
            }
        }
    }
    enable_watch_table[table]=true;
}

/*
 * METHOD TO READ THE TABLE VIEW
 * */
QMap<QString,QStringList> MainWindow::readViewTable(QTableWidget* table){
    QMap<QString,QStringList> map;
    QStringList header;
    for (int col=0; col < table->columnCount(); col++){
        header << table->horizontalHeaderItem(col)->text();
    }
    if (!header.isEmpty()){
        map.insert("__HEADER__",header);
    }
    for (int row=0; row < table->rowCount(); row++){
        QString key = table->verticalHeaderItem(row)->text();
        QStringList rowList;
        for (int col=0; col < table->columnCount(); col++){
            rowList << table->item(row,col)->text();
        }
        map.insert(key,rowList);
    }
    return map;
}

/*
 * METHOD TO GET TABLE DIFFERENCES td1 (base table), td2 (table to check)
 * */
QMap<QString,QStringList> MainWindow::getTableDifferences(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2){
    QStringList keys1 = td1->keys();
    QStringList keys2 = td2->keys();
    QMap<QString,QStringList> differences;
    QStringList keys_to_compare;

    for(auto const& k2: keys2){
        if (keys1.contains(k2)){
            keys_to_compare << k2;
        }else{
            differences.insert(k2,td2->value(k2));
        }
    }
    for (auto const& k1: keys_to_compare){
        if (td1->value(k1).size() != td2->value(k1).size()){
            differences.insert(k1,td2->value(k1));
        }else{
            for(int i=0; i< td2->value(k1).size(); i++){
                if (td1->value(k1).value(i) != td2->value(k1).value(i)){
                    differences.insert(k1,td2->value(k1));
                    break;
                }
            }
        }
    }
    return differences;
}

/*
 * BUILD COMPLETE TABLE MODEL FROM DIFERECES BETWEEN TWO MODELS
 * */
QMap<QString,QStringList> MainWindow::getTableDifferencesWithHeader(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2){
      QMap<QString,QStringList> tablemodel;
      tablemodel = getTableDifferences(td1,td2);
      tablemodel.insert("__HEADER__",td2->value("__HEADER__"));
      return tablemodel;
}

/*
 * Apply Changes to table model
 * */
QMap<QString,QStringList> MainWindow::ApplyChangesToModel(QMap<QString,QStringList>* model,QMap<QString,QStringList>* changes){
    QMap<QString,QStringList> map;
    QStringList k1 = model->keys();
    QStringList k2 = changes->keys();
    for (auto const& i: k2){
        map.insert(i,changes->value(i));
        k1.removeOne(i);
    }
    for (auto const& i: k1){
        map.insert(i,model->value(i));
    }
    return map;
}

/*
 * Unify distinct table models
 * */
bool MainWindow::MakeSameCols(QMap<QString,QStringList> &model,QMap<QString,QStringList> &changes){
    if (!model.contains("__HEADER__") || ! changes.contains("__HEADER__")){
        return false;
    }
    QStringList k_model = model.value("__HEADER__");
    QStringList k_changes = changes.value("__HEADER__");

    QStringList k_common;
    QList<int> removeIndex;

    for (auto const& k: k_model){
        if (k_changes.contains(k)){
            k_common << k;
        }
    }
    for (int i = 0; i < k_changes.size(); i++){
        if (! k_common.contains(k_changes.at(i))){
            removeIndex.append(i);
        }
    }
    std::sort(removeIndex.begin(),removeIndex.end(), std::greater<int>());
    for(auto const& k: changes.keys()){
        for (auto const& i: removeIndex){
            changes[k].removeAt(i);
        }
    }
    return true;
}

/*
 * METHOD TO CHECK IF ONE TABLE IS MODIFIED td1 (base table), td2 (table to check)
 * */
bool MainWindow::isModified(QMap<QString,QStringList>* td1,QMap<QString,QStringList>* td2){
    QMap<QString,QStringList> result = getTableDifferences(td1,td2);
    if (result.isEmpty()){
        return false;
    }else{
        //qDebug() << result;
        return true;
    }
}

/*
 * SHOW PENDING CHANGES FOR GROUP ACTIONS
 * */
void MainWindow::showConfirmationTable(){
    enable_watch_table[ui->table_pending] = false;
    changes_to_apply.clear();
    QMap<QString,QMap<QString,QStringList>> contents;
    QMap<QString,QMap<QString,QStringList>> oldcontents;

    QMap<QString,QStringList> contents_table;
    QMap<QString,QMap<QString,QStringList>> diff;
    QMap<QString,QMap<QString,QStringList>> old;
    QString type_modification;

    for (auto const& table: tablewidgets){
        if (table == ui->table_pending){
            continue;
        }
        if (table == ui->tableGroupEdition){
            type_modification = "Group";
        }else{
            type_modification = "User";
        }
        // THE ONLY COLUMN THAT MUST APPEAR ON CONFIRMATION TABLE FROM MODEL!
        // MakeSameColumns uses common columns to remove from second table uncommon columns
        QMap<QString,QStringList> c;
        c.insert("__HEADER__",QStringList({tr("Quota")}));
        contents.insert(type_modification,c);
        QMap<QString,QStringList> oc;
        oc.insert("__HEADER__",QStringList({tr("Quota")}));
        oldcontents.insert(type_modification,oc);

        contents_table = readViewTable(table);
        diff.insert(type_modification,getTableDifferencesWithHeader(modelmap[table],&contents_table));
        old.insert(type_modification,getTableDifferencesWithHeader(&contents_table,modelmap[table]));

        if (diff[type_modification].size() > 1){
            if (MakeSameCols(contents[type_modification],diff[type_modification])){
                contents[type_modification] = ApplyChangesToModel(&contents[type_modification],&diff[type_modification]);
                MakeSameCols(oldcontents[type_modification],old[type_modification]); // old Value uses "Quota" field
                oldcontents[type_modification] = ApplyChangesToModel(&oldcontents[type_modification], &old[type_modification]);
            }
        }
    }
    QMap<QString,QStringList> show_final_contents;
    // Mix old value into table & put type of modification
    // structure to show changes (Need to be unique keys, if not unique when user & group have the same name will remove one of both on table)
    show_final_contents["__HEADER__"] << tr("Quota");
    show_final_contents["__HEADER__"] << tr("Previous value");

    for (auto const& type: contents.keys()){
        for (auto const& key: oldcontents[type].keys()){
            if (key != "__HEADER__"){
                QStringList row;
                row << contents[type][key];
                row << oldcontents[type][key];
                show_final_contents.insert(type+" "+key,row);
                QStringList store_list;
                store_list.append(key);
                store_list.append(type);
                store_list.append(oldcontents[type][key]);
                store_list.append(contents[type][key]);
                changes_to_apply.append(store_list);
            }
        }
    }
    PopulateTable(&show_final_contents,ui->table_pending);
    makeReadOnlyTable(ui->table_pending);
}

/*
 * Change units to integer values when send to n4d system
 * */
QString MainWindow::denormalizeUnits(QString value){
    QString out;
    QRegularExpression number_with_unit("^(\\d+[.]?\\d*)[ ]*([gGmM])$");
    QRegularExpression number("^(\\d+[.]?\\d*)$");
    QRegularExpressionMatch match;

    match = number_with_unit.match(value);
    if (match.hasMatch()){
        QString numeric_value = match.captured(1);
        QString unit_value = match.captured(2);
        if (unit_value == "0.0" || unit_value == "0"){
            out = "0";
            qDebug() << "Denormalizing " << value << " to " << out;
            return out;
        }
        if (unit_value.toUpper() == "G"){ // "G" as unit
            if (numeric_value.contains(".")){
                numeric_value = QString::number((numeric_value.toFloat() * 1024),10,0);
                out = numeric_value + "m";
            }else{
                out = QString::number(numeric_value.toFloat() * 1024,10,0) + "m";
            }
        }else{ // "M" as unit
            if (numeric_value.contains(".")){
                numeric_value = QString::number(numeric_value.toFloat(),10,0);
            }else{
                numeric_value = QString::number(numeric_value.toInt(),10,0);
                out = numeric_value + "m";
            }
        }
    }else{
        match = number.match(value);
        if (match.hasMatch()){
            QString numeric_value = match.captured(1);
            numeric_value = QString::number(numeric_value.toFloat(),10,0);
            out = numeric_value + "k";
        }else{
            out = "0";
        }
    }
    qDebug() << "Denormalizing " << value << " to " << out;
    return out;
}

/*
 * NORMALIZE QUOTA VALUE
 * */
QString MainWindow::normalizeUnits(QString value, bool conversion, bool fromHumanUnits){
    QString out;
    /* DEPRECATED 
    QRegularExpression number_with_unit("^(\\d+[.]?\\d*)[ ]*([gGmM])$");
    QRegularExpression number("^(\\d+[.]?\\d*)$");
    */
    
    QRegularExpression integer("^(\\d+)[ gG]*");
    QRegularExpressionMatch match;
/*  new case to allow normalize "quota applied" */
    if (fromHumanUnits){
	//qDebug() << "Normalizing 'Quota applied' from callback getConfigured (hardquota case)";
	QRegularExpression number_with_unit("^(\\d+[.]?\\d*)[ ]*([gGmMkK])$");
	match = number_with_unit.match(value);
	if (match.hasMatch()){
	    QString value_numeric = match.captured(1);
	    QString value_unit = match.captured(2);
	    if (value_unit.toUpper() == "G"){
		    out = value_numeric;
	    }
	    if (value_unit.toUpper() == "M"){
		    out = QString::number(value_numeric.toFloat()/1024,10,0);
	    }
	    if (value_unit.toUpper() == "K"){
		    out = QString::number(value_numeric.toFloat()/1024/1024,10,0);
	    }
	    out = out + "G";
	}
    }else{
/* default case: normalize data adding unit if user modify cell data (without conversion)
   or normalize units with conversion if its n4d returned data from getconfigured */
	
        match = integer.match(value);
        if (match.hasMatch()){
             QString value_numeric = match.captured(1);
	     if (conversion){
		  //qDebug() << "Normalizing from user cell modification";
	          value_numeric = QString::number(value_numeric.toFloat()/1024/1024,10,0);
	     }
	     //qDebug() << "Normalizing from callback getConfigured";
             out = value_numeric + "G";
	}else{
             out = "0G";
	}
    }
    //qDebug() << "Normalizing " << value << " to " << out;
    return out;
    
    /* DEPRECATED 
    match = number_with_unit.match(value);
    if (match.hasMatch()){
        QString numeric_value = match.captured(1);
        QString unit_value = match.captured(2);
        if (unit_value.toUpper() == "G"){ // "G" as unit
            out = numeric_value + "G";
        }else{ // "M" as unit
            numeric_value = QString::number(numeric_value.toFloat() / 1024,10,2);
            out = numeric_value + "G";
        }
    }else{
        match = number.match(value);
        if (match.hasMatch()){
            QString numeric_value = match.captured(1);
            float numeric_float = numeric_value.toFloat();

            if (numeric_float >= 1024){ // Assume "K" as unit (native mode)
                numeric_value = QString::number(numeric_float/(1024*1024),10,2);
            }else{
                if (numeric_float <= 20){ // Assume "G" as unit
                    numeric_value = QString::number(numeric_float,10,2);
                }else{
                    if (numeric_float > 20 && numeric_float < 1024){ // Assume "M" as unit
                        numeric_value = QString::number(numeric_float/1024,10,2);
                    }
                }
            }
            out = numeric_value + "G";
        }else{
            out = "0";
        }
    }
    return out;
    */
}

/*
 * CHECK VALID QUOTA VALUE, VALIDATES USER INPUT WHEN CHANGES ANY COLUMN
 * */
bool MainWindow::isValidQuotaValue(QString value){
//    QRegularExpression number_with_unit("^\\d+[.]?\\d*[ ]*[gGmM]$");
//    QRegularExpression number("^\\d+[.]?\\d*$");

//  New accepted values: only G as unit and integer values
    QRegularExpression integer("^(\\d+)[ gG]*");
    QRegularExpressionMatch match;

    match = integer.match(value);
    if (match.hasMatch()){
        int value_numeric = match.captured(1).toInt();
        if (value_numeric >= 0 && value_numeric < 1000){
            return true;
        }
    }
    return false;
    
/*  Deprecated: allow real numbers with or without unit

    match = number_with_unit.match(value);
    if (match.hasMatch()){
        return true;
    }else{
        match = number.match(value);
        if (match.hasMatch()){
            return true;
        }else{
            return false;
        }
    }
*/
}

/*
 * DO ACTIONS WHEN CHANGED TABLE CELLS SLOTS ARE TRIGGERED
 * */
void MainWindow::CellChanged(int row, int col, QTableWidget* table){
    if (enable_watch_table.value(table)){
        QString value = table->item(row,col)->text();
        QString key = table->verticalHeaderItem(row)->text();
        if (isValidQuotaValue(value)){
            value = normalizeUnits(value,false);
        }else{
            value = modelmap[table]->value(key).at(col);
            ui->statusBar->showMessage(tr("Invalid value"),5000);
        }
        enable_watch_table[table]=false;
        table->item(row,col)->setText(value);
        enable_watch_table[table]=true;
        QMap<QString,QStringList> contents = readViewTable(table);
        if (isModified(modelmap[table],&contents)){
            pending_changes[table]=true;
        }else{
            pending_changes[table]=false;
        }
        checkApplyButtons();
    }
}

/*
 * ENABLE/DISABLE APPLY BUTTON
 * */
bool MainWindow::checkApplyButtons(){
    bool any=false;
    if (pending_changes[ui->tableGroupEdition]){
        any=true;
        ui->undoChangesButtonGroupTable->setEnabled(true);
    }else{
        ui->undoChangesButtonGroupTable->setDisabled(true);
    }
    if (pending_changes[ui->tableUserEdition]){
        any=true;
        ui->undoChangesUserTable->setEnabled(true);
    }else{
        ui->undoChangesUserTable->setDisabled(true);
    }
    if (any){
        ui->applyButtonGroupTable->setEnabled(true);
        ui->applyButtonUserTable->setEnabled(true);
    }else{
        ui->applyButtonGroupTable->setDisabled(true);
        ui->applyButtonUserTable->setDisabled(true);
    }
    return any;
}
/*
 * OLD CODE
 *
 * */
/*
void MainWindow::CompletePopulate(QString returned){
     map<string,UserData> table;
     ui->statusBar->showMessage(tr("Completing table populate !"),5000);
     model->LoadUsersFromString(returned.toStdString());
     table = model->getTableData();

     //InitializeTable();
     QStringList non_editable_items = {"name","spaceused","fileused","spacestatus","filestatus"};
     vector<int> non_editable;
     for (int i=0; i< ui->table_quotas->columnCount(); i++){
         QString label = ui->table_quotas->horizontalHeaderItem(i)->text();
         if (non_editable_items.contains(label)){
             non_editable.push_back(i);
             continue;
         }
     }

     unsigned int i=0;
     for (auto const& [key,value]: table){
         ui->table_quotas->model()->insertRow(ui->table_quotas->model()->rowCount());

         UserData data = value;
         string k=key;

         unsigned int j=0;
         QString *pstr;
         for (auto const& name : data.field_names){
            string item = data.getField(name);
            pstr = new QString(QString::fromStdString(item));
            setCellData(i,j++,pstr);
         }
         i++;
     }

     for (int i=0 ; i < ui->table_quotas->rowCount(); i++){
         for (auto const& ncol: non_editable){
             QTableWidgetItem *item = ui->table_quotas->item(i,ncol);
             item->setFlags(item->flags() ^ Qt::ItemIsEditable );
         }
     }
}





void MainWindow::cell_action(int row, int col){
    qDebug() << "Cell " << col << "," << row << " clicked action !";
    QTableWidgetItem *cell = ui->table_quotas->item(row,col);
    if (cell->flags() & Qt::ItemIsEditable){
        cell->setBackgroundColor(QColor(Qt::gray));
    }
}
void MainWindow::cell_changed(int row, int col){
   qDebug() << "Cell " << col << "," << row << " edited!";// << " from " << older->text() << " to " << newer->text() ;
}



void MainWindow::apply_table_to_model(){
    QStringList col_names;
    bool need_update=false;
    for(int row=0; row<ui->table_quotas->rowCount(); row++){
        UserData x;
        bool current_modified=false;
        if (row == 0){
            for(int col=0; col<ui->table_quotas->columnCount(); col++ ){
                col_names << ui->table_quotas->horizontalHeaderItem(col)->text();
            }
        }
        for(int col=0; col<ui->table_quotas->columnCount(); col++ ){
            x.setField(col_names[col].toStdString(),ui->table_quotas->item(row,col)->text().toStdString());
        }
        if (model->getUser(x.getField("name")) != x){
            qDebug() << "User " << QString::fromStdString(x.getField("name")) << " was modified!";
            need_update = true;
            current_modified = true;
        }

        QColor c;
        if (current_modified){
            c = QColor(Qt::red);
        }else{
            c = QColor(Qt::white);
        }
        for(int col=0; col<ui->table_quotas->columnCount(); col++ ){
            QTableWidgetItem *cell = ui->table_quotas->item(row,col);
            cell->setBackgroundColor(c);
        }
    }
    if (!need_update){
        qDebug() << "Table not modified :-)";
    }
}
*/
/*QString MainWindow::getCellData(int x, int y){
    QAbstractItemModel *model = ui->table_quotas->model();
    QModelIndex idx = model->index(x,y);
    return idx.data().toString();
}*/

/*void MainWindow::setCellData(int x, int y, QString *str){
    //qDebug() << "Cell " << x << "," << y << " changed from " << getCellData(x,y) << " to " << *str ;
    ui->table_quotas->setItem(x,y,new QTableWidgetItem(*str));
    return ;
}*/

//model = new DataModel();

//ui->table_quotas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//ui->logo->setPixmap(QPixmap(QString("/home/lliurex/banner.png")));
//ui->configure_status_img->setPixmap(QPixmap("://unconfigured.png"));

//connect(ui->table_quotas, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cell_action(int,int)));
//connect(ui->table_quotas, SIGNAL(cellActivated(int,int)), this, SLOT(cell_changed(int,int)));
