#ifndef DIALOG2_H
#define DIALOG2_H

#include <QDialog>

namespace Ui {
class Dialog2;
}

class Dialog2 : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog2(QWidget *parent = nullptr,QString r1="",QString r2="",QString r3="");
    ~Dialog2();

private:
    Ui::Dialog2 *ui;
};

#endif // DIALOG2_H
