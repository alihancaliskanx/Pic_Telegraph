#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QProcess>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QScrollBar>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothSocket>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QBluetoothAddress>

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
            emit errorOccurred("SERI PORT HATASI: " + serialPort->errorString());
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
                emit messageReceived(line);
            }
        }
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
        setWindowTitle("PIC KONTROL ISTASYONU V3");
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

        QLabel *settingsTitle = new QLabel("BAGLANTI & KONTROL");
        settingsTitle->setAlignment(Qt::AlignCenter);
        settingsTitle->setObjectName("boxTitle");
        settingsLayout->addWidget(settingsTitle);
        settingsLayout->addSpacing(10);

        themeButton = new QPushButton("AYDINLIK MODA GEC");
        themeButton->setCursor(Qt::PointingHandCursor);

        connectionTypeSelect = new QComboBox();
        connectionTypeSelect->addItem("BLUETOOTH SERI");
        connectionTypeSelect->addItem("USB SERI (KABLO)");

        portSelect = new QComboBox();
        
        baudSelect = new QComboBox();
        QStringList bauds = {"9600", "115200", "57600", "38400"};
        baudSelect->addItems(bauds);

        customCmdInput = new QLineEdit();
        customCmdInput->setPlaceholderText("Ozel Komut (Orn: RESET)");
        
        sendCmdButton = new QPushButton("KOMUTU YOLLA");
        sendCmdButton->setCursor(Qt::PointingHandCursor);
        sendCmdButton->setFixedHeight(40);

        statusLabel = new QLabel("DURUM: BAGLI DEGIL");
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setObjectName("statusLabel");

        connectButton = new QPushButton("BAGLAN");
        connectButton->setCursor(Qt::PointingHandCursor);
        connectButton->setFixedHeight(50);
        connectButton->setObjectName("connectBtn");

        scanButton = new QPushButton("BLUETOOTH TARA");
        scanButton->setCursor(Qt::PointingHandCursor);

        settingsLayout->addWidget(themeButton);
        settingsLayout->addSpacing(10);
        
        settingsLayout->addWidget(new QLabel("BAGLANTI TURU"));
        settingsLayout->addWidget(connectionTypeSelect);

        settingsLayout->addWidget(new QLabel("CIHAZ / PORT SECIMI"));
        settingsLayout->addWidget(portSelect);
        settingsLayout->addWidget(new QLabel("BAUDRATE"));
        settingsLayout->addWidget(baudSelect);
        settingsLayout->addWidget(scanButton);
        
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
        
        QLabel *chatTitle = new QLabel("MESAJLASMA");
        chatTitle->setAlignment(Qt::AlignCenter);
        chatTitle->setObjectName("boxTitle");
        chatLayout->addWidget(chatTitle);
        chatLayout->addSpacing(10);

        chatDisplay = new QTextEdit();
        chatDisplay->setReadOnly(true);
        chatDisplay->setFocusPolicy(Qt::NoFocus);

        messageInput = new QLineEdit();
        messageInput->setPlaceholderText("Mesaj yazin...");
        messageInput->setFixedHeight(45);

        chatLayout->addWidget(chatDisplay);
        chatLayout->addSpacing(15); 
        chatLayout->addWidget(messageInput);

        QGroupBox *logGroup = new QGroupBox();
        QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
        logGroup->setFixedWidth(360);
        logLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *logTitle = new QLabel("SISTEM LOGLARI");
        logTitle->setAlignment(Qt::AlignCenter);
        logTitle->setObjectName("boxTitle");
        logLayout->addWidget(logTitle);
        logLayout->addSpacing(10);

        logDisplay = new QTextEdit();
        logDisplay->setReadOnly(true);

        QPushButton *clearButton = new QPushButton("LOGLARI TEMIZLE");
        clearButton->setCursor(Qt::PointingHandCursor);

        logLayout->addWidget(logDisplay);
        logLayout->addSpacing(15); 
        logLayout->addWidget(clearButton);

        mainLayout->addWidget(settingsGroup);
        mainLayout->addWidget(chatGroup, 1);
        mainLayout->addWidget(logGroup);

        btSocket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
        discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

        serialThread = new QThread(this);
        worker = new SerialWorker();
        worker->moveToThread(serialThread);

        connect(serialThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &TelegraphWindow::operateOpenSerial, worker, &SerialWorker::openPort);
        connect(this, &TelegraphWindow::operateCloseSerial, worker, &SerialWorker::closePort);
        connect(this, &TelegraphWindow::operateWriteSerial, worker, &SerialWorker::writeData);
        
        connect(worker, &SerialWorker::messageReceived, this, &TelegraphWindow::handleSerialMessage);
        connect(worker, &SerialWorker::connectionStatusChanged, this, &TelegraphWindow::handleSerialConnectionStatus);
        connect(worker, &SerialWorker::errorOccurred, this, &TelegraphWindow::appendLog);

        serialThread->start();

