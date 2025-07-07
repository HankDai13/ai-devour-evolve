#include "DemoQtVS.h"
#include "ui_DemoQtVS.h"

DemoQtVS::DemoQtVS(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui_DemoQtVS)
{
    ui->setupUi(this);
}

DemoQtVS::~DemoQtVS()
{
    delete ui; 
}