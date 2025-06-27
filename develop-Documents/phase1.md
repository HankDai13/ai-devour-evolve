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

### 步骤 3.4: 实现玩家键盘控制移动

我们需要让玩家细胞能响应键盘的WASD按键，以固定速度移动。

**修改 `GameView.h`**:

```cpp
// ...
#include <QKeyEvent> // 包含键盘事件的头文件
#include <QTimer> // 包含定时器的头文件
#include <QSet> // 包含集合的头文件

class PlayerCell; // 前向声明，告诉编译器有这么一个类，避免头文件循环包含

class GameView : public QGraphicsView
{
    Q_OBJECT // 宏，使用Qt信号与槽机制时必须添加

public:
    // 构造函数，QWidget *parent = nullptr 是C++的常用写法
    GameView(QWidget *parent = nullptr);

protected:
    // 重写键盘事件的处理函数
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void updatePlayerMovement(); // 更新玩家移动的槽函数

private:
    PlayerCell *m_player; // 添加一个成员变量来持有玩家的指针
    QTimer *m_movementTimer; // 移动更新定时器
    QSet<int> m_pressedKeys; // 当前按下的按键集合
    qreal m_playerSpeed; // 玩家移动速度
};
```

**修改 `GameView.cpp`**:

```cpp
#include "GameView.h"
#include "PlayerCell.h" // 包含头文件
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QTimer>
#include <QSet>

GameView::GameView(QWidget *parent) : QGraphicsView(parent), m_player(nullptr), m_playerSpeed(5.0)
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
    
    // 4. 启用键盘焦点，这样才能接收键盘事件
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();
    
    // 5. 创建一个玩家细胞，初始半径为20，放在舞台中央
    m_player = new PlayerCell(800, 600, 20);
    scene->addItem(m_player); // 将玩家添加到舞台上！
    
    // 6. 设置移动更新定时器，60FPS
    m_movementTimer = new QTimer(this);
    connect(m_movementTimer, &QTimer::timeout, this, &GameView::updatePlayerMovement);
    m_movementTimer->start(16); // 约60FPS (1000ms / 60 ≈ 16ms)
}

void GameView::keyPressEvent(QKeyEvent *event)
{
    // 将按下的键添加到集合中
    m_pressedKeys.insert(event->key());
    
    // 调用父类的实现，保持其他默认行为
    QGraphicsView::keyPressEvent(event);
}

void GameView::keyReleaseEvent(QKeyEvent *event)
{
    // 将释放的键从集合中移除
    m_pressedKeys.remove(event->key());
    
    // 调用父类的实现，保持其他默认行为
    QGraphicsView::keyReleaseEvent(event);
}

void GameView::updatePlayerMovement()
{
    if (!m_player) return;
    
    QPointF currentPos = m_player->pos();
    QPointF newPos = currentPos;
    
    // 检查按下的键，更新位置
    if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up)) {
        newPos.setY(newPos.y() - m_playerSpeed); // 向上移动
    }
    if (m_pressedKeys.contains(Qt::Key_S) || m_pressedKeys.contains(Qt::Key_Down)) {
        newPos.setY(newPos.y() + m_playerSpeed); // 向下移动
    }
    if (m_pressedKeys.contains(Qt::Key_A) || m_pressedKeys.contains(Qt::Key_Left)) {
        newPos.setX(newPos.x() - m_playerSpeed); // 向左移动
    }
    if (m_pressedKeys.contains(Qt::Key_D) || m_pressedKeys.contains(Qt::Key_Right)) {
        newPos.setX(newPos.x() + m_playerSpeed); // 向右移动
    }
    
    // 边界检查，确保玩家不会移出场景
    QRectF sceneRect = scene()->sceneRect();
    qreal radius = m_player->radius();
    
    if (newPos.x() - radius < sceneRect.left()) {
        newPos.setX(sceneRect.left() + radius);
    }
    if (newPos.x() + radius > sceneRect.right()) {
        newPos.setX(sceneRect.right() - radius);
    }
    if (newPos.y() - radius < sceneRect.top()) {
        newPos.setY(sceneRect.top() + radius);
    }
    if (newPos.y() + radius > sceneRect.bottom()) {
        newPos.setY(sceneRect.bottom() - radius);
    }
    
    // 设置新位置
    m_player->setPos(newPos);
}
```

编译运行，现在你可以使用WASD键或方向键来控制蓝色圆圈的移动了！移动非常流畅，并且玩家不会移出游戏边界。

**第一阶段基础部分已经完成！** 你已经掌握了Qt图形开发最核心的流程。现在让我们继续完成完整的游戏原型。

## 4. 完成完整原型：食物生成和碰撞检测

接下来我们将实现完整的游戏机制，包括食物生成、碰撞检测和成长系统。这将让你拥有一个真正可玩的游戏原型！

### 步骤 4.1: 创建食物类 (FoodItem)

**新建文件**: `src/FoodItem.h`

