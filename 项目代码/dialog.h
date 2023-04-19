#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr,QString r1="",QString r2="",QString r3="");
    ~Dialog();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
