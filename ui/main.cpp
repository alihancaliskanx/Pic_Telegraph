#include <QSettings>
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDir>
#include <QCoreApplication>
#include <QKeyEvent>

// Struct to hold configuration for system commands
struct CommandConfig {
    QString systemCommand;  // The actual OS command to execute
    qint64 lastRunTime;     // Timestamp of the last execution
    int offsetMs;           // Debounce/Cooldown time in milliseconds
};

// Worker class to handle Serial Port operations in a separate thread
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
    // Opens the serial port with specified parameters
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
            emit errorOccurred("SERIAL PORT ERROR: " + serialPort->errorString());
        }
    }

    // Closes the serial port
    void closePort() {
        if(serialPort->isOpen()) {
            serialPort->close();
            disconnect(serialPort, &QSerialPort::readyRead, this, &SerialWorker::readData);
        }
        emit connectionStatusChanged(false, "");
    }

    // Writes data to the open serial port
    void writeData(QString data) {
        if(serialPort->isOpen()) {
            serialPort->write(data.toUtf8());
        }
    }

    // Reads incoming data line by line
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

// Main Application Window
class TelegraphWindow : public QMainWindow {
    Q_OBJECT

public:
    TelegraphWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("PIC CONTROL STATION V3");
        resize(1200, 750);

        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setSpacing(25);
        mainLayout->setContentsMargins(25, 25, 25, 25);

        // Initialize UI styling (default: Dark Mode)
        setupStyles(true);

        // --- Settings Group Box ---
        QGroupBox *settingsGroup = new QGroupBox();
        QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);
        settingsGroup->setFixedWidth(340);
        settingsLayout->setContentsMargins(15, 15, 15, 15);
        settingsLayout->setSpacing(12);

        QLabel *settingsTitle = new QLabel("CONNECTION & CONTROL");
        settingsTitle->setAlignment(Qt::AlignCenter);
        settingsTitle->setObjectName("boxTitle");
        settingsLayout->addWidget(settingsTitle);
        settingsLayout->addSpacing(10);

        themeButton = new QPushButton("SWITCH TO LIGHT MODE");
        themeButton->setCursor(Qt::PointingHandCursor);

        connectionTypeSelect = new QComboBox();
        connectionTypeSelect->addItem("BLUETOOTH SERIAL");
        connectionTypeSelect->addItem("USB SERIAL");

        portSelect = new QComboBox();
        
        baudSelect = new QComboBox();
        QStringList bauds = {"9600", "38400", "57600", "115200"};
        baudSelect->addItems(bauds);

        customCmdInput = new QLineEdit();
        customCmdInput->setPlaceholderText("Custom Command");
        
        sendCmdButton = new QPushButton("SEND COMMAND");
        sendCmdButton->setCursor(Qt::PointingHandCursor);
        sendCmdButton->setFixedHeight(40);