```cpp
#ifndef FOODITEM_H
#define FOODITEM_H

#include <QGraphicsObject>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

class FoodItem : public QGraphicsObject
{
    Q_OBJECT

public:
    FoodItem(qreal x, qreal y, qreal radius = 8.0);

    // QGraphicsItem的必须重写的函数
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 获取食物的营养值（影响玩家成长幅度）
    qreal nutritionValue() const { return m_nutritionValue; }

private:
    qreal m_radius;
    qreal m_nutritionValue;
    QColor m_color;
};

#endif // FOODITEM_H
```

**新建文件**: `src/FoodItem.cpp`

```cpp
#include "FoodItem.h"
#include <QPainter>
#include <QPen>
#include <QRandomGenerator>

FoodItem::FoodItem(qreal x, qreal y, qreal radius) : m_radius(radius)
{
    // 设置位置
    setPos(x, y);
    
    // 根据大小设置营养值
    m_nutritionValue = m_radius * 0.5;
    
    // 随机选择食物颜色（不同颜色代表不同营养价值）
    QList<QColor> foodColors = {
        QColor(255, 100, 100), // 红色
        QColor(100, 255, 100), // 绿色
        QColor(255, 255, 100), // 黄色
        QColor(255, 150, 100), // 橙色
        QColor(150, 100, 255)  // 紫色
    };
    
    m_color = foodColors[QRandomGenerator::global()->bounded(foodColors.size())];
    
    // 设置一些标志
    setFlag(QGraphicsItem::ItemIsSelectable, false);
}

QRectF FoodItem::boundingRect() const
{
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}

void FoodItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // 设置抗锯齿
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 设置填充颜色
    painter->setBrush(QBrush(m_color));
    
    // 设置边框
    QPen pen(m_color.darker(150), 1);
    painter->setPen(pen);
    
    // 绘制圆形食物
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);
}
```

### 步骤 4.2: 在GameView中添加食物生成系统

**修改 `GameView.h`**:

```cpp
#include <QGraphicsView>
#include <QKeyEvent>
#include <QTimer>
#include <QSet>

class PlayerCell;
class FoodItem; // 新增前向声明

class GameView : public QGraphicsView
{
    Q_OBJECT

public:
    GameView(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void updatePlayerMovement();
    void generateFood(); // 新增：生成食物的槽函数

private:
    PlayerCell *m_player;
    QTimer *m_movementTimer;
    QTimer *m_foodTimer; // 新增：食物生成定时器
    QSet<int> m_pressedKeys;
    qreal m_playerSpeed;
    
    // 新增：食物生成相关的私有函数
    void spawnFoodAtRandomLocation();
    QPointF getRandomFoodLocation();
};
```

**修改 `GameView.cpp`**:

```cpp
#include "GameView.h"
#include "PlayerCell.h"
#include "FoodItem.h" // 新增包含
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QTimer>
#include <QSet>
#include <QRandomGenerator> // 新增包含

GameView::GameView(QWidget *parent) : QGraphicsView(parent), m_player(nullptr), m_playerSpeed(5.0)
{
    // ... 之前的代码 ...
    
    // 7. 设置食物生成定时器，每500毫秒生成一个食物
    m_foodTimer = new QTimer(this);
    connect(m_foodTimer, &QTimer::timeout, this, &GameView::generateFood);
    m_foodTimer->start(500); // 每500毫秒生成一个食物
    
    // 8. 初始生成一些食物
    for (int i = 0; i < 10; ++i) {
        spawnFoodAtRandomLocation();
    }
}

// ... keyPressEvent, keyReleaseEvent, updatePlayerMovement 保持不变 ...

void GameView::generateFood()
{
    // 检查场景中食物数量，如果少于15个就生成新的
    QList<QGraphicsItem*> items = scene()->items();
    int foodCount = 0;
    for (QGraphicsItem* item : items) {
        if (qgraphicsitem_cast<FoodItem*>(item)) {
            foodCount++;
        }
    }
    
    if (foodCount < 15) {
        spawnFoodAtRandomLocation();
    }
}

void GameView::spawnFoodAtRandomLocation()
{
    QPointF pos = getRandomFoodLocation();
    
    // 随机生成不同大小的食物
    qreal radius = 5.0 + QRandomGenerator::global()->bounded(8.0); // 5-13像素的随机半径
    
    FoodItem *food = new FoodItem(pos.x(), pos.y(), radius);
    scene()->addItem(food);
}

QPointF GameView::getRandomFoodLocation()
{
    QRectF sceneRect = scene()->sceneRect();
    
    qreal x = sceneRect.left() + QRandomGenerator::global()->bounded(int(sceneRect.width()));
    qreal y = sceneRect.top() + QRandomGenerator::global()->bounded(int(sceneRect.height()));
    
    return QPointF(x, y);
}
```

### 步骤 4.3: 实现碰撞检测和吞噬机制

**修改 `PlayerCell.h`**:

