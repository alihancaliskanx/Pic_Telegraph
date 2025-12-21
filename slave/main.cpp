#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QGroupBox>
#include <QProcess>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QScrollBar>
#include <QMessageBox>
#include <QThread>

struct CommandConfig {
    QString systemCommand;
    qint64 lastRunTime;
    int offsetMs;
};

class SerialWorker : public QObject {
    Q_OBJECT
public:
    QSerialPort *serialPort;

    SerialWorker() {
        serialPort = new QSerialPort();
    }
    ~SerialWorker() {
        if(serialPort->isOpen()) serialPort->close();
        delete serialPort;
    }

public slots:
    void openPort(QString name, int baud) {
        serialPort->setPortName(name);
        serialPort->setBaudRate(baud);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        
        if(serialPort->open(QIODevice::ReadWrite)) {
            emit connectionStatusChanged(true, name);
            connect(serialPort, &QSerialPort::readyRead, this, &SerialWorker::readData);
        } else {
            emit connectionStatusChanged(false, "");
        }
    }

    void closePort() {
        if(serialPort->isOpen()) {
            serialPort->close();
            disconnect(serialPort, &QSerialPort::readyRead, this, &SerialWorker::readData);
        }
        emit connectionStatusChanged(false, "");
    }

    void writeData(QString data) {
        if(serialPort->isOpen()) {
            serialPort->write(data.toUtf8());
        }
    }

    void readData() {
        while(serialPort->canReadLine()) {
            QByteArray data = serialPort->readLine().trimmed();
            QString line = QString::fromUtf8(data);
            if(!line.isEmpty()) {
                if(validateChecksum(line)) {
                    emit messageReceived(line);
                } else {
                    emit errorOccurred("CHECKSUM HATASI: " + line);
                }
            }
        }
    }

private:
    bool validateChecksum(QString packet) {
        if(!packet.startsWith("$") || !packet.contains("*")) return false;
        
        int starIndex = packet.lastIndexOf('*');
        QString content = packet.mid(1, starIndex - 1);
        QString receivedSumStr = packet.mid(starIndex + 1);
        
        int calculatedSum = 0;
        QByteArray bytes = content.toLatin1();
        for(char c : bytes) {
            calculatedSum ^= c;
        }
        
        bool ok;
        int receivedSum = receivedSumStr.toInt(&ok, 16);
        
        return ok && (calculatedSum == receivedSum);
    }

signals:
    void messageReceived(QString message);
    void connectionStatusChanged(bool connected, QString portName);
    void errorOccurred(QString error);
};

class TelegraphWindow : public QMainWindow {
    Q_OBJECT

public:
    TelegraphWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("PIC KONTROL İSTASYONU V2");
        resize(1200, 750);

        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setSpacing(25);
        mainLayout->setContentsMargins(25, 25, 25, 25);

        setupStyles(true);
        setupUI(mainLayout);
        setupCommands();

        serialThread = new QThread(this);
        worker = new SerialWorker();
        worker->moveToThread(serialThread);

        connect(serialThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &TelegraphWindow::operateOpen, worker, &SerialWorker::openPort);
        connect(this, &TelegraphWindow::operateClose, worker, &SerialWorker::closePort);
        connect(this, &TelegraphWindow::operateWrite, worker, &SerialWorker::writeData);
        
        connect(worker, &SerialWorker::messageReceived, this, &TelegraphWindow::handleSerialMessage);
        connect(worker, &SerialWorker::connectionStatusChanged, this, &TelegraphWindow::updateConnectionUI);
        connect(worker, &SerialWorker::errorOccurred, this, &TelegraphWindow::appendLog);

        serialThread->start();
        writeToFile("--- OTURUM BAŞLADI ---");
    }

    ~TelegraphWindow() {
        serialThread->quit();
        serialThread->wait();
    }

