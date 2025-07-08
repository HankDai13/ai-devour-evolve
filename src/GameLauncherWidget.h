#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QFrame>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include "SimpleAIPlayer.h"

class GameView;

class GameLauncherWidget : public QWidget {
    Q_OBJECT

public:
    explicit GameLauncherWidget(QWidget* parent = nullptr);
    ~GameLauncherWidget();

    // 游戏模式
    enum class GameMode {
        SINGLE_DEBUG,    // 单人调试模式
        SURVIVAL,        // 生存游戏模式
        BOSS            // BOSS模式
    };

    // 游戏配置
    class GameConfig {
    public:
        GameMode mode;
        int teamCount;
        int playersPerTeam;
        int initialScoreMin;
        int initialScoreMax;
        bool enableRLModel;
        QString rlModelPath;
        QList<GoBigger::AI::SimpleAIPlayer::AIStrategy> aiStrategies;
    };

signals:
    void gameStartRequested(const GameConfig& config);

private slots:
    void onSingleDebugClicked();
    void onSurvivalClicked();
    void onBossClicked();
    void onStartGameClicked();
    void onBackToMenuClicked();
    void updateConfigPreview();
    void animateTitle();

private:
    void setupUI();
    void setupTitleArea();
    void setupModeButtons();
    void setupConfigPanel();
    void setupSurvivalConfig();
    void setupBossConfig();
    void showConfigPanel(GameMode mode);
    void hideConfigPanel();
    void applyTheme();
    
    GameConfig getCurrentConfig() const;
    QString getConfigSummary(const GameConfig& config) const;

    // UI组件
    QVBoxLayout* m_mainLayout;
    QWidget* m_titleWidget;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    
    // 模式选择按钮
    QWidget* m_modeButtonsWidget;
    QPushButton* m_singleDebugButton;
    QPushButton* m_survivalButton;
    QPushButton* m_bossButton;
    
    // 配置面板
    QScrollArea* m_configScrollArea;
    QWidget* m_configWidget;
    QGroupBox* m_configGroup;
    QLabel* m_configPreviewLabel;
    
    // 生存模式配置
    QGroupBox* m_survivalConfigGroup;
    QSpinBox* m_teamCountSpin;
    QSpinBox* m_initialScoreMinSpin;
    QSpinBox* m_initialScoreMaxSpin;
    QCheckBox* m_enableRLCheckBox;
    QComboBox* m_aiStrategy1Combo;
    QComboBox* m_aiStrategy2Combo;
    
    // BOSS模式配置
    QGroupBox* m_bossConfigGroup;
    QSpinBox* m_bossScoreSpin;
    QSpinBox* m_playerScoreSpin;
    QSpinBox* m_bossCountSpin;
    QComboBox* m_bossStrategyCombo;
    
    // 控制按钮
    QPushButton* m_startGameButton;
    QPushButton* m_backButton;
    
    // 当前配置
    GameMode m_currentMode;
    
    // 动画
    QPropertyAnimation* m_titleAnimation;
    QTimer* m_animationTimer;
    
    // 样式
    QString m_primaryColor;
    QString m_secondaryColor;
    QString m_accentColor;
    QString m_backgroundColor;
};
