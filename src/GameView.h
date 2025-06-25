#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>
#include <QKeyEvent> // 包含键盘事件的头文件
#include <QTimer> // 包含定时器的头文件
#include <QSet> // 包含集合的头文件

class PlayerCell; // 前向声明，告诉编译器有这么一个类，避免头文件循环包含
class FoodItem; // 前向声明

// GameView继承自QGraphicsView，成为我们定制的游戏"摄像机"
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
    void generateFood(); // 生成食物的槽函数
    void onScoreChanged(int newScore); // 得分变化处理

private:
    PlayerCell *m_player; // 添加一个成员变量来持有玩家的指针
    QTimer *m_movementTimer; // 移动更新定时器
    QTimer *m_foodTimer; // 食物生成定时器
    QSet<int> m_pressedKeys; // 当前按下的按键集合
    qreal m_playerSpeed; // 玩家移动速度
    
    // 食物生成相关的私有函数
    void spawnFoodAtRandomLocation();
    QPointF getRandomFoodLocation();
};

#endif // GAMEVIEW_H