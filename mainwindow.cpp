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
    for (int i=0 ; i < ui->table_quotas->rowCount(); i++){
        QTableWidgetItem *item = ui->table_quotas->item(i,0);
        item->setFlags(item->flags() ^ Qt::ItemIsEditable );
    }
//    qDebug() << getCellData(1,1);
//    setCellData(1,1,new QString("Hello"));
//    qDebug() << getCellData(1,1);

    connect(ui->table_quotas, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellChanged(int,int)));
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

void MainWindow::CompletePopulate(QString returned){
     map<string,UserData> table;
     qDebug() << "Completing table populate !";
     model->LoadUsersFromString(returned.toStdString());
     table = model->getTableData();
     /*ui->table_quotas->model()->insertRow(0);
     ui->table_quotas->model()->insertRow(0);
     ui->table_quotas->model()->insertRow(0);
     ui->table_quotas->model()->insertRow(0);
     ui->table_quotas->model()->insertRow(0);*/
     ui->table_quotas->model()->removeRows(0,2);
     unsigned i=0;
     for (auto const& [key,value]: table){
         ui->table_quotas->model()->insertRow(ui->table_quotas->model()->rowCount());

         UserData data = value;
         string k=key;

         for (unsigned int j=0; j< 2; j++){
              if (j == 0){
                 k ="name";
             }else{
                 k = "spaceused";
             }
             setCellData(i,j,new QString(QString::fromStdString(data.getField(k))));
         }
         i++;
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
    //CheckValidation(QString("admins"));
    //return;
    n4duser = ui->txt_usu->text();
    n4dpwd = ui->txt_pwd->text();
    if (n4duser.trimmed() == "" or n4dpwd.trimmed() == ""){
        ui->txt_usu->setText("");
        ui->txt_pwd->setText("");
        return;
    }
    InitN4DCall(QtN4DWorker::Methods::LOGIN);
}

void MainWindow::cellChanged(int row, int col){
    qDebug() << "Cell " << col << "," << row << " clicked !" << endl;
    if (col == 0){
        qDebug() << "Action on col 0 don't work";
        //ui->stackedWidget->setCurrentIndex(0);
    }else{
        qDebug() << "Action on col 1 work";
    }

}

QString MainWindow::getCellData(int x, int y){
    QAbstractItemModel *model = ui->table_quotas->model();
    QModelIndex idx = model->index(x,y);
    return idx.data().toString();
}

void MainWindow::setCellData(int x, int y, QString *str){
    qDebug() << "Cell " << x << "," << y << " changed from " << getCellData(x,y) << " to " << *str << endl;
    ui->table_quotas->setItem(x,y,new QTableWidgetItem(*str));
    return ;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete model;
}
