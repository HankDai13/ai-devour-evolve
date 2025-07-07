#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QTabWidget>
#include <QTextEdit>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QFrame>
#include <QScrollArea>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProgressBar>
#include <QButtonGroup>
#include <QRadioButton>

class GameView;

// 游戏模式枚举
enum class GameMode {
    DEBUG_SINGLE_PLAYER,    // 单人调试模式
    SURVIVAL_BATTLE,        // 生存游戏模式
    BOSS_CHALLENGE          // BOSS挑战模式
};

// AI策略配置
struct AIConfig {
    QString name;
    QString description;
    int foodHunterCount = 0;
    int aggressiveCount = 0;
    int modelBasedCount = 0;
    int randomCount = 0;
    
    int getTotalAICount() const {
        return foodHunterCount + aggressiveCount + modelBasedCount + randomCount;
    }
};

// 游戏配置
struct GameConfig {
    GameMode mode;
    int totalTeams = 8;
    int initialScoreMin = 5000;
    int initialScoreMax = 10000;
    QMap<int, AIConfig> teamAIConfigs;
    
    // BOSS模式特殊配置
    int bossInitialScore = 50000;
    int bossTeamId = 0;
    bool enableBossSpecialAbilities = true;
    
    // 游戏世界配置
    int worldSize = 2000;
    int foodDensity = 100;
    int thornDensity = 50;
    
    // 游戏规则配置
    bool enableTeamMode = true;
    bool enableFriendlyFire = false;
    int gameTimeLimit = 0; // 0表示无时间限制
};

class GameStartScreen : public QWidget {
    Q_OBJECT

public:
    explicit GameStartScreen(QWidget* parent = nullptr);
    ~GameStartScreen();

    // 获取游戏配置
    GameConfig getGameConfig() const { return m_gameConfig; }

signals:
    void startGameRequested(const GameConfig& config);
    void exitRequested();

private slots:
    void onModeButtonClicked();
    void onStartGameClicked();
    void onExitClicked();
    void onConfigurationChanged();
    void onRestoreDefaultsClicked();
    void onSaveConfigClicked();
    void onLoadConfigClicked();
    void updateAnimations();

private:
    void setupUI();
    void setupModeSelectionPanel();
    void setupConfigurationPanel();
    void setupActionPanel();
    void setupAnimations();
    void setupModeButtons();
    void setupDebugModeConfig();
    void setupSurvivalModeConfig();
    void setupBossModeConfig();
    
    void switchToMode(GameMode mode);
    void updateModeDescription();
    void updateStartButtonText();
    void updateConfigurationVisibility();
    void resetConfigurationToDefaults();
    
    void applyTheme();
    void createGradientBackground();
    void createFloatingParticles();
    
    // 辅助方法声明
    void updateGameConfig();
    void updateAIConfigInterface();
    void updateBossTeamComboBox();
    bool validateConfiguration();
    
    // 配置序列化
    QJsonObject configToJson() const;
    void configFromJson(const QJsonObject& json);
    
    // UI组件
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    
    // 左侧面板：模式选择
    QGroupBox* m_modeSelectionGroup;
    QVBoxLayout* m_modeLayout;
    QButtonGroup* m_modeButtonGroup;
    QPushButton* m_debugModeBtn;
    QPushButton* m_survivalModeBtn;
    QPushButton* m_bossModeBtn;
    QLabel* m_modeDescriptionLabel;
    
    // 右侧面板：配置选项
    QGroupBox* m_configurationGroup;
    QScrollArea* m_configScrollArea;
    QWidget* m_configWidget;
    QVBoxLayout* m_configLayout;
    QTabWidget* m_configTabs;
    
    // 配置选项卡
    QWidget* m_basicConfigTab;
    QWidget* m_aiConfigTab;
    QWidget* m_worldConfigTab;
    QWidget* m_advancedConfigTab;
    
    // 基本配置
    QLabel* m_teamsLabel;
    QSpinBox* m_teamsSpinBox;
    QLabel* m_scoreRangeLabel;
    QSpinBox* m_scoreMinSpinBox;
    QSpinBox* m_scoreMaxSpinBox;
    
    // AI配置
    QMap<int, QWidget*> m_teamConfigWidgets;
    QVBoxLayout* m_aiConfigLayout;
    
    // 世界配置
    QLabel* m_worldSizeLabel;
    QSpinBox* m_worldSizeSpinBox;
    QLabel* m_foodDensityLabel;
    QSlider* m_foodDensitySlider;
    QLabel* m_thornDensityLabel;
    QSlider* m_thornDensitySlider;
    
    // 高级配置
    QCheckBox* m_teamModeCheckBox;
    QCheckBox* m_friendlyFireCheckBox;
    QLabel* m_timeLimitLabel;
    QSpinBox* m_timeLimitSpinBox;
    
    // BOSS模式特殊配置
    QWidget* m_bossConfigWidget;
    QLabel* m_bossScoreLabel;
    QSpinBox* m_bossScoreSpinBox;
    QComboBox* m_bossTeamComboBox;
    QCheckBox* m_bossAbilitiesCheckBox;
    
    // 底部操作面板
    QHBoxLayout* m_actionLayout;
    QPushButton* m_startGameBtn;
    QPushButton* m_exitBtn;
    QPushButton* m_saveConfigBtn;
    QPushButton* m_loadConfigBtn;
    QPushButton* m_defaultsBtn;
    
    // 动画和效果
    QPropertyAnimation* m_fadeAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    QTimer* m_animationTimer;
    QVector<QWidget*> m_floatingParticles;
    
    // 状态管理
    GameConfig m_gameConfig;
    GameMode m_currentMode;
    bool m_isConfigurationValid;
    
    // 样式常量
    static const QString THEME_COLOR_PRIMARY;
    static const QString THEME_COLOR_SECONDARY;
    static const QString THEME_COLOR_ACCENT;
    static const QString THEME_COLOR_BACKGROUND;
    static const QString THEME_COLOR_SURFACE;
    static const QString THEME_COLOR_TEXT;
    static const QString THEME_COLOR_TEXT_SECONDARY;

    // 配置界面设置方法
    void setupBasicConfigTab();
    void setupAIConfigTab();
    void setupWorldConfigTab();
    void setupAdvancedConfigTab();
    void setupBossConfigWidget();
};
