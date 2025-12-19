# Canlı Telgraf ve Bluetooth Terminali (PIC16F887)

Bu proje, PIC16F887 mikrodenetleyicisi kullanılarak geliştirilmiş, **Bluetooth destekli bir Mors Alfabesi Çözücü ve Mesajlaşma Sistemi**dir. Kullanıcı tarafından girilen Mors kodlarını metne çevirir, LCD ekranda gösterir, EEPROM hafızasına kaydeder ve Bluetooth üzerinden bağlı bir cihaza (Telefon/PC) gönderir. Ayrıca karşı taraftan gelen mesajları **kayan yazı animasyonu** ile ekranda görüntüler.

## 📋 Özellikler

- **Çift Yönlü İletişim:** Hem Mors kodlarını metne çevirip yollar hem de Bluetooth üzerinden gelen mesajları alır.
- **Canlı Çeviri:** Girilen nokta (`.`) ve çizgileri (`-`) eş zamanlı analiz eder.
- **Akıllı Gönderme (Smart Upload):** Tek buton ile hem harf ekleme hem de veri gönderme özelliği (Kısa/Uzun basış algılama).
- **Kayan Yazı Animasyonu (Ticker Tape):** Bluetooth üzerinden gelen uzun mesajlar, LCD'nin 4. satırında sürekli kayarak okunabilir hale gelir (Timer0 kontrollü).
- **EEPROM Hafıza Desteği:**
  - Yazılan metinler (Adres 0-20).
  - Bluetooth'tan gelen son mesaj (Adres 50+).
  - Enerji kesilse bile veriler silinmez.
- **Otomatik Uyku Modu:** Cihaz boşta kaldığında güç tasarrufu moduna geçer, herhangi bir tuşla uyanır.
- **Kesme (Interrupt) Tabanlı Alım:** Veri geldiği anda ana döngüyü aksatmadan arka planda yakalar (`INT_RDA`).

## 🛠 Donanım Gereksinimleri

- **Mikrodenetleyici:** PIC16F887
- **Kablosuz Modül:** HC-06 Bluetooth Modülü
- **Ekran:** HD44780 uyumlu LCD Ekran (20x4)
- **Giriş:** 5 Adet Push Buton
- **Çıkış:** 1x LED, 1x Buzzer (Aktif Buzzer)
- **Osilatör:** Dahili Osilatör (8MHz - `INTRC_IO`)

## 🔌 Pin Bağlantı Şeması

| Bileşen       | Pin Adı        | PIC Pini | Açıklama                                |
| ------------- | -------------- | -------- | --------------------------------------- |
| **Girişler**  | BTN_SIGNAL     | PIN_B0   | Sinyal gönderme tuşu (Mors girişi)      |
|               | BTN_UPLOAD     | PIN_B1   | Harf Ekleme (Kısa) / BT Gönderme (Uzun) |
|               | BTN_DELETE     | PIN_B2   | Silme tuşu (Karakter veya Harf)         |
|               | BTN_RESET      | PIN_B3   | Ekranı ve Tamponu Temizle (Soft Reset)  |
|               | BTN_HARD_RESET | PIN_B4   | EEPROM Formatlama (Hard Reset)          |
| **Çıkışlar**  | LED            | PIN_A0   | Sinyal LED'i                            |
|               | BUZZER         | PIN_A1   | Sinyal Sesi                             |
| **Bluetooth** | HC-06 TX       | PIN_C7   | PIC RX (Veri Alma)                      |
|               | HC-06 RX       | PIN_C6   | PIC TX (Veri Gönderme)                  |
| **LCD**       | RS             | PIN_D1   | Register Select                         |
|               | RW             | PIN_D2   | Read/Write                              |
|               | EN             | PIN_D3   | Enable                                  |
|               | DATA 4-7       | D4-D7    | LCD Data Yolları                        |

## 🚀 Kullanım Kılavuzu

### 1. Mors Girişi (BTN_SIGNAL - B0)

- Kısa basışlar nokta (`.`), uzun basışlar çizgi (`-`) olarak algılanır.
- Yazılan kod ekranda `<` işaretinin yanında önizlenir.

### 2. Harf Ekleme ve Gönderme (BTN_UPLOAD - B1)

Bu buton **çift işlevlidir**:

- **Kısa Basış (< 0.5 sn):** Yazılan Mors kodunu harfe çevirir ve ekrana/hafızaya ekler.
- **Uzun Basış (> 0.5 sn):** Oluşturulan tüm cümleyi **Bluetooth üzerinden** (HC-06) bağlı cihaza gönderir ("VERI GONDERILDI" uyarısı verir).

### 3. Bluetooth Mesaj Okuma (Otomatik)

- Telefondan veya bilgisayardan gönderilen veriler (`\`, `\n` veya `\r` ile bitenler) otomatik algılanır.
- Gelen mesaj LCD'nin **4. satırında** belirir.
- Mesaj ekrana sığmazsa, **otomatik olarak sağdan sola doğru kaymaya başlar.**
- Son gelen mesaj otomatik olarak EEPROM'a kaydedilir.

### 4. Silme (BTN_DELETE - B2)

- Yazım hatası yapıldığında son girilen Mors karakterini siler.
- Mors tamponu boşsa, yazılmış olan son harfi metinden siler (Backspace).

### 5. Format Atma (BTN_HARD_RESET - B4)

- Basılı tutulduğunda EEPROM'daki **hem gönderilecek metni hem de gelen Bluetooth mesajını** tamamen siler.
- Cihazı fabrika ayarlarına döndürür.

## 💻 Haberleşme Ayarları (HC-06)

Bluetooth terminal uygulamanızda aşağıdaki ayarları kullanmalısınız:

- **Baud Rate:** 9600
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1

---

_Geliştirici: Alihan_
_Düzenleme Tarihi: 2025_
