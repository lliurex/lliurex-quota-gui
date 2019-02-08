#include "examples.h"

void example_new_image_label(){

    QWidget* window = new QWidget();
    QHBoxLayout* l = new QHBoxLayout();
    window->setLayout(l);

    QPixmap* img = new QPixmap(QString("/home/lliurex/banner.png"));
    QLabel* lbl = new QLabel();

    lbl->setPixmap(*img);
    l->addWidget(lbl);
    window->show();
}

void example_new_image_scene(){
    QGraphicsScene* scene = new QGraphicsScene();
    QGraphicsView* view = new QGraphicsView(scene);
    view->setBackgroundBrush(QBrush(Qt::transparent));
    QPixmap* pix = new QPixmap(QString("/home/lliurex/banner.png"));
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(*pix);
    scene->addItem(item);
    view->show();
}
/*QString s = QString("/home/lliurex/banner.png");

QImage* image = new QImage(640,80,QImage::Format_ARGB32);
image->fill(qRgba(10,10,10,250));
image->load(s);
QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(*image));
item->show();*/
//QGraphicsScene* scene = new QGraphicsScene();

//ui->frame->children.append(QImage(QString("/home/lliurex/banner.png")));
//ui->frame->setStyleSheet("{background-image: url(:banner.png);}");
//ui->frame->setBaseSize(640,50);
//QGraphicsScene* scene = new QGraphicsScene();
//QGraphicsView* view = new QGraphicsView(scene);
/*ui->graphicsView->setScene(scene);
QString s = QString("/home/lliurex/banner.png");

QImage* image = new QImage(640,80,QImage::Format_ARGB32);
image->fill(qRgba(10,10,10,250));
image->load(s);

QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(*image));
scene->addItem(item);
qDebug() << ui->frame->backgroundRole();
//    qDebug() << p.background();
//    //QBrush* br = new QBrush(qRgba(0,0,0,0));
QBrush *br = new QBrush();
br->setColor(Qt::transparent);
qDebug() << br->style();
qDebug() << "Opaque: "<< br->isOpaque();
ui->graphicsView->setBackgroundBrush(*br);

ui->graphicsView->show();*/
