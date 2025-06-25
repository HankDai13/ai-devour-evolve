#include <QApplication>
#include <QMainWindow>
#include "GameView.h" // 包含头文件

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow mainWindow;
    
    // 创建我们自己的GameView实例
    GameView *view = new GameView(&mainWindow);
    
    // 将主窗口的中心部件设置为我们的游戏视图
    mainWindow.setCentralWidget(view);
    
    mainWindow.setWindowTitle("智能吞噬进化 - 开发原型");
    mainWindow.resize(800, 600); // 使用resize，而不是setFixedSize，让窗口可以调整大小
    mainWindow.show();

    return app.exec();
}