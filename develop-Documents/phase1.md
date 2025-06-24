# 第一阶段开发指南：使用VS Code与C++/Qt构建游戏原型

## 1. 目标

完成一个基础但完整的单人游戏原型。它将包含以下功能：

- 一个可以响应鼠标移动的玩家"细胞"
- 地图上随机生成的"能量点"
- 玩家可以吞噬能量点并变大

本指南将带你熟悉Qt的图形视图框架、事件处理和核心类的使用。

## 2. 准备工作：VS Code与项目结构

在开始之前，请确保你已经在VS Code中安装了以下两个关键插件：

- **C/C++**: 微软官方的C++语言支持插件
- **CMake Tools**: 用于在VS Code中方便地配置和运行CMake项目

### 2.1 项目文件结构

在你的项目根目录 `ai-devour-evolve` 中，我们先创建如下的初始文件结构：

```
ai-devour-evolve/
├── .vscode/               # VS Code配置文件目录
│   └── settings.json
├── src/                   # 我们的源代码都放在这里
│   ├── main.cpp
│   └── (之后会添加更多文件)
└── CMakeLists.txt         # 项目的构建脚本 (最重要!)
```

### 2.2 配置CMakeLists.txt (项目的"说明书")

这是整个项目的心脏。将以下内容复制到你的 `CMakeLists.txt` 文件中。它告诉CMake如何找到Qt库，以及如何编译我们的代码。

```cmake
# CMake 最低版本要求
cmake_minimum_required(VERSION 3.16)

# 项目名称和版本
project(AiDevourEvolve VERSION 1.0)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理Qt的元对象编译器 (MOC), 资源编译器 (RCC) 等
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt6库，我们需要Core, Gui, Widgets这三个核心模块
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 创建我们的可执行程序，名为"AiDevourEvolve"
# 源文件暂时只有 main.cpp
add_executable(AiDevourEvolve
    src/main.cpp
)

# 将Qt6的库链接到我们的程序上
target_link_libraries(AiDevourEvolve PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
```

### 2.3 配置VS Code

在 `.vscode/settings.json` 文件中添加以下内容，它可以帮助CMake找到你的Qt安装路径。请务必将路径修改为你自己电脑上Qt的实际安装路径。

```json
{
    "cmake.configureArgs": [
        "-DCMAKE_PREFIX_PATH=D:/Qt/6.7.1/mingw_64"
    ]
}
```

> **路径提示**: `CMAKE_PREFIX_PATH` 应该指向包含 `bin`, `lib`, `include` 等子目录的Qt版本文件夹。

配置完成后，VS Code的CMake Tools插件会自动检测到CMakeLists.txt，并开始配置项目。你可以通过VS Code下方的状态栏来查看和切换构建套件（Kit）、构建模式（Debug/Release）等。

## 3. 编写代码：一步步实现游戏

### 步骤 3.1: 创建程序入口和主窗口

Qt程序都需要一个QApplication对象和至少一个窗口。

**文件**: `src/main.cpp`

```cpp
#include <QApplication>
#include <QMainWindow>

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
```

**编译运行**:
- 按下 `F7` (或在VS Code命令面板输入 `CMake: Build`) 来编译项目
- 编译成功后，按下 `F5` (或点击状态栏的运行按钮) 来运行程序
- 你应该能看到一个800x600的空白窗口

### 步骤 3.2: 设置游戏场景 (QGraphicsView框架)

Qt的QGraphicsView框架是制作2D游戏的利器。把它想象成一个"舞台剧"：

- **QGraphicsScene**: 这是无限大的"舞台"，我们所有的游戏元素（玩家、食物）都放在上面
- **QGraphicsView**: 这是观众的"视野"或"摄像机"，我们通过它来观察舞台

让我们来创建游戏视图。

**新建文件**: `src/GameView.h`

```cpp
#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>

// GameView继承自QGraphicsView，成为我们定制的游戏"摄像机"
class GameView : public QGraphicsView
{
    Q_OBJECT // 宏，使用Qt信号与槽机制时必须添加

public:
    // 构造函数，QWidget *parent = nullptr 是C++的常用写法
    GameView(QWidget *parent = nullptr);
};

#endif // GAMEVIEW_H
```

**新建文件**: `src/GameView.cpp`

```cpp
#include "GameView.h"
#include <QGraphicsScene>

GameView::GameView(QWidget *parent) : QGraphicsView(parent)
{
    // 1. 创建一个"舞台"
    QGraphicsScene *scene = new QGraphicsScene(this);
    // 将舞台的范围设置为1600x1200，比我们的视野大
    scene->setSceneRect(0, 0, 1600, 1200);

    // 2. 将我们这个"摄像机"的舞台设置为刚刚创建的scene
    this->setScene(scene);
    
    // 3. 一些渲染和交互优化
    this->setRenderHint(QPainter::Antialiasing); // 开启抗锯齿，让圆形更平滑
    this->setCacheMode(QGraphicsView::CacheBackground);
    this->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
}
```

**修改 `main.cpp` 来使用我们的 GameView**:

```cpp
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
```

**修改 `CMakeLists.txt`**:

别忘了把新文件加进去！