#ifdef Q_OS_WIN
        commandMap["BR"]   = {"cmd /c start https://google.com", 0, 3000};
        commandMap["TERM"] = {"start cmd.exe", 0, 2000};
#elif defined(Q_OS_MAC)
        commandMap["BR"]   = {"open https://google.com", 0, 3000};
        commandMap["TERM"] = {"open -a Terminal", 0, 2000};
#else
        commandMap["BR"] = {"chromium https://google.com", 0, 3000};
        commandMap["TERM"] = {"kitty", 0, 2000};
        commandMap["NV"] = {"kitty nvim", 0, 2000};
        commandMap["FL"] = {"nautilus", 0, 2000};
        commandMap["UPD"] = {"kitty --hold sudo pacman -Syu", 0, 10000};
#endif

        connect(connectionTypeSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TelegraphWindow::onConnectionTypeChanged);
        connect(connectButton, &QPushButton::clicked, this, &TelegraphWindow::toggleConnection);
        connect(scanButton, &QPushButton::clicked, this, &TelegraphWindow::handleScanOrRefresh);
        
        connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &TelegraphWindow::deviceDiscovered);
        connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, [this](){
            scanButton->setEnabled(true);
            scanButton->setText("BLUETOOTH TARA");
            appendLog("SISTEM: Tarama tamamlandi.");
        });

        connect(btSocket, &QBluetoothSocket::connected, this, &TelegraphWindow::btSocketConnected);
        connect(btSocket, &QBluetoothSocket::disconnected, this, &TelegraphWindow::btSocketDisconnected);
        connect(btSocket, &QBluetoothSocket::readyRead, this, &TelegraphWindow::readBtSocketData);
        
        connect(messageInput, &QLineEdit::returnPressed, this, &TelegraphWindow::sendMessage);
        connect(sendCmdButton, &QPushButton::clicked, this, &TelegraphWindow::sendCustomCommand);
        connect(themeButton, &QPushButton::clicked, this, &TelegraphWindow::toggleTheme);
        connect(clearButton, &QPushButton::clicked, this, [this](){
            logDisplay->clear();
            chatDisplay->clear();
            writeToFile("--- LOGLAR TEMIZLENDI ---");
        });

        writeToFile("--- OTURUM BASLADI ---");
    }

    ~TelegraphWindow() {
        serialThread->quit();
        serialThread->wait();
    }

