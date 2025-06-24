#include "DemoQtVS.h"

#include <QApplication>
#include <QMainWindow>

// 注释掉MSVC特定的pragma，MinGW不需要
// #pragma comment(lib, "user32.lib")

int main(int argc, char *argv[])
{
    // 1. 创建应用程序对象，这是每个Qt程序都必须的
    QApplication app(argc, argv);

    // 2. 创建一个主窗口
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("智能吞噬进化 - 开发原型"); // 设置窗口标题
    mainWindow.setFixedSize(800, 600);                // 设置窗口大小为800x600

    // 3. 显示主窗口
    mainWindow.show();

    // 4. 进入事件循环，程序会在这里等待用户的操作（如点击、移动鼠标）
    return app.exec();
}