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
#include <QRandomGenerator>
#include <QListView>

struct CommandConfig {
    QString systemCommand;
    qint64 lastRunTime;
    int offsetMs;
};

class TelegraphWindow : public QMainWindow {
    Q_OBJECT

public:
    TelegraphWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("PIC KONTROL İSTASYONU");
        resize(1200, 750);

        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setSpacing(25);
        mainLayout->setContentsMargins(25, 25, 25, 25);

        setupStyles(true);

        QGroupBox *settingsGroup = new QGroupBox();
        QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);
        settingsGroup->setFixedWidth(340);
        settingsLayout->setContentsMargins(15, 15, 15, 15);
        settingsLayout->setSpacing(12);

        QLabel *settingsTitle = new QLabel("BAĞLANTI & KONTROL");
        settingsTitle->setAlignment(Qt::AlignCenter);
        settingsTitle->setObjectName("boxTitle");
        settingsLayout->addWidget(settingsTitle);
        settingsLayout->addSpacing(10);

        portSelect = new QComboBox();
        refreshPorts();

        baudSelect = new QComboBox();
        QStringList bauds = {"9600", "115200", "57600", "38400"};
        baudSelect->addItems(bauds);

        themeButton = new QPushButton("AYDINLIK MODA GEÇ");
        themeButton->setCursor(Qt::PointingHandCursor);

        customCmdInput = new QLineEdit();
        customCmdInput->setPlaceholderText("Özel Komut");
        
        sendCmdButton = new QPushButton("KOMUTU YOLLA");
        sendCmdButton->setCursor(Qt::PointingHandCursor);
        sendCmdButton->setFixedHeight(40);

        statusLabel = new QLabel("DURUM: BAĞLI DEĞİL");
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setObjectName("statusLabel");

        connectButton = new QPushButton("BAĞLAN");
        connectButton->setCursor(Qt::PointingHandCursor);
        connectButton->setFixedHeight(50);
        connectButton->setObjectName("connectBtn");

        QPushButton *refreshBtn = new QPushButton("PORTLARI YENİLE");
        refreshBtn->setCursor(Qt::PointingHandCursor);

        settingsLayout->addWidget(themeButton);
        settingsLayout->addSpacing(10);
        settingsLayout->addWidget(new QLabel("PORT SEÇİMİ"));
        settingsLayout->addWidget(portSelect);
        settingsLayout->addWidget(new QLabel("BAUDRATE"));
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
        chatLayout->addWidget(chatTitle);
        chatLayout->addSpacing(10);

        chatDisplay = new QTextEdit();
        chatDisplay->setReadOnly(true);
        chatDisplay->setFocusPolicy(Qt::NoFocus);

        messageInput = new QLineEdit();
        messageInput->setPlaceholderText("Mesaj yazın...");
        messageInput->setFixedHeight(45);

        chatLayout->addWidget(chatDisplay);
        chatLayout->addSpacing(15); 
        chatLayout->addWidget(messageInput);

        QGroupBox *logGroup = new QGroupBox();
        QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
        logGroup->setFixedWidth(360);
        logLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *logTitle = new QLabel("SİSTEM LOGLARI");
        logTitle->setAlignment(Qt::AlignCenter);
        logTitle->setObjectName("boxTitle");
        logLayout->addWidget(logTitle);
        logLayout->addSpacing(10);

        logDisplay = new QTextEdit();
        logDisplay->setReadOnly(true);

        QPushButton *clearButton = new QPushButton("LOGLARI TEMİZLE");
        clearButton->setCursor(Qt::PointingHandCursor);

        logLayout->addWidget(logDisplay);
        logLayout->addSpacing(15); 
        logLayout->addWidget(clearButton);

        mainLayout->addWidget(settingsGroup);
        mainLayout->addWidget(chatGroup, 1);
        mainLayout->addWidget(logGroup);

        serial = new QSerialPort(this);

