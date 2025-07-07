#pragma once
#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
class Ui_DemoQtVS;
QT_END_NAMESPACE

class DemoQtVS : public QMainWindow {
    Q_OBJECT
    
public:
    DemoQtVS(QWidget* parent = nullptr);
    ~DemoQtVS();

private:
    Ui_DemoQtVS* ui;
};