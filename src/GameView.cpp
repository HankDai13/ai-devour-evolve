#include "GameView.h"
#include "PlayerCell.h" // 包含头文件
#include "FoodItem.h" // 添加食物头文件
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QTimer>
#include <QSet>
#include <QColor>
#include <QRandomGenerator> // 添加随机数生成器
#include <QDebug>

GameView::GameView(QWidget *parent) : QGraphicsView(parent), m_player(nullptr), m_playerSpeed(4.0)
{
    // 1. 创建一个"舞台"
    QGraphicsScene *scene = new QGraphicsScene(this);
    // 将舞台的范围设置为与窗口一致，800x600
    scene->setSceneRect(0, 0, 800, 600);
    // 为场景设置一个浅灰色背景，这有助于彻底解决残影问题
    scene->setBackgroundBrush(QColor(240, 240, 240));

    // 2. 将我们这个"摄像机"的舞台设置为刚刚创建的scene
    this->setScene(scene);    // 3. 一些渲染和交互优化
    this->setRenderHint(QPainter::Antialiasing); // 开启抗锯齿，让圆形更平滑
    this->setCacheMode(QGraphicsView::CacheBackground);
    this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate); // 改为全量更新，彻底解决残影问题
    this->setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);
      // 4. 禁用自动视窗调整，防止视窗跟随玩家移动
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setDragMode(QGraphicsView::NoDrag);
      // 5. 启用键盘焦点，这样才能接收键盘事件
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();
      // 6. 创建一个玩家细胞，初始半径为20，放在舞台中央
    m_player = new PlayerCell(400, 300, 20); // 调整为800x600场景的中央位置
    scene->addItem(m_player); // 将玩家添加到舞台上！
    
    // 连接玩家得分变化信号
    connect(m_player, &PlayerCell::scoreChanged, this, &GameView::onScoreChanged);
    
      // 7. 设置移动更新定时器，恢复到60FPS确保流畅性
    m_movementTimer = new QTimer(this);
    connect(m_movementTimer, &QTimer::timeout, this, &GameView::updatePlayerMovement);
    m_movementTimer->start(16); // 60FPS (1000ms / 60 ≈ 16ms)，确保流畅移动
    
    // 8. 设置食物生成定时器，每500毫秒生成一个食物
    m_foodTimer = new QTimer(this);
    connect(m_foodTimer, &QTimer::timeout, this, &GameView::generateFood);
    m_foodTimer->start(500); // 每500毫秒生成一个食物
    
    // 9. 初始生成一些食物
    for (int i = 0; i < 15; ++i) {
        spawnFoodAtRandomLocation();
    }
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
    bool moved = false;
    
    // 检查按下的键，更新位置
    if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up)) {
        newPos.setY(newPos.y() - m_playerSpeed); // 向上移动
        moved = true;
    }
    if (m_pressedKeys.contains(Qt::Key_S) || m_pressedKeys.contains(Qt::Key_Down)) {
        newPos.setY(newPos.y() + m_playerSpeed); // 向下移动
        moved = true;
    }
    if (m_pressedKeys.contains(Qt::Key_A) || m_pressedKeys.contains(Qt::Key_Left)) {
        newPos.setX(newPos.x() - m_playerSpeed); // 向左移动
        moved = true;
    }
    if (m_pressedKeys.contains(Qt::Key_D) || m_pressedKeys.contains(Qt::Key_Right)) {
        newPos.setX(newPos.x() + m_playerSpeed); // 向右移动
        moved = true;
    }
    
    // 只有在移动时才进行边界检查和位置更新
    if (moved) {
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
}

void GameView::generateFood()
{
    // 检查场景中食物数量，如果少于20个就生成新的
    QList<QGraphicsItem*> items = scene()->items();
    int foodCount = 0;
    for (QGraphicsItem* item : items) {
        if (qgraphicsitem_cast<FoodItem*>(item)) {
            foodCount++;
        }
    }
    
    if (foodCount < 20) {
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
    
    // 确保食物不会生成在边缘附近
    qreal margin = 30;
    qreal x = sceneRect.left() + margin + QRandomGenerator::global()->bounded(int(sceneRect.width() - 2 * margin));
    qreal y = sceneRect.top() + margin + QRandomGenerator::global()->bounded(int(sceneRect.height() - 2 * margin));
    
    return QPointF(x, y);
}

void GameView::onScoreChanged(int newScore)
{
    // 在控制台输出得分变化（可以后续改为UI显示）
    qDebug() << "Player Score:" << newScore;
}