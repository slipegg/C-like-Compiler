#include "dialog2.h"
#include "ui_dialog2.h"

Dialog2::Dialog2(QWidget *parent,QString r1,QString r2,QString r3) :
    QDialog(parent),
    ui(new Ui::Dialog2)
{
    ui->setupUi(this);
    ui->textBrowser->setText(r1);
    ui->textBrowser_2->setText(r2);
    ui->textBrowser_5->setText(r3);
}

Dialog2::~Dialog2()
{
    delete ui;
}
