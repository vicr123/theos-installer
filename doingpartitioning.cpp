#include "doingpartitioning.h"
#include "ui_doingpartitioning.h"

DoingPartitioning::DoingPartitioning(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DoingPartitioning)
{
    ui->setupUi(this);
}

DoingPartitioning::~DoingPartitioning()
{
    delete ui;
}

void DoingPartitioning::addOperation(QString operationName, QIcon icon) {
    QListWidgetItem* item = new QListWidgetItem();
    item->setText(operationName);
    item->setIcon(icon);
    ui->operationsList->addItem(item);
    icons.append(icon);
}

void DoingPartitioning::nextStep() {
    if (stepCounter != -1) {
        ui->operationsList->item(stepCounter)->setIcon(icons.at(stepCounter));
    }
    stepCounter++;
    if (stepCounter > icons.count()) {
        ui->currentOperationText->setText(ui->operationsList->item(stepCounter)->text());
        ui->operationsList->item(stepCounter)->setIcon(QIcon::fromTheme("go-forward"));
    }
}
