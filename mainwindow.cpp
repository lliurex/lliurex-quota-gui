#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qgraphicsproxywidget.h"
#include "qdebug.h"
#include "n4d.cpp"
#include "datamodel.h"
#include <QThread>
#include <QStandardItemModel>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    model = new DataModel();
    ui->stackedWidget->setCurrentIndex(0);
    ui->table_quotas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->logo->setPixmap(QPixmap(QString("/home/lliurex/banner.png")));
    ui->logo->setAlignment(Qt::AlignCenter);

//    qDebug() << getCellData(1,1);
//    setCellData(1,1,new QString("Hello"));
//    qDebug() << getCellData(1,1);

    connect(ui->table_quotas, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cell_action(int,int)));
    connect(ui->table_quotas, SIGNAL(cellActivated(int,int)), this, SLOT(cell_changed(int,int)));
}

void MainWindow::CheckValidation(QString result){
    qDebug() << "Checking validation" << result;
    QString res = result.toLower();
    if (res.contains("adm") and res.contains("admins")){
        ui->stackedWidget->setCurrentIndex(1);
        qDebug() << "Validation successful";
        InitPopulateTable();
    }else{
        qDebug() << "Validation failed";
    }
    ui->txt_usu->setText("");
    ui->txt_pwd->setText("");
}

void MainWindow::InitPopulateTable(){
    InitN4DCall(QtN4DWorker::Methods::GET_DATA);
}

void MainWindow::ProcessCallback(QtN4DWorker::Methods from, QString returned){
    switch (from){
        case QtN4DWorker::Methods::LOGIN:
            CheckValidation(returned);
            break;
        case QtN4DWorker::Methods::GET_DATA:
            CompletePopulate(returned);
            break;
    };
}

void MainWindow::InitializeTable(){
    //remove all rows & columns
    ui->table_quotas->model()->removeRows(0,ui->table_quotas->model()->rowCount());
    ui->table_quotas->model()->removeColumns(0,ui->table_quotas->model()->columnCount());

    UserData x;
    QStringList headers;

    for (auto const& name: x.field_names){
        ui->table_quotas->model()->insertColumn(0);
        headers << QString::fromStdString(name);
    }

    ui->table_quotas->setHorizontalHeaderLabels(headers);
}

void MainWindow::CompletePopulate(QString returned){
     map<string,UserData> table;
     qDebug() << "Completing table populate !";
     model->LoadUsersFromString(returned.toStdString());
     table = model->getTableData();

     InitializeTable();
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
    };

    connect(worker, SIGNAL(n4d_call_completed(QtN4DWorker::Methods,QString)),this, SLOT(ProcessCallback(QtN4DWorker::Methods,QString)));
    local_thread->start();
    qDebug() << "Running N4D Call " << method;
}

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

QString MainWindow::getCellData(int x, int y){
    QAbstractItemModel *model = ui->table_quotas->model();
    QModelIndex idx = model->index(x,y);
    return idx.data().toString();
}

void MainWindow::setCellData(int x, int y, QString *str){
    qDebug() << "Cell " << x << "," << y << " changed from " << getCellData(x,y) << " to " << *str ;
    ui->table_quotas->setItem(x,y,new QTableWidgetItem(*str));
    return ;
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
MainWindow::~MainWindow()
{
    delete ui;
    delete model;
}
