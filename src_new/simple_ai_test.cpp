#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include <QFont>
#include <vector>
#include <memory>
#include <ctime>
#include <algorithm>

// 禁用LibTorch的一些警告
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4251)
#pragma warning(disable: 4267)
#endif

#include <torch/torch.h>
#include <torch/script.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

class SimpleAITest : public QWidget {
    Q_OBJECT
    
public:
    SimpleAITest(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        connectSignals();
    }
    
private slots:
    void selectModel() {
        QString modelPath = QFileDialog::getOpenFileName(
            this, 
            "Select AI Model", 
            "assets/ai_models/exported_models", 
            "PyTorch Models (*.pt);;All Files (*)"
        );
        
        if (!modelPath.isEmpty()) {
            m_modelPath = modelPath;
            m_modelPathLabel->setText(modelPath);
            loadModel();
        }
    }
    
    void loadModel() {
        if (m_modelPath.isEmpty()) {
            appendLog("No model path selected");
            return;
        }
        
        try {
            appendLog("Loading model: " + m_modelPath);
            
            // 加载TorchScript模型
            m_model = std::make_unique<torch::jit::Module>(torch::jit::load(m_modelPath.toStdString()));
            m_model->eval();
            
            m_modelLoaded = true;
            m_testInferenceButton->setEnabled(true);
            
            appendLog("✓ Model loaded successfully!");
            
        } catch (const std::exception& e) {
            appendLog(QString("✗ Failed to load model: %1").arg(e.what()));
            m_modelLoaded = false;
            m_testInferenceButton->setEnabled(false);
        }
    }
    
    void testInference() {
        if (!m_modelLoaded || !m_model) {
            appendLog("Model not loaded");
            return;
        }
        
        try {
            appendLog("Testing inference...");
            
            // 创建随机输入数据 (400维观测向量)
            std::vector<float> observation(400);
            for (int i = 0; i < 400; ++i) {
                observation[i] = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f; // [-1, 1]
            }
            
            // 创建输入tensor
            torch::Tensor input_tensor = torch::from_blob(
                observation.data(), 
                {1, 400}, 
                torch::kFloat
            );
            
            // 执行推理
            std::vector<torch::jit::IValue> inputs;
            inputs.push_back(input_tensor);
            
            appendLog(QString("Input tensor shape: [%1, %2]")
                     .arg(input_tensor.size(0))
                     .arg(input_tensor.size(1)));
            
            at::Tensor output = m_model->forward(inputs).toTensor();
            
            appendLog(QString("Output tensor shape: [%1, %2]")
                     .arg(output.size(0))
                     .arg(output.size(1)));
            
            // 提取输出值
            auto output_accessor = output.accessor<float, 2>();
            float dx = output_accessor[0][0];
            float dy = output_accessor[0][1];
            float action_type_raw = output_accessor[0][2];
            
            // 处理输出
            dx = std::clamp(dx, -1.0f, 1.0f);
            dy = std::clamp(dy, -1.0f, 1.0f);
            
            int action_type = 0; // MOVE
            if (action_type_raw >= 1.5f) {
                action_type = 2; // EJECT
            } else if (action_type_raw >= 0.5f) {
                action_type = 1; // SPLIT
            }
            
            QString actionName = (action_type == 0) ? "MOVE" : 
                               (action_type == 1) ? "SPLIT" : "EJECT";
            
            appendLog(QString("✓ Inference successful!"));
            appendLog(QString("  dx: %1, dy: %2").arg(dx, 0, 'f', 3).arg(dy, 0, 'f', 3));
            appendLog(QString("  action: %1 (raw: %2)").arg(actionName).arg(action_type_raw, 0, 'f', 3));
            
        } catch (const std::exception& e) {
            appendLog(QString("✗ Inference failed: %1").arg(e.what()));
        }
    }
    
