# 📠 Akıllı Mors Telgraf ve Bluetooth Terminali (PIC16F887)

Bu proje, PIC16F887 mikrodenetleyicisi kullanılarak geliştirilmiş, **Bluetooth destekli, hibrit bir Mors Alfabesi Çözücü ve Mesajlaşma Sistemi**dir.

Sistem, kullanıcının girdiği Mors kodlarını anlık olarak metne çevirir, hafızaya kaydeder ve Bluetooth üzerinden bağlı bir cihaza (Telefon/PC) gönderir. Aynı zamanda, karşı taraftan gelen mesajları **kayan yazı animasyonu** ile LCD ekranda gösterir.

## 🌟 Öne Çıkan Özellikler

- **Çift Yönlü Haberleşme:** Hem Mors vericisi (TX) hem de Bluetooth terminal alıcısı (RX) olarak çalışır.
- **Gerçek Zamanlı Çözümleme:** Butona basılma sürelerine göre nokta (`.`) ve çizgileri (`-`) anında ayırt eder ve harfe dönüştürür.
- **Akıllı Gönderme (Smart Upload):** Tek buton ile iki işlev:
  - _Kısa Basış:_ Harfi hafızaya ekle.
  - _Uzun Basış:_ Yazılan tüm cümleyi NMEA formatında Bluetooth ile gönder.
- **Kayan Yazı (Ticker Tape):** Bluetooth üzerinden gelen mesajlar, uzunluğuna bakılmaksızın ekranın en alt satırında sağdan sola kayarak okunur.
- **Kalıcı Hafıza (EEPROM):**
  - Yazılan mesajlar ve gelen Bluetooth verileri enerji kesilse bile silinmez.
  - Cihaz yeniden açıldığında kaldığı yerden devam eder.
- **Tam Format (Hard Reset):** Tek tuşla cihazdaki tüm yazılı metinleri ve gelen kutusunu (Bluetooth tamponu) kalıcı olarak siler.
- **Güç Tasarrufu:** Cihaz belirli bir süre işlem görmezse otomatik olarak **Uyku Moduna** geçer.

## 🛠 Donanım Bileşenleri

| Bileşen      | Model/Tip    | Açıklama                                      |
| :----------- | :----------- | :-------------------------------------------- |
| **MCU**      | PIC16F887    | 8-bit Mikrodenetleyici (8MHz Dahili Osilatör) |
| **Kablosuz** | HC-06        | Bluetooth Seri Modülü (Slave)                 |
| **Ekran**    | LCD 20x4     | HD44780 Sürücülü Karakter LCD                 |
| **Giriş**    | 5x Buton     | Pull-up dirençli giriş butonları              |
| **Uyarı**    | Buzzer & LED | İşitsel ve görsel geri bildirim               |

## 🔌 Pin Bağlantı Şeması

| Buton/Bileşen      | PIC Pini | İşlev                                     |
| :----------------- | :------- | :---------------------------------------- |
| **BTN_SIGNAL**     | `PIN_B0` | Mors sinyali girişi (Nokta/Çizgi)         |
| **BTN_UPLOAD**     | `PIN_B1` | Harf Ekleme (Kısa) / Veri Gönderme (Uzun) |
| **BTN_DELETE**     | `PIN_B2` | Silme (Backspace)                         |
| **BTN_RESET**      | `PIN_B3` | Ekranı temizle (Soft Reset)               |
| **BTN_HARD_RESET** | `PIN_B4` | **Format Atma** (Tüm EEPROM'u Siler)      |
| **LED**            | `PIN_A0` | Sinyal durumu                             |
| **BUZZER**         | `PIN_A1` | Tuş ve uyarı sesleri                      |
| **HC-06 RX**       | `PIN_C6` | Veri Gönderme (TX)                        |
| **HC-06 TX**       | `PIN_C7` | Veri Alma (RX)                            |

## 🚀 Nasıl Kullanılır?

### 1. Mesaj Yazma (Mors Girişi)

- **Sinyal Butonu (B0):** Kısa basışlar `.`, uzun basışlar `-` olarak algılanır.
- Karakterler anlık olarak ekranın 3. satırında önizlenir.

### 2. Harf Ekleme ve Gönderme

- **Harf Ekleme:** `BTN_UPLOAD` (B1) butonuna **kısa** bastığınızda, önizlenen Mors kodu harfe çevrilip ana metne eklenir.
- **Gönderme:** Aynı butona **uzun** bastığınızda, yazılan mesaj Bluetooth üzerinden gönderilir ve ekran temizlenir.

### 3. Gelen Mesajları Okuma

- Telefon veya PC'den gönderilen mesajlar otomatik olarak algılanır.
- Mesajlar ekranın **4. satırında** belirir. Eğer mesaj ekrana sığmıyorsa otomatik olarak kaymaya başlar.

### 4. Silme ve Sıfırlama

- **Silme (B2):** Yanlış girilen bir nokta/çizgiyi veya son eklenen harfi siler.
- **Soft Reset (B3):** O anki ekranı temizler ancak hafızayı tamamen silmez.
- **Hard Reset (B4):** Butona basılı tuttuğunuzda "FORMAT ATILIYOR" uyarısı gelir. Bu işlem **giden mesajları ve gelen Bluetooth geçmişini** tamamen temizler (EEPROM Wipe).

## 💻 Seri Port (UART) Ayarları

Bluetooth modülü ile iletişim kurarken şu ayarları kullanın:

- **Baud Rate:** 9600
- **Data Bits:** 8
- **Parity:** None (Yok)
- **Stop Bit:** 1

---

_Proje Geliştiricisi: Alihan_
_Son Güncelleme: Aralık 2025_
