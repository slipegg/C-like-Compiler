#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent,QString r1,QString r2,QString r3) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->textBrowser->setText(r1);
    ui->textBrowser_2->setText(r2);
    ui->textBrowser_5->setText(r3);
}

Dialog::~Dialog()
{
    delete ui;
}