private:
    QBluetoothSocket *btSocket;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    
    QThread *serialThread;
    SerialWorker *worker;

    QComboBox *connectionTypeSelect;
    QComboBox *portSelect;
    QComboBox *baudSelect;
    QPushButton *connectButton;
    QPushButton *scanButton;
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
    bool isBtConnected = false;
    bool isUsbConnected = false;

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
        QString danger = "#f38ba8";

        QString style = QString(R"(
            QMainWindow { background-color: %1; }
            QWidget { color: %2; font-family: 'JetBrains Mono', 'Segoe UI', sans-serif; font-size: 13px; outline: none; }
            QGroupBox { border: 2px solid %4; border-radius: 16px; background-color: %3; }
            QLabel#boxTitle { background-color: %10; color: #000000; font-size: 14px; font-weight: 900; padding: 10px; border-radius: 6px; margin: 0px; }
            QLineEdit { background-color: %6; border: 2px solid %4; border-radius: 8px; padding: 8px; color: %2; selection-background-color: %5; }
            QLineEdit:focus { border: 2px solid %5; }
            QComboBox { background-color: %6; border: 2px solid %4; border-radius: 8px; padding: 8px 10px; color: %2; }
            QComboBox::down-arrow { width: 10px; height: 10px; background: none; border-left: 2px solid %2; border-bottom: 2px solid %2; transform: rotate(-45deg); margin-top: -3px; margin-left: -3px; }
            QTextEdit { background-color: %6; border: 2px solid %4; border-radius: 12px; padding: 10px; color: %2; }
            QPushButton { background-color: %7; color: %2; border: none; border-radius: 8px; padding: 10px 20px; font-weight: bold; outline: none; }
            QPushButton:hover { background-color: %8; color: #1e1e2e; }
            QPushButton#connectBtn { background-color: %5; color: %1; font-size: 15px; border-radius: 12px; }
            QPushButton#connectBtn:hover { background-color: %10; }
            QLabel#statusLabel { font-size: 14px; font-weight: bold; padding: 10px; border: 2px dashed %4; border-radius: 8px; margin-bottom: 10px; }
            QScrollBar:vertical { border: none; background: %1; width: 10px; margin: 0; border-radius: 5px; }
            QScrollBar::handle:vertical { background: %7; min-height: 20px; border-radius: 5px; }
        )")
        .arg(bgColor, fgColor, boxBg, boxBorder, accent, inputBg, btnBg, btnHover, danger, titleBg, titleFg);

        qApp->setStyleSheet(style);
    }

    void toggleTheme() {
        isDarkTheme = !isDarkTheme;
        themeButton->setText(isDarkTheme ? "AYDINLIK MODA GEC" : "KARANLIK MODA GEC");
        setupStyles(isDarkTheme);
        appendLog("KULLANICI: Tema degistirildi.");
    }

    void onConnectionTypeChanged(int index) {
        portSelect->clear();
        if (index == 0) { 
            scanButton->setText("BLUETOOTH TARA");
        } else { 
            scanButton->setText("PORTLARI YENILE");
            refreshUsbPorts();
        }
    }

    void handleScanOrRefresh() {
        if (connectionTypeSelect->currentIndex() == 0) {
            startBtDiscovery();
        } else {
            refreshUsbPorts();
            appendLog("SISTEM: USB portlari yenilendi.");
        }
    }

    void startBtDiscovery() {
        if(discoveryAgent->isActive()) return;
        portSelect->clear();
        scanButton->setEnabled(false);
        scanButton->setText("TARANIYOR...");
        discoveryAgent->start();
        appendLog("SISTEM: Bluetooth taramasi baslatildi...");
    }

    void deviceDiscovered(const QBluetoothDeviceInfo &device) {
        QString label = QString("%1 (%2)").arg(device.name(), device.address().toString());
        portSelect->addItem(label, QVariant::fromValue(device.address().toString()));
    }

    void btSocketConnected() {
        isBtConnected = true;
        updateUIConnectedState(true, "BLUETOOTH");
        appendLog("SISTEM: Bluetooth baglantisi basarili.");
    }

    void btSocketDisconnected() {
        isBtConnected = false;
        updateUIConnectedState(false, "");
        appendLog("SISTEM: Bluetooth baglantisi kesildi.");
    }

    void readBtSocketData() {
        while (btSocket->canReadLine()) {
            QByteArray data = btSocket->readLine().trimmed();
            QString line = QString::fromUtf8(data);
            processIncomingData(line);
        }
    }

    void refreshUsbPorts() {
        portSelect->clear();
        const auto infos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &info : infos) {
            portSelect->addItem(info.portName());
        }
    }

    void handleSerialConnectionStatus(bool connected, QString portName) {
        isUsbConnected = connected;
        if(connected) {
            updateUIConnectedState(true, "USB (" + portName + ")");
            appendLog("SISTEM: USB Seri baglanti basarili -> " + portName);
        } else {
            updateUIConnectedState(false, "");
            appendLog("SISTEM: USB baglantisi kapatildi veya koptu.");
        }
    }

    void handleSerialMessage(QString line) {
        processIncomingData(line);
    }

    void toggleConnection() {
        if (isBtConnected) {
            btSocket->disconnectFromService();
            return;
        }
        if (isUsbConnected) {
            emit operateCloseSerial();
            return;
        }

        int type = connectionTypeSelect->currentIndex();
        
        if (type == 0) { 
            QString addressStr = portSelect->currentData().toString();
            if (addressStr.isEmpty()) {
                QMessageBox::warning(this, "Hata", "Lutfen bir Bluetooth cihazi secin.");
                return;
            }
            QBluetoothAddress address(addressStr);
            btSocket->connectToService(address, QBluetoothUuid::ServiceClassUuid::SerialPort);
            connectButton->setText("BAGLANIYOR...");
            connectButton->setEnabled(false);
            appendLog("KULLANICI: BT Baglanti istegi -> " + addressStr);
        } 
        else { 
            QString portName = portSelect->currentText();
            if (portName.isEmpty()) {
                QMessageBox::warning(this, "Hata", "Lutfen bir Seri Port secin.");
                return;
            }
            int baud = baudSelect->currentText().toInt();
            emit operateOpenSerial(portName, baud);
            connectButton->setText("BAGLANIYOR...");
            connectButton->setEnabled(false);
            appendLog("KULLANICI: USB Baglanti istegi -> " + portName);
        }
    }

    void updateUIConnectedState(bool connected, QString typeInfo) {
        connectButton->setEnabled(true);
        if (connected) {
            connectButton->setText("BAGLANTIYI KES");
            connectButton->setStyleSheet("background-color: #f38ba8; color: #1e1e2e;");
            statusLabel->setText("DURUM: BAGLI - " + typeInfo);
            statusLabel->setStyleSheet("color: #a6e3a1; font-weight: bold; border: 2px solid #a6e3a1;");
            connectionTypeSelect->setEnabled(false);
            portSelect->setEnabled(false);
            baudSelect->setEnabled(false);
            scanButton->setEnabled(false);
        } else {
            connectButton->setText("BAGLAN");
            connectButton->setStyleSheet(""); 
            statusLabel->setText("DURUM: BAGLI DEGIL");
            statusLabel->setStyleSheet("color: #f38ba8; font-weight: bold; border: 2px dashed #f38ba8;");
            connectionTypeSelect->setEnabled(true);
            portSelect->setEnabled(true);
            baudSelect->setEnabled(true);
            scanButton->setEnabled(true);
        }
    }

    void sendPacket(QString type, QString payload) {
        if (!isBtConnected && !isUsbConnected) {
            QMessageBox::warning(this, "Uyari", "Once baglanti kurmalisiniz!");
            return;
        }

        QString raw = type + "," + payload;
        int checksum = 0;
        QByteArray bytes = raw.toLatin1();
        for(char c : bytes) {
            checksum ^= c;
        }

        QString packet = "$" + raw + "*" + QString::number(checksum, 16).toUpper() + "\r\n";

        if (isBtConnected) {
            btSocket->write(packet.toUtf8());
        } else if (isUsbConnected) {
            emit operateWriteSerial(packet);
        }
    }

    void sendMessage() {
        QString msg = messageInput->text();
        if (msg.isEmpty()) return;

        sendPacket("M", msg);
        appendChat(msg, true);
        appendLog("GONDERILDI ($M): " + msg);
        messageInput->clear();
    }

    void sendCustomCommand() {
        QString cmd = customCmdInput->text();
        if (cmd.isEmpty()) cmd = "PING"; 

        sendPacket("K", cmd);
        appendLog("KOMUT YOLLANDI ($K): " + cmd);
        customCmdInput->clear();
    }

    void processIncomingData(QString line) {
        if (line.isEmpty()) return;

        int starIndex = line.indexOf('*');
        if (starIndex != -1) {
             line = line.left(starIndex);
        }
        if (line.startsWith("$")) {
            line = line.mid(1);
        }

        if (line.startsWith("K,")) {
            handleSystemCommand(line);
            appendLog("GELEN KOMUT ($K): " + line);
        } 
        else if (line.startsWith("M,")) {
            QString msgContent = line.mid(2);
            appendChat(msgContent, false);
            appendLog("GELEN MESAJ ($M): " + msgContent);
        }
        else {
             appendLog("RAW: " + line);
        }
    }

    void handleSystemCommand(QString rawData) {
        QStringList parts = rawData.split(',');
        if (parts.size() < 2) return; 
        
        QString cmdKey = parts[1].toUpper(); 
        
        if (commandMap.contains(cmdKey)) {
            CommandConfig &cfg = commandMap[cmdKey];
            qint64 now = QDateTime::currentMSecsSinceEpoch();

            if (now - cfg.lastRunTime > cfg.offsetMs) {
                QProcess::startDetached(cfg.systemCommand);
                cfg.lastRunTime = now;
                appendLog("SISTEM EYLEMI BASLATILDI: " + cfg.systemCommand);
            } else {
                appendLog("SISTEM: " + cmdKey + " (Zaman asimi/Debounce)");
            }
        } else {
            appendLog("BILINMEYEN KOMUT: " + cmdKey);
        }
    }

    void appendLog(QString text) {
        QString currentDate = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss");

        if (currentDate != lastLogDate) {
            logDisplay->setAlignment(Qt::AlignCenter);
            logDisplay->append("\n--- " + currentDate + " ---\n");
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
            "<div align='%1' style='margin-bottom:10px;'>"
            "<span style='background-color:%2; color:%3; padding:10px; font-size:14px; text-decoration:none;'>"
            "%4"
            "<br><small style='color:%3; opacity:0.7; font-size:10px;'>%5</small>"
            "</span>"
            "</div>"
        ).arg(align).arg(bgColor).arg(textColor).arg(text).arg(timeStr);
        
        chatDisplay->append(html);
        chatDisplay->verticalScrollBar()->setValue(chatDisplay->verticalScrollBar()->maximum());
    }

signals:
    void operateOpenSerial(QString name, int baud);
    void operateCloseSerial();
    void operateWriteSerial(QString data);
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TelegraphWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"