#ifdef Q_OS_WIN
        commandMap["BR"]   = {"cmd /c start https://google.com", 0, 3000};
        commandMap["TERM"] = {"start cmd.exe", 0, 2000};
#else
        commandMap["BR"]   = {"xdg-open https://google.com", 0, 3000};
        commandMap["TERM"] = {"alacritty", 0, 2000};
#endif

        connect(connectButton, &QPushButton::clicked, this, &TelegraphWindow::toggleConnection);
        connect(refreshBtn, &QPushButton::clicked, this, [this](){
            refreshPorts();
            appendLog("KULLANICI: Port listesi yenilendi.");
        });
        connect(serial, &QSerialPort::readyRead, this, &TelegraphWindow::readSerialData);
        connect(messageInput, &QLineEdit::returnPressed, this, &TelegraphWindow::sendMessage);
        connect(sendCmdButton, &QPushButton::clicked, this, &TelegraphWindow::sendCustomCommand);
        connect(themeButton, &QPushButton::clicked, this, &TelegraphWindow::toggleTheme);
        connect(clearButton, &QPushButton::clicked, this, [this](){
            logDisplay->clear();
            chatDisplay->clear();
            writeToFile("--- LOGLAR TEMİZLENDİ ---");
        });

        writeToFile("--- OTURUM BAŞLADI ---");
    }

