#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qgraphicsproxywidget.h"
#include "qdebug.h"
#include "n4d.cpp"
#include <QThread>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->table_quotas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->logo->setPixmap(QPixmap(QString("/home/lliurex/banner.png")));
    ui->logo->setAlignment(Qt::AlignCenter);

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
    }
    ui->txt_usu->setText("");
    ui->txt_pwd->setText("");
}

void MainWindow::InitValidation(){
    QString n4duser = ui->txt_usu->text();
    QString n4dpwd = ui->txt_pwd->text();
    if (n4duser.trimmed() == "" or n4dpwd.trimmed() == ""){
        ui->txt_usu->setText("");
        ui->txt_pwd->setText("");
        return;
    }

    local_thread = new QThread;
    QtN4DWorker* worker = new QtN4DWorker();
    worker->set_auth(n4duser,n4dpwd);
    worker->moveToThread(local_thread);
    connect(local_thread, SIGNAL(started()), worker, SLOT(validate_user()));
    connect(worker, SIGNAL(n4d_call_completed(QString)),this, SLOT(CheckValidation(QString)));

    local_thread->start();
    qDebug() << "Validation Started!";
}

void MainWindow::cellChanged(int row, int col){
    qDebug() << "Cell " << col << "," << row << " clicked !" << endl;
    if (col == 0){
        ui->stackedWidget->setCurrentIndex(0);
    }else{
        qDebug() << "Action on col 1 don't work";
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
}
