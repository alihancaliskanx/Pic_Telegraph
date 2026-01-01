# 📠 Pic_Telegraph: Hybrid Morse Communication and Control System

**Pic_Telegraph** is a bi-directional communication and remote control project that combines traditional Morse code with modern wireless technologies. The system consists of a **PIC16F887** based hardware unit and a modern desktop control station developed with **Qt6**.

It does not only send messages; it can execute commands on your computer (opening browser, starting terminal, etc.) using Morse codes on the PIC, or control the PIC hardware (LED, Buzzer, Reset) from the computer.

---

## 🌟 Key Features

### 🔌 Hardware (PIC16F887)

* **Real-Time Morse Decoder:** Instantly distinguishes dots (`.`) and dashes (`-`) based on button duration and converts them to letters.
* **Dual-Mode Operation:**
1. **Message Mode:** Sends the typed text to the chat via Bluetooth.
2. **Command Mode:** Triggers system commands by sending shortcuts written in Morse (e.g., "BR") to the PC.


* **Smart NMEA Protocol:** Uses Checksum (`*CS`) protected `$M` (Message) and `$K` (Command) packet structure for data security.
* **EEPROM Memory:** Stores the last written message and data received from Bluetooth even if power is cut.
* **Scrolling Text (Ticker):** Displays received long messages as an animation on the bottom line of the 20x4 LCD screen.
* **Power Management:** Automatically switches to **Sleep Mode** when the system is idle.

### 💻 Software (Qt6 Desktop Interface)

* **Multi-Connection Support:** Can connect via both Bluetooth (RFCOMM) and USB Serial Port (UART).
* **Modern & Customizable UI:** * Dark and Light theme options.
    * **External Styling:** UI appearance can be customized via `style.css`.
* **Chat Interface:** Displays incoming and outgoing messages with timestamps.
* **Configurable Command Execution:** Detects commands coming from the PIC and executes them based on the `Command.json` configuration file.
* **System Logging:** Records all data traffic and errors to the `telegraph.log` file.
* **Keybindings:** Customizable keyboard shortcuts via `keybindings.conf`.

---

## 🛠 Technical Architecture and Protocol

The system uses a custom **NMEA-like** protocol between devices.

| Packet Type | Format | Description |
| --- | --- | --- |
| **Message Sending** | `$M,HELLO*5A` | Carries text message. |
| **Command Sending** | `$K,BR*XX` | Opens browser on PC. |
| **Hardware Control** | `$K,rst*XX` | Sends reset signal from PC to PIC. |

### Defined Commands on Desktop Side (Qt)

Commands are now defined in the `.config/Command.json` file. You can add your own shortcuts. Default examples:

* **BR:** Opens Google in the browser.
* **TERM:** Opens Terminal/Command prompt.
* **NV:** Opens Neovim editor.
* **UPD:** Starts system update.

---

## 🔌 Pinout Diagram (PIC16F887)

| Component | Pin | Description |
| --- | --- | --- |
| **LCD RS** | `PIN_D1` | LCD Register Select |
| **LCD EN** | `PIN_D3` | LCD Enable |
| **LCD DATA** | `PIN_D4-D7` | LCD 4-bit Data Bus |
| **BTN_SIGNAL** | `PIN_B0` | Morse Input (Dot/Dash) |
| **BTN_UPLOAD** | `PIN_B1` | Add Letter (Short) / Send (Long) |
| **BTN_DELETE** | `PIN_B2` | Delete Character (Backspace) |
| **BTN_RESET** | `PIN_B3` | Clear Text |
| **BTN_MODE** | `PIN_B4` | Change Mode (Message <-> Command) |
| **UART TX** | `PIN_C6` | Goes to Bluetooth RX Pin |
| **UART RX** | `PIN_C7` | Goes to Bluetooth TX Pin |
| **LED** | `PIN_A0` | Status LED |
| **BUZZER** | `PIN_A1` | Audio Feedback |

---

## 🚀 Installation and Usage

### 1. Hardware Preparation

1. Assemble the circuit according to the schematic.
2. Compile the `Pic_Telegraph/src/main.c` file with **CCS C Compiler**.
3. Upload the generated `.hex` file to the PIC16F887.

### 2. Desktop Application (Windows/Linux)

1. Ensure **Qt6** and **CMake** are installed.
2. Open the terminal and navigate to the `ui` folder:
```bash
cd ui
bash build.sh
```

*Note: The build process will automatically copy the configuration files (`Command.json`, `style.css`, etc.) to the executable directory.*

3. Run the `TelgrafApp` application.

### 3. Usage Steps

* **Typing Morse:** Create a dot with a short press and a dash with a long press on the signal button (B0).
* **Confirming Letter:** Short press the B1 button to add the character to the text.
* **Sending Message:** **Long** press the B1 button when the message is finished to send it via Bluetooth.
* **Changing Mode:** Press the B4 button to switch to "MODE: COMMAND" screen to send commands to the PC instead of writing messages.

---

## 📂 Project Structure

```
Pic_Telegraph/
├── src/                  # PIC16F887 Embedded Software (CCS C)
│   ├── main.c            # Main source code
│   └── ...
├── ui/                   # Desktop Control Software (Qt6 C++)
│   ├── .config/          # Configuration files (Commands, Styles, Keys)
│   ├── main.cpp          # Main application and UI code
│   ├── CMakeLists.txt    # Qt Build configuration
│   └── ...
├── Library/              # Required DLL and Proteus libraries
└── Readme.md             # Project Documentation

```

---

## 🤝 Contributing

This project was developed for educational and hobby purposes. Feel free to send a "Pull Request" for bug fixes or new feature suggestions.

**Developer:** Alihan