```cpp
#ifndef PLAYERCELL_H
#define PLAYERCELL_H

#include <QGraphicsObject>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTimer> // 新增

class PlayerCell : public QGraphicsObject
{
    Q_OBJECT

public:
    PlayerCell(qreal x, qreal y, qreal radius);
    
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    qreal radius() const { return m_radius; }
    void setRadius(qreal radius);
    
    // 新增：获取玩家得分
    int score() const { return m_score; }

private slots:
    void checkCollisions(); // 新增：检查碰撞的槽函数

private:
    qreal m_radius;
    QTimer *m_collisionTimer; // 新增：碰撞检测定时器
    int m_score; // 新增：玩家得分
    
    void eatFood(class FoodItem* food); // 新增：吞噬食物的函数
};

#endif // PLAYERCELL_H
```

**修改 `PlayerCell.cpp`**:

```cpp
#include "PlayerCell.h"
#include "FoodItem.h" // 新增包含
#include <QPainter>
#include <QPen>
#include <QGraphicsScene>
#include <QTimer>

PlayerCell::PlayerCell(qreal x, qreal y, qreal radius) : m_radius(radius), m_score(0)
{
    setPos(x, y);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    
    // 新增：设置碰撞检测定时器
    m_collisionTimer = new QTimer(this);
    connect(m_collisionTimer, &QTimer::timeout, this, &PlayerCell::checkCollisions);
    m_collisionTimer->start(16); // 60FPS检测碰撞
}

// boundingRect() 和 paint() 保持不变...

void PlayerCell::setRadius(qreal radius)
{
    if (radius != m_radius) {
        prepareGeometryChange();
        m_radius = radius;
        update();
    }
}

void PlayerCell::checkCollisions()
{
    // 获取所有与玩家碰撞的物品
    QList<QGraphicsItem*> collidingItems = this->collidingItems();
    
    for (QGraphicsItem* item : collidingItems) {
        // 检查是否是食物
        FoodItem* food = qgraphicsitem_cast<FoodItem*>(item);
        if (food) {
            eatFood(food);
        }
    }
}

void PlayerCell::eatFood(FoodItem* food)
{
    if (!food || !scene()) return;
    
    // 增加得分
    m_score += static_cast<int>(food->nutritionValue() * 10);
    
    // 增加半径（成长效果）
    qreal growthAmount = food->nutritionValue() * 0.3;
    setRadius(m_radius + growthAmount);
    
    // 从场景中移除食物
    scene()->removeItem(food);
    delete food;
    
    // 在控制台输出得分（后续可以用UI显示）
    qDebug() << "Score:" << m_score << "Radius:" << m_radius;
}
```

### 步骤 4.4: 更新CMakeLists.txt

```cmake
# 更新源文件列表
set(SOURCES
    src/main.cpp
    src/DemoQtVS.cpp
    src/GameView.cpp
    src/PlayerCell.cpp
    src/FoodItem.cpp    # 新增
    # 未来添加更多源文件时在这里列出
)

set(HEADERS
    src/DemoQtVS.h
    src/GameView.h
    src/PlayerCell.h
    src/FoodItem.h      # 新增
    # 未来添加更多头文件时在这里列出
)
```

### 步骤 4.6: 编译运行完整原型

**更新CMakeLists.txt**:

确保你的CMakeLists.txt包含所有新文件：

```cmake
# 明确指定源文件
set(SOURCES
    src/main.cpp
    src/DemoQtVS.cpp
    src/GameView.cpp
    src/PlayerCell.cpp
    src/FoodItem.cpp    # 新增
)

set(HEADERS
    src/DemoQtVS.h
    src/GameView.h
    src/PlayerCell.h
    src/FoodItem.h      # 新增
)
```

现在编译运行项目，你将体验到：

1. **键盘控制**: 使用WASD或方向键控制蓝色玩家细胞移动
2. **食物生成**: 场景中会随机生成彩色的食物点
3. **吞噬成长**: 碰撞食物后玩家细胞会变大，并获得得分
4. **边界限制**: 玩家无法移出游戏区域
5. **动态食物**: 食物会持续生成，保持游戏的趣味性

**🎉 恭喜！你现在拥有了一个功能完整的游戏原型！**

这个原型包含了现代游戏的核心要素：
- 流畅的玩家控制
- 动态的游戏对象生成
- 碰撞检测和游戏逻辑
- 成长机制和得分系统
- 良好的视觉反馈

**Phase 1 完整原型开发完成！** 你已经成功实现了一个可玩的细胞吞噬游戏原型。

### 步骤 4.6: 可选的增强功能

如果你还想继续完善，可以尝试：

1. **添加UI界面**: 显示当前得分和玩家大小
2. **添加音效**: 吞噬食物时播放音效
3. **添加特殊食物**: 不同类型的食物有不同效果
4. **添加敌对细胞**: AI控制的竞争对手
5. **保存最高分**: 将最佳成绩保存到文件

**你的第一阶段开发完美收官！** 为后续的网络多人和AI集成奠定了坚实的基础。