statusLabel = new QLabel("STATUS: DISCONNECTED");
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setObjectName("statusLabel");
        statusLabel->setFixedHeight(50);

        connectButton = new QPushButton("CONNECT");
        connectButton->setCursor(Qt::PointingHandCursor);
        connectButton->setFixedHeight(50);
        connectButton->setObjectName("connectBtn");

        scanButton = new QPushButton("SCAN BLUETOOTH");
        scanButton->setFixedHeight(40);
        scanButton->setCursor(Qt::PointingHandCursor);

        // Add widgets to settings layout
        settingsLayout->addWidget(themeButton);
        settingsLayout->addSpacing(10);
        
        settingsLayout->addWidget(new QLabel("CONNECTION TYPE"));
        settingsLayout->addWidget(connectionTypeSelect);

        settingsLayout->addWidget(new QLabel("DEVICE / PORT SELECTION"));
        settingsLayout->addWidget(portSelect);
        settingsLayout->addWidget(new QLabel("BAUDRATE"));
        settingsLayout->addWidget(baudSelect);
        settingsLayout->addWidget(scanButton);
        
        settingsLayout->addSpacing(20);
        settingsLayout->addWidget(new QLabel("MANUAL COMMAND"));
        settingsLayout->addWidget(customCmdInput);
        settingsLayout->addWidget(sendCmdButton);

        settingsLayout->addStretch();
        settingsLayout->addWidget(statusLabel);
        settingsLayout->addWidget(connectButton);

        // --- Chat Group Box ---
        QGroupBox *chatGroup = new QGroupBox();
        QVBoxLayout *chatLayout = new QVBoxLayout(chatGroup);
        chatLayout->setContentsMargins(15, 15, 15, 15);
        
        QLabel *chatTitle = new QLabel("MESSAGING");
        chatTitle->setAlignment(Qt::AlignCenter);
        chatTitle->setObjectName("boxTitle");
        chatLayout->addWidget(chatTitle);
        chatLayout->addSpacing(10);

        chatDisplay = new QTextEdit();
        chatDisplay->setReadOnly(true);
        chatDisplay->setFocusPolicy(Qt::NoFocus);

        messageInput = new QLineEdit();
        messageInput->setPlaceholderText("Type a message...");
        messageInput->setFixedHeight(45);

        chatLayout->addWidget(chatDisplay);
        chatLayout->addSpacing(15); 
        chatLayout->addWidget(messageInput);

        // --- Logs Group Box ---
        QGroupBox *logGroup = new QGroupBox();
        QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
        logGroup->setFixedWidth(360);
        logLayout->setContentsMargins(15, 15, 15, 15);

        QLabel *logTitle = new QLabel("SYSTEM LOGS");
        logTitle->setAlignment(Qt::AlignCenter);
        logTitle->setObjectName("boxTitle");
        logLayout->addWidget(logTitle);
        logLayout->addSpacing(10);

        logDisplay = new QTextEdit();
        logDisplay->setReadOnly(true);

        QPushButton *clearButton = new QPushButton("CLEAR LOGS");
        clearButton->setCursor(Qt::PointingHandCursor);
        clearButton->setFixedHeight(45);

        logLayout->addWidget(logDisplay);
        logLayout->addSpacing(15); 
        logLayout->addWidget(clearButton);

        // Add groups to main layout
        mainLayout->addWidget(settingsGroup);
        mainLayout->addWidget(chatGroup, 1);
        mainLayout->addWidget(logGroup);

        // Initialize Bluetooth and Serial components
        btSocket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
        discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

        serialThread = new QThread(this);
        worker = new SerialWorker();
        worker->moveToThread(serialThread);

        // Connect threading signals
        connect(serialThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &TelegraphWindow::operateOpenSerial, worker, &SerialWorker::openPort);
        connect(this, &TelegraphWindow::operateCloseSerial, worker, &SerialWorker::closePort);
        connect(this, &TelegraphWindow::operateWriteSerial, worker, &SerialWorker::writeData);
        
        connect(worker, &SerialWorker::messageReceived, this, &TelegraphWindow::handleSerialMessage);
        connect(worker, &SerialWorker::connectionStatusChanged, this, &TelegraphWindow::handleSerialConnectionStatus);
        connect(worker, &SerialWorker::errorOccurred, this, &TelegraphWindow::appendLog);

        serialThread->start();

        // Load commands from external JSON file
        loadSystemCommands(); 

        // Connect UI Signals
        connect(connectionTypeSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TelegraphWindow::onConnectionTypeChanged);
        connect(connectButton, &QPushButton::clicked, this, &TelegraphWindow::toggleConnection);
        connect(scanButton, &QPushButton::clicked, this, &TelegraphWindow::handleScanOrRefresh);
        
        // Bluetooth Discovery Signals
        connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &TelegraphWindow::deviceDiscovered);
        connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, [this](){
            scanButton->setEnabled(true);
            scanButton->setText("SCAN BLUETOOTH");
            appendLog("SYSTEM: Scan complete.");
        });

        // Bluetooth Socket Signals
        connect(btSocket, &QBluetoothSocket::connected, this, &TelegraphWindow::btSocketConnected);
        connect(btSocket, &QBluetoothSocket::disconnected, this, &TelegraphWindow::btSocketDisconnected);
        connect(btSocket, &QBluetoothSocket::readyRead, this, &TelegraphWindow::readBtSocketData);
        
        // Input Handling
        connect(messageInput, &QLineEdit::returnPressed, this, &TelegraphWindow::sendMessage);
        connect(sendCmdButton, &QPushButton::clicked, this, &TelegraphWindow::sendCustomCommand);
        connect(themeButton, &QPushButton::clicked, this, &TelegraphWindow::toggleTheme);
        connect(clearButton, &QPushButton::clicked, this, [this](){
            logDisplay->clear();
            chatDisplay->clear();
            writeToFile("--- LOGS CLEARED ---");
        });

        writeToFile("--- SESSION STARTED ---");
        
        qApp->installEventFilter(this); 

        loadKeyBindings();

        loadSettings();
    }

    ~TelegraphWindow() {
        saveSettings();
        
        serialThread->quit();
        serialThread->wait();
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

            QWidget *currentFocus = QApplication::focusWidget();
            
            bool isTyping = (qobject_cast<QLineEdit*>(currentFocus) || qobject_cast<QTextEdit*>(currentFocus));
            
            if (keyEvent->key() == keyClearFocus) {
                if (currentFocus && isTyping) {
                    currentFocus->clearFocus();
                    return true;
                }
            }

            if (keyEvent->matches(QKeySequence::Copy) || (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_C)) {
                bool hasSelection = false;
                
                if (QLineEdit *le = qobject_cast<QLineEdit*>(currentFocus)) {
                    hasSelection = le->hasSelectedText();
                } else if (QTextEdit *te = qobject_cast<QTextEdit*>(currentFocus)) {
                    hasSelection = te->textCursor().hasSelection();
                }

                if (!hasSelection) {
                    logDisplay->clear();
                    chatDisplay->clear();
                    writeToFile("--- LOGS CLEARED (Shortcut) ---");
                    appendLog("SYSTEM: Logs cleared via shortcut.");
                    return true;
                }
            }

            if (!isTyping) {
                if (keyEvent->text().toUpper() == keyFocusCmd) {
                    customCmdInput->setFocus();
                    return true;
                }
                if (keyEvent->text().toUpper() == keyFocusMsg) {
                    messageInput->setFocus();
                    return true;
                }
            }
        }
        return QMainWindow::eventFilter(obj, event);
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
    QString keyFocusCmd;
    QString keyFocusMsg;
    int keyClearFocus;
    
    QMap<QString, CommandConfig> commandMap;
    QString lastLogDate;
    bool isDarkTheme = true;
    bool isBtConnected = false;
    bool isUsbConnected = false;

    // Setups the CSS stylesheet for the application (Light/Dark mode)
    void setupStyles(bool dark) {
        // Renk Tanımları
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

        QString stylePath = QCoreApplication::applicationDirPath() + "/.config/style.css";
        QFile file(stylePath);
        
        QString styleContent;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            styleContent = QString::fromUtf8(file.readAll());
            file.close();
        } else {
            appendLog("STYLE ERROR: CSS file not found at " + stylePath);
            return; 
        }

        QString finalStyle = styleContent.arg(
            bgColor, fgColor, boxBg, boxBorder, accent, 
            inputBg, btnBg, btnHover, danger, titleBg, titleFg
        );

        qApp->setStyleSheet(finalStyle);
    }

    void loadKeyBindings() {
        QString configPath = QCoreApplication::applicationDirPath() + "/.config/keybindings.conf";
        QSettings settings(configPath, QSettings::IniFormat);

        // Varsayılan değerler dosyada yoksa "K", "M", "Esc"
        keyFocusCmd = settings.value("Shortcuts/FocusCommandInput", "K").toString().toUpper();
        keyFocusMsg = settings.value("Shortcuts/FocusMessageInput", "M").toString().toUpper();
        
        QString escString = settings.value("Shortcuts/ClearFocus", "Esc").toString();
        QKeySequence seq(escString);
        if (!seq.isEmpty()) {
            keyClearFocus = seq[0].key();
        } else {
            keyClearFocus = Qt::Key_Escape;
        }

        appendLog("SYSTEM: Keybindings loaded.");
    }

    void loadSettings() {
        QString configPath = QCoreApplication::applicationDirPath() + "/.config/TelgrafApp.conf";

        QSettings settings(configPath, QSettings::IniFormat);

        bool savedThemeIsDark = settings.value("UI/ThemeIsDark", true).toBool();
        if (savedThemeIsDark != isDarkTheme) {
            toggleTheme(); 
        }

        int connType = settings.value("Connection/Type", 0).toInt();
        connectionTypeSelect->setCurrentIndex(connType);

        QString savedBaud = settings.value("Connection/Baud", "9600").toString();
        baudSelect->setCurrentText(savedBaud);

        QString savedPort = settings.value("Connection/LastPort", "").toString();
        int portIndex = portSelect->findText(savedPort);
        if (portIndex != -1) {
            portSelect->setCurrentIndex(portIndex);
        }

        appendLog("SYSTEM: Settings loaded from config");
    }

    void saveSettings() {
        QString configPath = QCoreApplication::applicationDirPath() + "/.config/TelgrafApp.conf";
        QSettings settings(configPath, QSettings::IniFormat);

        settings.setValue("UI/ThemeIsDark", isDarkTheme);

        settings.setValue("Connection/Type", connectionTypeSelect->currentIndex());
        settings.setValue("Connection/Baud", baudSelect->currentText());
        settings.setValue("Connection/LastPort", portSelect->currentText());

        settings.sync();
    }

    void toggleTheme() {
        isDarkTheme = !isDarkTheme;
        themeButton->setText(isDarkTheme ? "SWITCH TO LIGHT MODE" : "SWITCH TO DARK MODE");
        setupStyles(isDarkTheme);
        appendLog("USER: Theme changed.");
    }

    void onConnectionTypeChanged(int index) {
        portSelect->clear();
        if (index == 0) { // Bluetooth
            scanButton->setText("SCAN BLUETOOTH");
        } else { // USB
            scanButton->setText("REFRESH PORTS");
            refreshUsbPorts();
        }
    }

    void handleScanOrRefresh() {
        if (connectionTypeSelect->currentIndex() == 0) {
            startBtDiscovery();
        } else {
            refreshUsbPorts();
            appendLog("SYSTEM: USB ports refreshed.");
        }
    }

    void startBtDiscovery() {
        if(discoveryAgent->isActive()) return;
        portSelect->clear();
        scanButton->setEnabled(false);
        scanButton->setText("SCANNING...");
        discoveryAgent->start();
        appendLog("SYSTEM: Bluetooth scan started...");
    }

    void deviceDiscovered(const QBluetoothDeviceInfo &device) {
        QString label = QString("%1 (%2)").arg(device.name(), device.address().toString());
        portSelect->addItem(label, QVariant::fromValue(device.address().toString()));
    }

    void btSocketConnected() {
        isBtConnected = true;
        updateUIConnectedState(true, "BLUETOOTH");
        appendLog("SYSTEM: Bluetooth connection successful.");
    }

    void btSocketDisconnected() {
        isBtConnected = false;
        updateUIConnectedState(false, "");
        appendLog("SYSTEM: Bluetooth connection disconnected.");
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
            appendLog("SYSTEM: USB Serial connection successful -> " + portName);
        } else {
            updateUIConnectedState(false, "");
            appendLog("SYSTEM: USB connection closed or lost.");
        }
    }

    void handleSerialMessage(QString line) {
        processIncomingData(line);
    }

    // Handles the Connect/Disconnect button logic
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
        
        if (type == 0) { // Bluetooth
            QString addressStr = portSelect->currentData().toString();
            if (addressStr.isEmpty()) {
                QMessageBox::warning(this, "Error", "Please select a Bluetooth device.");
                return;
            }
            QBluetoothAddress address(addressStr);
            btSocket->connectToService(address, QBluetoothUuid::ServiceClassUuid::SerialPort);
            connectButton->setText("CONNECTING...");
            connectButton->setEnabled(false);
            appendLog("USER: BT Connection request -> " + addressStr);
        } 
        else { // USB
            QString portName = portSelect->currentText();
            if (portName.isEmpty()) {
                QMessageBox::warning(this, "Error", "Please select a Serial Port.");
                return;
            }
            int baud = baudSelect->currentText().toInt();
            emit operateOpenSerial(portName, baud);
            connectButton->setText("CONNECTING...");
            connectButton->setEnabled(false);
            appendLog("USER: USB Connection request -> " + portName);
        }
    }

    // Updates the UI elements based on connection state
    void updateUIConnectedState(bool connected, QString typeInfo) {
        connectButton->setEnabled(true);
        if (connected) {
            connectButton->setText("DISCONNECT");
            connectButton->setStyleSheet("background-color: #f38ba8; color: #1e1e2e;");
            statusLabel->setText("STATUS: CONNECTED - " + typeInfo);
            statusLabel->setStyleSheet("color: #a6e3a1; font-weight: bold; border: 2px solid #a6e3a1;");
            connectionTypeSelect->setEnabled(false);
            portSelect->setEnabled(false);
            baudSelect->setEnabled(false);
            scanButton->setEnabled(false);
        } else {
            connectButton->setText("CONNECT");
            connectButton->setStyleSheet(""); 
            statusLabel->setText("STATUS: DISCONNECTED");
            statusLabel->setStyleSheet("color: #f38ba8; font-weight: bold; border: 2px dashed #f38ba8;");
            connectionTypeSelect->setEnabled(true);
            portSelect->setEnabled(true);
            baudSelect->setEnabled(true);
            scanButton->setEnabled(true);
        }
    }

    // Constructs and sends a data packet with checksum
    void sendPacket(QString type, QString payload) {
        if (!isBtConnected && !isUsbConnected) {
            QMessageBox::warning(this, "Warning", "You must connect first!");
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
        appendLog("SENT ($M): " + msg);
        messageInput->clear();
    }

    void sendCustomCommand() {
        QString cmd = customCmdInput->text();
        if (cmd.isEmpty()) cmd = "PING"; 

        sendPacket("K", cmd);
        appendLog("COMMAND SENT ($K): " + cmd);
        customCmdInput->clear();
    }

    // Parses incoming raw data strings
    void processIncomingData(QString line) {
        if (line.isEmpty()) return;

        // Strip checksum and prefix
        int starIndex = line.indexOf('*');
        if (starIndex != -1) {
             line = line.left(starIndex);
        }
        if (line.startsWith("$")) {
            line = line.mid(1);
        }

        if (line.startsWith("K,")) {
            handleSystemCommand(line);
            appendLog("INCOMING COMMAND ($K): " + line);
        } 
        else if (line.startsWith("M,")) {
            QString msgContent = line.mid(2);
            appendChat(msgContent, false);
            appendLog("INCOMING MESSAGE ($M): " + msgContent);
        }
        else {
             appendLog("RAW: " + line);
        }
    }

    // Executes system commands based on received data
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
                appendLog("SYSTEM ACTION STARTED: " + cfg.systemCommand);
            } else {
                appendLog("SYSTEM: " + cmdKey + " (Timeout/Debounce)");
            }
        } else {
            appendLog("UNKNOWN COMMAND: " + cmdKey);
        }
    }

    // Appends text to the log window and file
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
        QFile file("telegraph.log");
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << text << "\n";
            file.close();
        }
    }

    // Appends message to the chat display with HTML styling
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

    void loadSystemCommands() {
        QString configPath = QCoreApplication::applicationDirPath() + "/.config/Command.json";
        QFile file(configPath);

        if (!file.open(QIODevice::ReadOnly)) {
            appendLog("SYSTEM ERROR: Config file not found at " + configPath);
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            appendLog("SYSTEM ERROR: Config JSON is invalid.");
            return;
        }

        QJsonObject root = doc.object();
        commandMap.clear();

        for (const QString &key : root.keys()) {
            QJsonObject cmdObj = root.value(key).toObject();
            
            CommandConfig config;
            config.systemCommand = cmdObj["cmd"].toString();
            config.offsetMs = cmdObj["timeout"].toInt(2000);
            config.lastRunTime = 0;

            commandMap.insert(key, config);
        }
        appendLog("SYSTEM: " + QString::number(commandMap.size()) + " commands loaded from config.");
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