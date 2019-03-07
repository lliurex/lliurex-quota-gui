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

#include <algorithm>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tablewidgets = {ui->tableGroupEdition,ui->table_pending,ui->tableUserEdition};
    for (auto const& k: tablewidgets){
        enable_watch_table.insert(k,false);
        pending_changes.insert(k,false);
    }

    ChangePannel(ui->page_login);
}

MainWindow::~MainWindow()
{
    delete ui;
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
    qDebug() << "Result from N4D:" << from;
    qDebug() << returned;
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
 * SLOT THAT START TO GET CLASSROOM GROUPS
 * */
void MainWindow::InitGetGolemGroups(){
    InitN4DCall(QtN4DWorker::Methods::GET_GOLEM_GROUPS);
}

/*
 * SLOT THAT START TO GET USERS QUOTA
 * */
//void MainWindow::InitPopulateTable(){
//    InitN4DCall(QtN4DWorker::Methods::GET_DATA);
//}

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

/*************************************
 *  END SLOTS
 ************************************/

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
 * Method to customize pannel styles and related widgets globally
 * */
void MainWindow::ChangePannel(QWidget* pannel){
    static int MAX_HISTORY_ELEMENTS = 10;

    if (pannel == ui->page_login){
        ui->statusBar->showMessage(tr("Ready"),2000);
        ui->logo->setPixmap(QPixmap("://banner.png"));
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
    }

    // Page history
    last_page_used.push_front(pannel);
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
    qDebug() << res;
    if (res.isObject()){
        QJsonObject obj = res.object();
        QMap result = obj.toVariantMap();
        QMap<QString,QStringList>* map;
        QStringList headers;
        // Groups information
        QMap<QString,QVariant> result_groups = result["groups"].toMap();
        QStringList groups = result_groups.keys();
        map = new QMap<QString,QStringList>;
        headers = QStringList::fromStdList({ "Quota" });
        map->insert("__HEADER__",headers);
        for (int i=0; i < groups.size(); i++){
            QStringList tableitem;
            tableitem << normalizeUnits(result_groups[groups[i]].toMap()["quota"].toString());
            map->insert(groups[i],tableitem);
        }
        modelmap.insert(ui->tableGroupEdition,map);
        // Users information
        QMap<QString,QVariant> result_users = result["users"].toMap();
        QStringList users = result_users.keys();
        map = new QMap<QString,QStringList>;
        headers = QStringList::fromStdList({ "Quota" });
        map->insert("__HEADER__",headers);
        for (int i=0; i < users.size(); i++){
            QStringList tableitem;
            tableitem << normalizeUnits(result_users[users[i]].toMap()["quota"].toString());
            map->insert(users[i],tableitem);
        }
        modelmap.insert(ui->tableUserEdition,map);

        ResetGroupTable();
        ResetUserTable();
    }
}

/*
 * Callback to store classroom groups
 * */
void MainWindow::StoreGolemGroups(QString returned){
    golem_groups.clear();
    string json = n4dresult2json(returned.toStdString());
    QJsonDocument res = QJsonDocument::fromJson(QString(json.data()).toUtf8());
    if (res.isArray()){
        QJsonArray arr = res.array();
        for (auto e: arr){
            QJsonObject o = e.toObject();
            if (o["cn"].isArray()){
                golem_groups << o["cn"].toArray().at(0).toString();
            }
        }
    }
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
 *
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
    QMap<QString,QStringList> contents;
    QMap<QString,QStringList> contents_table;
    for (auto const& table: tablewidgets){
        if (table == ui->table_pending){
            continue;
        }
        contents_table = readViewTable(table);
        QMap<QString,QStringList> diff = getTableDifferencesWithHeader(modelmap[table],&contents_table);
        if (diff.size() > 1){
            contents = ApplyChangesToModel(&contents,&diff);
        }
    }
    PopulateTable(&contents,ui->table_pending);
    makeReadOnlyTable(ui->table_pending);
}

/*
 * NORMALIZE QUOTA VALUE
 * */
QString MainWindow::normalizeUnits(QString value){
    QString out;
    QRegularExpression number_with_unit("^(\\d+[.]?\\d*)[ ]*([gGmM])$");
    QRegularExpression number("^(\\d+[.]?\\d*)$");
    QRegularExpressionMatch match;

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
}

/*
 * CHECK VALID QUOTA VALUE, VALIDATES USER INPUT WHEN CHANGES ANY COLUMN
 * */
bool MainWindow::isValidQuotaValue(QString value){
    QRegularExpression number_with_unit("^\\d+[.]?\\d*[ ]*[gGmM]$");
    QRegularExpression number("^\\d+[.]?\\d*$");
    QRegularExpressionMatch match;

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

}

/*
 * DO ACTIONS WHEN CHANGED TABLE CELLS SLOTS ARE TRIGGERED
 * */
void MainWindow::CellChanged(int row, int col, QTableWidget* table){
    if (enable_watch_table.value(table)){
        QString value = table->item(row,col)->text();
        QString key = table->verticalHeaderItem(row)->text();
        if (isValidQuotaValue(value)){
            value = normalizeUnits(value);
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
    if (pending_changes[ui->tableGroupEdition] || pending_changes[ui->tableUserEdition]){
        ui->applyButtonGroupTable->setEnabled(true);
        ui->applyButtonUserTable->setEnabled(true);
        return true;
    }else{
        ui->applyButtonGroupTable->setDisabled(true);
        ui->applyButtonUserTable->setDisabled(true);
        return false;
    }
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