    void runContinuousTest() {
        if (!m_continuousTimer) {
            m_continuousTimer = new QTimer(this);
            connect(m_continuousTimer, &QTimer::timeout, this, &SimpleAITest::testInference);
        }
        
        if (!m_continuousTimer->isActive()) {
            m_continuousTimer->start(1000); // 每秒测试一次
            m_continuousTestButton->setText("Stop Continuous Test");
            appendLog("Started continuous inference test...");
        } else {
            m_continuousTimer->stop();
            m_continuousTestButton->setText("Start Continuous Test");
            appendLog("Stopped continuous inference test");
        }
    }
    
private:
    void setupUI() {
        setWindowTitle("Simple AI Model Test");
        setMinimumSize(800, 600);
        
        auto mainLayout = new QVBoxLayout(this);
        
        // 模型选择
        auto modelLayout = new QHBoxLayout();
        modelLayout->addWidget(new QLabel("Model:"));
        m_modelPathLabel = new QLabel("No model selected");
        m_modelPathLabel->setStyleSheet("border: 1px solid gray; padding: 5px;");
        modelLayout->addWidget(m_modelPathLabel, 1);
        
        auto selectButton = new QPushButton("Select Model");
        modelLayout->addWidget(selectButton);
        connect(selectButton, &QPushButton::clicked, this, &SimpleAITest::selectModel);
        
        mainLayout->addLayout(modelLayout);
        
        // 控制按钮
        auto buttonLayout = new QHBoxLayout();
        
        auto loadButton = new QPushButton("Load Model");
        connect(loadButton, &QPushButton::clicked, this, &SimpleAITest::loadModel);
        buttonLayout->addWidget(loadButton);
        
        m_testInferenceButton = new QPushButton("Test Inference");
        m_testInferenceButton->setEnabled(false);
        connect(m_testInferenceButton, &QPushButton::clicked, this, &SimpleAITest::testInference);
        buttonLayout->addWidget(m_testInferenceButton);
        
        m_continuousTestButton = new QPushButton("Start Continuous Test");
        connect(m_continuousTestButton, &QPushButton::clicked, this, &SimpleAITest::runContinuousTest);
        buttonLayout->addWidget(m_continuousTestButton);
        
        auto clearButton = new QPushButton("Clear Log");
        connect(clearButton, &QPushButton::clicked, [this]() { m_logText->clear(); });
        buttonLayout->addWidget(clearButton);
        
        mainLayout->addLayout(buttonLayout);
        
        // 日志输出
        mainLayout->addWidget(new QLabel("Log:"));
        m_logText = new QTextEdit();
        m_logText->setReadOnly(true);
        m_logText->setFont(QFont("Consolas", 9));
        mainLayout->addWidget(m_logText);
        
        // 设置默认模型路径
        QString defaultPath = "assets/ai_models/exported_models/ai_model_traced.pt";
        m_modelPathLabel->setText(defaultPath);
        m_modelPath = defaultPath;
    }
    
    void connectSignals() {
        // 连接信号（当前为空）
    }
    
    void appendLog(const QString& message) {
        QString timestamp = QTime::currentTime().toString("hh:mm:ss.zzz");
        m_logText->append(QString("[%1] %2").arg(timestamp, message));
        m_logText->ensureCursorVisible();
        
        // 同时输出到控制台
        qDebug() << message;
    }
    
private:
    QString m_modelPath;
    std::unique_ptr<torch::jit::Module> m_model;
    bool m_modelLoaded = false;
    QTimer* m_continuousTimer = nullptr;
    
    // UI组件
    QLabel* m_modelPathLabel;
    QPushButton* m_testInferenceButton;
    QPushButton* m_continuousTestButton;
    QTextEdit* m_logText;
};

#include "simple_ai_test.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 初始化随机数种子
    srand(static_cast<unsigned>(time(nullptr)));
    
    SimpleAITest window;
    window.show();
    
    return app.exec();
}