private:
    QThread *serialThread;
    SerialWorker *worker;
    
    QComboBox *portSelect;
    QComboBox *baudSelect;
    QPushButton *connectButton;
    QPushButton *themeButton;
    QPushButton *sendCmdButton;
    QLabel *statusLabel;
    QTextEdit *chatDisplay;
    QLineEdit *messageInput;
    QLineEdit *customCmdInput;
    QTextEdit *logDisplay;
    
    QMap<QString, CommandConfig> commandMap;
    QString lastLogDate;
    bool isDarkTheme = true;
    bool isConnected = false;

    void setupUI(QHBoxLayout *mainLayout) {
        QGroupBox *settingsGroup = new QGroupBox();
        QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);
        settingsGroup->setFixedWidth(340);
        settingsLayout->setContentsMargins(15, 15, 15, 15);
        settingsLayout->setSpacing(12);

        QLabel *settingsTitle = new QLabel("BAĞLANTI & KONTROL");
        settingsTitle->setAlignment(Qt::AlignCenter);
        settingsTitle->setObjectName("boxTitle");
        settingsLayout->addWidget(settingsTitle);

        portSelect = new QComboBox();
        refreshPorts();
        baudSelect = new QComboBox();
        baudSelect->addItems({"9600", "115200", "57600"});

        themeButton = new QPushButton("AYDINLIK MODA GEÇ");
        customCmdInput = new QLineEdit();
        customCmdInput->setPlaceholderText("Özel Komut (örn: rst)");
        sendCmdButton = new QPushButton("KOMUTU YOLLA");
        sendCmdButton->setFixedHeight(40);
        statusLabel = new QLabel("DURUM: BAĞLI DEĞİL");
        statusLabel->setObjectName("statusLabel");
        statusLabel->setAlignment(Qt::AlignCenter);
        connectButton = new QPushButton("BAĞLAN");
        connectButton->setFixedHeight(50);
        connectButton->setObjectName("connectBtn");

        QPushButton *refreshBtn = new QPushButton("PORTLARI YENİLE");

        settingsLayout->addSpacing(10);
        settingsLayout->addWidget(themeButton);
        settingsLayout->addWidget(new QLabel("PORT"));
        settingsLayout->addWidget(portSelect);
        settingsLayout->addWidget(new QLabel("BAUD"));
        settingsLayout->addWidget(baudSelect);
        settingsLayout->addWidget(refreshBtn);
        settingsLayout->addSpacing(20);
        settingsLayout->addWidget(new QLabel("MANUEL KOMUT"));
        settingsLayout->addWidget(customCmdInput);
        settingsLayout->addWidget(sendCmdButton);
        settingsLayout->addStretch();
        settingsLayout->addWidget(statusLabel);
        settingsLayout->addWidget(connectButton);

        QGroupBox *chatGroup = new QGroupBox();
        QVBoxLayout *chatLayout = new QVBoxLayout(chatGroup);
        chatLayout->setContentsMargins(15, 15, 15, 15);
        
        QLabel *chatTitle = new QLabel("MESAJLAŞMA");
        chatTitle->setAlignment(Qt::AlignCenter);
        chatTitle->setObjectName("boxTitle");
        chatDisplay = new QTextEdit();
        chatDisplay->setReadOnly(true);
        messageInput = new QLineEdit();
        messageInput->setPlaceholderText("Mesaj yazın...");
        messageInput->setFixedHeight(45);

        chatLayout->addWidget(chatTitle);
        chatLayout->addWidget(chatDisplay);
        chatLayout->addWidget(messageInput);

        QGroupBox *logGroup = new QGroupBox();
        QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
        logGroup->setFixedWidth(360);
        
        QLabel *logTitle = new QLabel("SİSTEM LOGLARI");
        logTitle->setAlignment(Qt::AlignCenter);
        logTitle->setObjectName("boxTitle");
        logDisplay = new QTextEdit();
        logDisplay->setReadOnly(true);
        QPushButton *clearButton = new QPushButton("LOGLARI TEMİZLE");

        logLayout->addWidget(logTitle);
        logLayout->addWidget(logDisplay);
        logLayout->addWidget(clearButton);

        mainLayout->addWidget(settingsGroup);
        mainLayout->addWidget(chatGroup, 1);
        mainLayout->addWidget(logGroup);

        connect(connectButton, &QPushButton::clicked, this, &TelegraphWindow::toggleConnection);
        connect(refreshBtn, &QPushButton::clicked, this, &TelegraphWindow::refreshPorts);
        connect(messageInput, &QLineEdit::returnPressed, this, &TelegraphWindow::sendMessage);
        connect(sendCmdButton, &QPushButton::clicked, this, &TelegraphWindow::sendCustomCommand);
        connect(themeButton, &QPushButton::clicked, this, &TelegraphWindow::toggleTheme);
        connect(clearButton, &QPushButton::clicked, [this](){ logDisplay->clear(); });
    }

    void setupCommands() {
#ifdef Q_OS_WIN
        commandMap["BR"] = {"cmd /c start https://google.com", 0, 3000};
        commandMap["TERM"] = {"start cmd.exe", 0, 2000};
#else
        commandMap["BR"] = {"xdg-open https://google.com", 0, 3000};
        commandMap["TERM"] = {"alacritty", 0, 2000};
#endif
    }

    void setupStyles(bool dark) {
        QString bgColor = dark ? "#1e1e2e" : "#eff1f5";
        QString fgColor = dark ? "#cdd6f4" : "#4c4f69";
        QString boxBg = dark ? "#181825" : "#e6e9ef";
        QString boxBorder = dark ? "#313244" : "#bcc0cc";
        QString accent = dark ? "#89b4fa" : "#1e66f5";
        QString inputBg = dark ? "#313244" : "#ffffff";
        QString btnBg = dark ? "#45475a" : "#ccd0da";
        
        QString style = QString(R"(
            QMainWindow { background-color: %1; }
            QWidget { color: %2; font-family: 'Segoe UI', sans-serif; font-size: 13px; }
            QGroupBox { border: 2px solid %4; border-radius: 16px; background-color: %3; }
            QLabel#boxTitle { background-color: %10; color: #000; font-weight: 900; padding: 10px; border-radius: 6px; }
            QLineEdit, QTextEdit, QComboBox { background-color: %6; border: 2px solid %4; border-radius: 8px; padding: 8px; color: %2; }
            QPushButton { background-color: %7; color: %2; border: none; border-radius: 8px; padding: 10px; font-weight: bold; }
            QPushButton:hover { background-color: %8; }
            QPushButton#connectBtn { background-color: %5; color: %1; }
            QLabel#statusLabel { border: 2px dashed %4; border-radius: 8px; font-weight: bold; }
        )").arg(bgColor, fgColor, boxBg, boxBorder, accent, inputBg, btnBg, "#585b70", "#f38ba8", "#bcc0cc");
        qApp->setStyleSheet(style);
    }

    void toggleTheme() {
        isDarkTheme = !isDarkTheme;
        themeButton->setText(isDarkTheme ? "AYDINLIK MODA GEÇ" : "KARANLIK MODA GEÇ");
        setupStyles(isDarkTheme);
    }

    void refreshPorts() {
        portSelect->clear();
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
            portSelect->addItem(info.portName());
        }
    }

    void toggleConnection() {
        if (isConnected) {
            emit operateClose();
        } else {
            emit operateOpen(portSelect->currentText(), baudSelect->currentText().toInt());
        }
    }

    void updateConnectionUI(bool connected, QString portName) {
        isConnected = connected;
        if (connected) {
            connectButton->setText("BAĞLANTIYI KES");
            connectButton->setStyleSheet("background-color: #f38ba8; color: #1e1e2e;");
            statusLabel->setText("DURUM: BAĞLI (" + portName + ")");
            statusLabel->setStyleSheet("color: #a6e3a1; border: 2px solid #a6e3a1;");
            appendLog("BAĞLANTI: Başarılı -> " + portName);
        } else {
            connectButton->setText("BAĞLAN");
            connectButton->setStyleSheet("");
            statusLabel->setText("DURUM: BAĞLI DEĞİL");
            statusLabel->setStyleSheet("color: #f38ba8; border: 2px dashed #f38ba8;");
            appendLog("BAĞLANTI: Kesildi veya Başarısız.");
        }
    }

    void sendPacket(QString type, QString payload) {
        if (!isConnected) return;
        QString raw = type + "," + payload;
        int checksum = 0;
        QByteArray bytes = raw.toLatin1();
        for(char c : bytes) checksum ^= c;
        QString packet = "$" + raw + "*" + QString::number(checksum, 16).toUpper() + "\r\n";
        emit operateWrite(packet);
    }

    void sendMessage() {
        QString msg = messageInput->text();
        if (msg.isEmpty()) return;
        sendPacket("M", msg);
        appendChat(msg, true);
        messageInput->clear();
    }

    void sendCustomCommand() {
        QString cmd = customCmdInput->text();
        if (cmd.isEmpty()) return;
        sendPacket("K", cmd);
        appendLog("GÖNDERİLEN KOMUT: " + cmd);
        customCmdInput->clear();
    }

    void handleSerialMessage(QString line) {
        if (line.startsWith("$ACK")) {
            QString cmdType = line.split(',')[1].split('*')[0];
            appendLog("SİSTEM ONAYI (ACK): " + cmdType);
        }
        else if (line.startsWith("$K")) {
            QString cmd = line.split(',')[1].split('*')[0];
            appendLog("GELEN KOMUT: " + cmd);
            
            if (commandMap.contains(cmd)) {
                QProcess::startDetached(commandMap[cmd].systemCommand);
                appendLog("SİSTEM EYLEMİ: " + commandMap[cmd].systemCommand);
            }
        }
        else if (line.startsWith("$M")) {
            QString msg = line.split(',')[1].split('*')[0];
            appendChat(msg, false);
        }
    }

    void appendLog(QString text) {
        QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
        logDisplay->append("[" + timeStr + "] " + text);
        writeToFile(text);
        logDisplay->verticalScrollBar()->setValue(logDisplay->verticalScrollBar()->maximum());
    }

    void appendChat(QString text, bool isMe) {
        QString align = isMe ? "right" : "left";
        QString color = isMe ? (isDarkTheme ? "#313244" : "#e6e9ef") : (isDarkTheme ? "#45475a" : "#ccd0da");
        QString html = QString("<div style='text-align:%1;'><span style='background:%2;padding:5px 10px;border-radius:10px;'>%3</span></div>")
                       .arg(align, color, text);
        chatDisplay->append(html);
    }

    void writeToFile(QString text) {
        QFile file("telgraf_v2.log");
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss") << " - " << text << "\n";
            file.close();
        }
    }

signals:
    void operateOpen(QString name, int baud);
    void operateClose();
    void operateWrite(QString data);
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TelegraphWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"