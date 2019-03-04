#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "qgraphicsproxywidget.h"
#include "qdebug.h"
#include "n4d.cpp"
//#include "datamodel.h"

#include <QThread>
#include <QStandardItemModel>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ChangePannel(ui->page_login);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete map_group_info;
}

/*
 * CONFIGURATION METHOD TO RUN THREADED N4D CALL
 * */
void MainWindow::InitN4DCall(QtN4DWorker::Methods method){

    local_thread = new QThread;
    QtN4DWorker* worker = new QtN4DWorker();
    worker->set_auth(n4duser,n4dpwd);
    worker->moveToThread(local_thread);

    switch(method){
        case QtN4DWorker::Methods::LOGIN:
            connect(local_thread, SIGNAL(started()), worker, SLOT(validate_user()));
            break;
        case QtN4DWorker::Methods::GET_DATA:
            connect(local_thread, SIGNAL(started()), worker, SLOT(get_table_data()));
            break;
        case QtN4DWorker::Methods::GET_STATUS:
            connect(local_thread, SIGNAL(started()), worker, SLOT(get_system_status()));
            break;
        case QtN4DWorker::Methods::GET_CONFIGURED:
            connect(local_thread, SIGNAL(started()), worker, SLOT(get_configured_status()));
            break;
        case QtN4DWorker::Methods::GET_GOLEM_GROUPS:
            connect(local_thread, SIGNAL(started()), worker, SLOT(get_golem_groups()));
            break;
    };

    connect(worker, SIGNAL(n4d_call_completed(QtN4DWorker::Methods,QString)),this, SLOT(ProcessCallback(QtN4DWorker::Methods,QString)));
    local_thread->start();
    qDebug() << "Running N4D Call " << method;
}

/*
 * COMMON CALLBACK FROM N4D CALL
 * */
void MainWindow::ProcessCallback(QtN4DWorker::Methods from, QString returned){
    switch (from){
        case QtN4DWorker::Methods::LOGIN:
            CheckValidation(returned);
            break;
        case QtN4DWorker::Methods::GET_DATA:
//            CompletePopulate(returned);
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
    };
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
 * SLOT THAT BEGIN TO GET SYSTEM STATUS
 * */
void MainWindow::InitCheckStatus(){
    InitN4DCall(QtN4DWorker::Methods::GET_STATUS);
}

/*
 * SLOT THAT START TO GET GROUP LIST & STATUS
 * */
void MainWindow::InitGetQuotaGroups(){
    InitN4DCall(QtN4DWorker::Methods::GET_CONFIGURED);
}

/*
 * SLOT THAT START TO GET USERS QUOTA
 * */
//void MainWindow::InitPopulateTable(){
//    InitN4DCall(QtN4DWorker::Methods::GET_DATA);
//}

/*
 * SLOT THAT START TO GET CLASSROOM GROUPS
 * */
void MainWindow::InitGetGolemGroups(){
    InitN4DCall(QtN4DWorker::Methods::GET_GOLEM_GROUPS);
}

/*
 * Method to customize pannel styles and related widgets globally
 * */
void MainWindow::ChangePannel(QWidget* pannel){
    ui->stackedWidget->setCurrentWidget(pannel);

    if (pannel == ui->page_login){
        ui->statusBar->showMessage(tr("Ready"),2000);
        ui->logo->setPixmap(QPixmap("://banner.png"));
        ui->logo->setAlignment(Qt::AlignCenter);
        ui->logo->show();
        ui->toolBar->hide();
    }
    if (pannel == ui->page_need_configuration){
        ui->logo->hide();
        ui->actionEnable->setEnabled(true);
        ui->actionDisable->setDisabled(true);
        ui->actionQuotaEditor->setDisabled(true);
        ui->toolBar->show();
    }
    if (pannel == ui->page_group_edit){
        ui->logo->hide();
        ui->actionEnable->setDisabled(true);
        ui->actionDisable->setEnabled(true);
        ui->actionQuotaEditor->setEnabled(true);
        ui->toolBar->show();
    }
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
        ui->statusBar->showMessage(tr("Validation failed"),5000);
    }
    ui->txt_usu->setText("");
    ui->txt_pwd->setText("");
}

/*
 * CALLBACK FROM CHECK SYSTEM STATUS
 * */
void MainWindow::CompleteGetStatus(QString result){
    ui->statusBar->showMessage(tr("Completing get status"),5000);
    if (n4dvalidator(result.toStdString(),"struct/{string/remote:{string/status_serversync:bool/true}}")){
        ui->statusBar->showMessage(tr("System is currently configured"),5000);
        ChangePannel(ui->page_group_edit);
        InitGetQuotaGroups();
    }else{
        ui->statusBar->showMessage(tr("System is currently unconfigured"),5000);
        ChangePannel(ui->page_need_configuration);
    }
}

/*
 * CALLBACK FROM GET THE GROUP LIST & QUOTAS
 * */
void MainWindow::CompleteGetConfigure(QString result){
    QJsonDocument res = QJsonDocument::fromJson(n4dresult2json(result.toStdString()).data());
    if (res.isObject()){
        QJsonObject obj = res.object();
        QMap result = obj.toVariantMap();
        result = result["groups"].toMap();
        QStringList groups = result.keys();
        QMap<QString,QStringList>* map = new QMap<QString,QStringList>;
        QStringList headers = { "Quota" };
        map->insert("__HEADER__",headers);
        for (int i=0; i < groups.size(); i++){
            QStringList tableitem;
            tableitem << result[groups[i]].toMap()["quota"].toString();
            map->insert(groups[i],tableitem);
        }
        map_group_info = map;
        PopulateGroupTableWithFilter();
    }
}


/*
 * Callback to store classroom groups
 * */
void MainWindow::StoreGolemGroups(QString returned){
    qDebug() << returned;
    InitCheckStatus();
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
 * SLOT TO SET GROUP FILTER FROM CHECKBOX
 * */
void MainWindow::PopulateGroupTableWithFilter(){
    QStringList filter_remove;
    QStringList filter_show;

    if (ui->check_show_all_groups->isChecked()){
        filter_show.clear();
    }else{
        filter_show = QStringList::fromStdList({"aluS","pRos","students","teachers","admins"});
    }
    filter_remove.clear();
    QMap<QString,QStringList> content = readViewTable(ui->tableGroupEdition);
    if (content.isEmpty()){
        for (auto const& k: map_group_info->keys()){
            if (! k.contains(ui->txt_filter_group_table->text(),Qt::CaseInsensitive)){
                filter_remove << k;
            }
        }
        PopulateTable(map_group_info,ui->tableGroupEdition,filter_show,filter_remove);
    }else{
        QMap<QString,QStringList> diff = getTableDifferences(map_group_info,&content);
        QMap<QString,QStringList> merged = ApplyChangesToModel(map_group_info,&diff);
        for (auto const& k: merged.keys()){
            if (! k.contains(ui->txt_filter_group_table->text(),Qt::CaseInsensitive)){
                filter_remove << k;
            }
        }
        PopulateTable(&merged,ui->tableGroupEdition,filter_show,filter_remove);
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
 * SLOT FOR CHECKING GROUP TABLE DIFFERENCES
 * */
void MainWindow::CheckGroupTableDifferences(){
    QMap<QString,QStringList> result = readViewTable(ui->tableGroupEdition);
    if (isModified(map_group_info,&result)){
        qDebug() << "Distinct Table";
    }else{
        qDebug() << "Same Table";
    }
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