```cmake
# ... (前面的内容不变)
add_executable(AiDevourEvolve
    src/main.cpp
    src/GameView.h     # 新增
    src/GameView.cpp   # 新增
)
# ... (后面的内容不变)
```

再次编译运行，你看到的依然是窗口，但现在它内部已经是一个功能完备的"舞台"了。

### 步骤 3.3: 创建玩家细胞 (QGraphicsItem)

现在，让我们在舞台上添加一个主角。

**新建文件**: `src/PlayerCell.h`

```cpp
#ifndef PLAYERCELL_H
#define PLAYERCELL_H

#include <QGraphicsEllipseItem> // 我们的玩家是个椭圆（圆形）

class PlayerCell : public QGraphicsObject // 使用QGraphicsObject以支持信号和槽
{
    Q_OBJECT

public:
    PlayerCell(qreal x, qreal y, qreal radius);

    // QGraphicsItem的两个必须重写的纯虚函数
    QRectF boundingRect() const override; // 返回一个能完全包围我们item的矩形
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override; // 如何绘制自己

private:
    qreal m_radius; // 成员变量习惯以 m_ 开头
};

#endif // PLAYERCELL_H
```

**新建文件**: `src/PlayerCell.cpp`

```cpp
#include "PlayerCell.h"
#include <QPainter>

PlayerCell::PlayerCell(qreal x, qreal y, qreal radius) : m_radius(radius)
{
    // 设置初始位置
    setPos(x, y);
}

// 告诉Qt，我们的绘图区域是多大
QRectF PlayerCell::boundingRect() const
{
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}

// 具体的绘制逻辑
void PlayerCell::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 设置画刷为蓝色
    painter->setBrush(Qt::blue);
    // 以(0,0)为中心，画一个半径为m_radius的圆
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}
```

**在 `GameView.cpp` 的构造函数中添加玩家**:

```cpp
#include "GameView.h"
#include <QGraphicsScene>
#include "PlayerCell.h" // 包含头文件

GameView::GameView(QWidget *parent) : QGraphicsView(parent)
{
    // ... (之前的代码)

    // 创建一个玩家细胞，初始半径为20，放在舞台中央
    PlayerCell *player = new PlayerCell(800, 600, 20);
    scene->addItem(player); // 将玩家添加到舞台上！
}
```

**更新 `CMakeLists.txt`**:

```cmake
# ...
add_executable(AiDevourEvolve
    src/main.cpp
    src/GameView.h
    src/GameView.cpp
    src/PlayerCell.h   # 新增
    src/PlayerCell.cpp # 新增
)
# ...
```

编译运行，你将在窗口中央看到一个蓝色的圆圈！

### 步骤 3.4: 实现玩家跟随鼠标移动

我们需要让玩家细胞能响应鼠标的移动。

**修改 `GameView.h`**:

```cpp
// ...
#include <QMouseEvent> // 包含鼠标事件的头文件

class PlayerCell; // 前向声明，告诉编译器有这么一个类，避免头文件循环包含

class GameView : public QGraphicsView
{
    // ...
protected:
    // 重写鼠标移动事件的处理函数
    void mouseMoveEvent(QMouseEvent *event) override;
private:
    PlayerCell *m_player; // 添加一个成员变量来持有玩家的指针
};
```

**修改 `GameView.cpp`**:

```cpp
// ...
GameView::GameView(QWidget *parent) : QGraphicsView(parent)
{
    // ...
    // 将 player 赋值给成员变量 m_player
    m_player = new PlayerCell(800, 600, 20);
    scene->addItem(m_player);
}

void GameView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_player) {
        // mapToScene可以将窗口内的坐标（像素）转换成舞台上的坐标
        QPointF targetPos = mapToScene(event->pos());
        m_player->setPos(targetPos); // 直接设置玩家的位置
    }
    
    // 调用父类的同名函数，确保事件能继续传递（好习惯）
    QGraphicsView::mouseMoveEvent(event);
}
```

编译运行，现在蓝色的圆圈会紧紧地跟随着你的鼠标指针移动了！

**第一阶段到此已经完成了一个巨大的里程碑！** 你已经掌握了Qt图形开发最核心的流程。接下来的食物生成和碰撞检测，都是在这个基础上添砖加瓦。

## 4. 下一步：向完整原型迈进 (选做)

如果你感觉良好，可以继续尝试实现以下功能，来完成整个第一阶段的目标。

1. **创建FoodItem类**: 仿照PlayerCell，创建一个FoodItem类，让它是一个小小的、比如红色的圆点
2. **随机生成食物**: 在GameView中使用QTimer，每隔一段时间（比如100毫秒），就在场景的随机位置new一个FoodItem并添加到scene中
3. **碰撞检测与吞噬**: 在PlayerCell类中也创建一个QTimer，每隔一小段时间（比如16毫秒，约等于60FPS），就调用`this->collidingItems()`来获取所有与自己碰撞的item
4. **实现吞噬逻辑**: 遍历碰撞的item，如果发现是FoodItem，就把它从场景中移除 (`scene()->removeItem(...)`)，并增加自己的半径`m_radius`，然后调用`update()`来重绘自己，让变大的效果显示出来

完成这些，你的第一阶段就完美收官了。你将拥有一个功能完备、可交互的游戏核心原型，为后续的网络和AI集成打下了坚实的基础。祝你编码愉快！