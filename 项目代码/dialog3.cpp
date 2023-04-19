#include "dialog3.h"
#include "ui_dialog3.h"

Dialog3::Dialog3(QWidget *parent,QString res) :
    QDialog(parent),
    ui(new Ui::Dialog3)
{
    ui->setupUi(this);
    ui->textBrowser->setText(res);
}

Dialog3::~Dialog3()
{
    delete ui;
}
