#include "signupwidget.h"
#include "ui_signupwidget.h"
#include <QDebug>

SignUpWidget::SignUpWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SignUpWidget) {
    ui->setupUi(this);
    connect(ui->submit, &QPushButton::clicked, this, [this]() {
        qInfo() << "username:" << ui->userName->text();
        qInfo() << "familiar with data visualization:"
                << ui->famDataVis->isChecked() << ui->unfamDataVis->isChecked();
        qInfo() << "familiar with scientific visualization:"
                << ui->famSciVis->isChecked() << ui->unfamSciVis->isChecked();
        qInfo() << "familiar with volumetric data:"
                << ui->famVolData->isChecked() << ui->unfamVolData->isChecked();
        qInfo() << "familiar with histogram:"
                << ui->famHist->isChecked() << ui->unfamHist->isChecked();
        clear();
        emit submit();
    });
    connect(ui->cancel, &QPushButton::clicked, this, [this]() {
        clear();
        emit cancel();
    });
}

SignUpWidget::~SignUpWidget() {
    delete ui;
}

void SignUpWidget::clear() {
    ui->userName->clear();
    ui->famDataVis->setChecked(false);
    ui->unfamDataVis->setChecked(false);
    ui->famSciVis->setChecked(false);
    ui->unfamSciVis->setChecked(false);
    ui->famVolData->setChecked(false);
    ui->unfamVolData->setChecked(false);
    ui->famHist->setChecked(false);
    ui->unfamHist->setChecked(false);
}
