#ifndef DOINGPARTITIONING_H
#define DOINGPARTITIONING_H

#include <QDialog>
#include <QIcon>

namespace Ui {
class DoingPartitioning;
}

class DoingPartitioning : public QDialog
{
    Q_OBJECT

public:
    explicit DoingPartitioning(QWidget *parent = 0);
    ~DoingPartitioning();

public slots:
    void addOperation(QString operationName, QIcon icon);
    void nextStep();

private:
    Ui::DoingPartitioning *ui;
    int stepCounter = -1;
    QList<QIcon> icons;
};

#endif // DOINGPARTITIONING_H
