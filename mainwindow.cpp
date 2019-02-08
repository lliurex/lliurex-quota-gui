#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qgraphicsproxywidget.h"
#include "qdebug.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->table_quotas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->logo->setPixmap(QPixmap(QString("/home/lliurex/banner.png")));
    ui->logo->setAlignment(Qt::AlignCenter);

    qDebug() << getCellData(1,1);
    setCellData(1,1,new QString("Hello"));
    qDebug() << getCellData(1,1);

    connect(ui->table_quotas, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellChanged(int,int)));
}

void MainWindow::ChangeStack(){
    ui->stackedWidget->setCurrentIndex(1);
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