private:
    QSerialPort *serial;
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

    void setupStyles(bool dark) {
        QString bgColor = dark ? "#1e1e2e" : "#eff1f5";
        QString fgColor = dark ? "#cdd6f4" : "#4c4f69";
        QString boxBg = dark ? "#181825" : "#e6e9ef";
        QString boxBorder = dark ? "#313244" : "#bcc0cc";
        QString accent = dark ? "#89b4fa" : "#1e66f5";
        QString inputBg = dark ? "#313244" : "#ffffff";
        QString btnBg = dark ? "#45475a" : "#ccd0da";
        QString btnHover = dark ? "#585b70" : "#b4befe";
        QString titleBg = dark ? "#45475a" : "#bcc0cc";
        QString titleFg = dark ? "#cdd6f4" : "#4c4f69"; 
        QString success = "#a6e3a1"; 
        QString danger = "#f38ba8";

        QString style = QString(R"(
            QMainWindow { background-color: %1; }
            QWidget { color: %2; font-family: 'JetBrains Mono', 'Segoe UI', sans-serif; font-size: 13px; outline: none; }
            
            QGroupBox {
                border: 2px solid %4;
                border-radius: 16px;
                background-color: %3;
            }

            QLabel#boxTitle {
                background-color: %10;
                color: #000000;
                font-size: 14px;
                font-weight: 900;
                padding: 10px;
                border-radius: 6px;
                margin: 0px;
            }

            QLineEdit {
                background-color: %6;
                border: 2px solid %4;
                border-radius: 8px;
                padding: 8px;
                color: %2;
                selection-background-color: %5;
            }
            QLineEdit:focus {
                border: 2px solid %5;
            }

            QComboBox {
                background-color: %6;
                border: 2px solid %4;
                border-radius: 8px;
                padding: 8px 10px;
                color: %2;
            }
            QComboBox:focus {
                border: 2px solid %5;
            }
            QComboBox::drop-down {
                subcontrol-origin: padding;
                subcontrol-position: top right;
                width: 30px;
                border-left-width: 0px;
                border-top-right-radius: 8px;
                border-bottom-right-radius: 8px;
                background-color: rgba(0,0,0,0.1);
            }
            QComboBox::down-arrow {
                width: 10px;
                height: 10px;
                background: none;
                border-left: 2px solid %2;
                border-bottom: 2px solid %2;
                transform: rotate(-45deg);
                margin-top: -3px;
                margin-left: -3px;
            }
            QComboBox QAbstractItemView {
                background-color: %6;
                color: %2;
                border: 2px solid %4;
                selection-background-color: %5;
                selection-color: %1;
                outline: none;
                border-radius: 4px;
                padding: 5px;
            }

            QTextEdit {
                background-color: %6;
                border: 2px solid %4;
                border-radius: 12px;
                padding: 10px;
                color: %2;
            }

            QPushButton {
                background-color: %7;
                color: %2;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-weight: bold;
                outline: none;
            }
            QPushButton:hover { background-color: %8; color: #1e1e2e; }
            QPushButton:focus { outline: none; border: none; }

            QPushButton#connectBtn {
                background-color: %5;
                color: %1;
                font-size: 15px;
                border-radius: 12px;
            }
            QPushButton#connectBtn:hover { background-color: %10; }
            
            QLabel#statusLabel {
                font-size: 14px;
                font-weight: bold;
                padding: 10px;
                border: 2px dashed %4;
                border-radius: 8px;
                margin-bottom: 10px;
            }

            QScrollBar:vertical {
                border: none; background: %1; width: 10px; margin: 0; border-radius: 5px;
            }
            QScrollBar::handle:vertical {
                background: %7; min-height: 20px; border-radius: 5px;
            }
        )")
        .arg(bgColor, fgColor, boxBg, boxBorder, accent, inputBg, btnBg, btnHover, danger, titleBg, titleFg);

        qApp->setStyleSheet(style);
    }

    void toggleTheme() {
        isDarkTheme = !isDarkTheme;
        themeButton->setText(isDarkTheme ? "AYDINLIK MODA GEÇ" : "KARANLIK MODA GEÇ");
        setupStyles(isDarkTheme);
        appendLog("KULLANICI: Tema değiştirildi.");
    }

    void refreshPorts() {
        portSelect->clear();
        const auto infos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &info : infos) {
            portSelect->addItem(info.portName());
        }
        if(portSelect->count() == 0) {
#ifdef Q_OS_WIN
            portSelect->addItem("COM1");
            portSelect->addItem("COM3");
#else
            portSelect->addItem("/dev/ttyACM0");
            portSelect->addItem("/dev/ttyUSB0");
#endif
        }
    }

    void toggleConnection() {
        if (serial->isOpen()) {
            serial->close();
            connectButton->setText("BAĞLAN");
            connectButton->setStyleSheet(""); 
            statusLabel->setText("DURUM: BAĞLI DEĞİL");
            statusLabel->setStyleSheet("color: #f38ba8; font-weight: bold; border: 2px dashed #f38ba8;");
            appendLog("KULLANICI: Bağlantı kesildi.");
        } else {
            serial->setPortName(portSelect->currentText());
            serial->setBaudRate(baudSelect->currentText().toInt());
            serial->setDataBits(QSerialPort::Data8);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setParity(QSerialPort::NoParity);
            
            if (serial->open(QIODevice::ReadWrite)) {
                connectButton->setText("BAĞLANTIYI KES");
                connectButton->setStyleSheet("background-color: #f38ba8; color: #1e1e2e;");
                statusLabel->setText("DURUM: BAĞLI (" + portSelect->currentText() + ")");
                statusLabel->setStyleSheet("color: #a6e3a1; font-weight: bold; border: 2px solid #a6e3a1;");
                appendLog("KULLANICI: Bağlantı kuruldu -> " + portSelect->currentText());
            } else {
                QMessageBox::warning(this, "Hata", "Port açılamadı.\nLütfen yetkilerinizi kontrol edin.");
                appendLog("HATA: Port açılamadı.");
            }
        }
    }

    void sendNMEAPacket(QString type, QString payload) {
        if (!serial->isOpen()) {
            QMessageBox::warning(this, "Uyarı", "Önce bağlantı kurmalısınız!");
            return;
        }

        QString raw = type + "," + payload;
        int checksum = 0;
        QByteArray bytes = raw.toLatin1();
        for(char c : bytes) {
            checksum ^= c;
        }

        QString packet = "$" + raw + "," + QString::number(checksum, 16).toUpper() + "\r\n";
        serial->write(packet.toUtf8());
    }

    void sendMessage() {
        QString msg = messageInput->text();
        if (msg.isEmpty()) return;

        sendNMEAPacket("M", msg);
        appendChat(msg, true);
        appendLog("GÖNDERİLDİ ($M): " + msg);
        messageInput->clear();
    }

    void sendCustomCommand() {
        QString cmd = customCmdInput->text();
        if (cmd.isEmpty()) {
            cmd = "PING"; 
        }

        sendNMEAPacket("K", cmd);
        appendLog("KOMUT YOLLANDI ($K): " + cmd);
        customCmdInput->clear();
    }

    void readSerialData() {
        while (serial->canReadLine()) {
            QByteArray data = serial->readLine().trimmed();
            QString line = QString::fromUtf8(data);
            
            if (line.isEmpty()) continue;

            if (line.startsWith("$K")) {
                handleCommand(line);
                appendLog("GELEN KOMUT: " + line);
            } 
            else {
                if(line.startsWith("$M,")) line = line.mid(3).split(',')[0];
                else if(line.startsWith("$")) line = line.mid(1).split('*')[0]; 
                
                appendChat(line, false);
                appendLog("GELEN MESAJ: " + line);
            }
        }
    }

    void handleCommand(QString rawData) {
        QStringList parts = rawData.split(',');
        if (parts.size() < 2) return; 
        
        QString cmdKey = parts[1].toUpper(); 
        
        if (commandMap.contains(cmdKey)) {
            CommandConfig &cfg = commandMap[cmdKey];
            qint64 now = QDateTime::currentMSecsSinceEpoch();

            if (now - cfg.lastRunTime > cfg.offsetMs) {
                QProcess::startDetached(cfg.systemCommand);
                cfg.lastRunTime = now;
                appendLog("SİSTEM EYLEMİ: " + cfg.systemCommand);
            } else {
                appendLog("SİSTEM: " + cmdKey + " (Zaman aşımı)");
            }
        } else {
            appendLog("BİLİNMEYEN KOMUT: " + cmdKey);
        }
    }

    void appendLog(QString text) {
        QString currentDate = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss");

        if (currentDate != lastLogDate) {
            logDisplay->setAlignment(Qt::AlignCenter);
            logDisplay->append("\n─── " + currentDate + " ───\n");
            logDisplay->setAlignment(Qt::AlignLeft);
            lastLogDate = currentDate;
        }

        logDisplay->append("[" + currentTime + "] " + text);
        writeToFile("[" + currentDate + " " + currentTime + "] " + text);
        logDisplay->verticalScrollBar()->setValue(logDisplay->verticalScrollBar()->maximum());
    }

    void writeToFile(QString text) {
        QFile file("telgraf.log");
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << text << "\n";
            file.close();
        }
    }

    void appendChat(QString text, bool isMe) {
        QString align = isMe ? "right" : "left";
        QString bgColor = isMe ? (isDarkTheme ? "#313244" : "#bcc0cc") : (isDarkTheme ? "#45475a" : "#9ca0b0"); 
        QString textColor = isDarkTheme ? "#cdd6f4" : "#303446";
        QString timeStr = QDateTime::currentDateTime().toString("HH:mm");

        QString html = QString(
            "<div style='width:100%; display:flex; justify-content:%1; margin-bottom:12px;'>"
            "<div style='background-color:%2; color:%3; padding:12px 16px; border-radius:12px; display:inline-block; border: 1px solid #555;'>"
            "<span style='font-size:14px; font-weight:500;'>%4</span>"
            "<br><span style='font-size:10px; opacity:0.7; float:right; margin-top:4px;'>%5</span>"
            "</div></div>"
        ).arg(align).arg(bgColor).arg(textColor).arg(text).arg(timeStr);
        
        chatDisplay->append(html);
        chatDisplay->verticalScrollBar()->setValue(chatDisplay->verticalScrollBar()->maximum());
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TelegraphWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"