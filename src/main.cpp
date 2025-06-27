#include <QApplication>
#include <ctime>
#include "StartScreen.h"
#include "HumanController.h"
#include "AIController.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    srand(static_cast<unsigned>(time(0)));
    StartScreen startScreen;
    startScreen.show();
    return app.exec();
}