# Canlı Telgraf (PIC16F887)

Bu proje, PIC16F887 mikrodenetleyicisi kullanılarak geliştirilmiş bir **Mors Alfabesi Çözücü ve Kayıt Sistemi**dir. Kullanıcı tarafından girilen nokta/çizgi kombinasyonlarını anlık olarak metne çevirir, LCD ekranda gösterir ve **Dahili EEPROM** hafızasına kaydeder. Ayrıca enerji tasarrufu için otomatik uyku modu özelliği bulunmaktadır.

## 📋 Özellikler

- **Canlı Çeviri:** Girilen nokta (`.`) ve çizgileri (`-`) eş zamanlı olarak analiz eder ve karşılık gelen harfi bulur.
- **EEPROM Hafıza Desteği:** Yazılan metinleri mikrodenetleyicinin kalıcı hafızasına (EEPROM) kaydeder. Enerji kesilse bile metin silinmez.
- **Otomatik Uyku Modu (Sleep Mode):** Cihaz boşta kaldığında otomatik olarak düşük güç tüketim moduna geçer. Herhangi bir tuşa basıldığında uyanır.
- **WDT (Watchdog Timer):** Sistemin kilitlenmesini önlemek için Watchdog Timer koruması aktiftir.
- **LCD Arayüzü:** Girilen Mors kodunu, çevrilen metni ve sistem durumunu gösterir.
- **Geri Bildirim:** Her tuş basımında LED ve Buzzer ile görsel/işitsel geri bildirim sağlar.
- **Formatlama (Hard Reset):** Tek bir tuş ile hafızayı tamamen temizleme özelliği.

## 🛠 Donanım Gereksinimleri

- **Mikrodenetleyici:** PIC16F887
- **Ekran:** HD44780 uyumlu LCD Ekran (20x4 veya 16x2)
- **Giriş:** 5 Adet Push Buton
- **Çıkış:** 1x LED, 1x Buzzer (Aktif Buzzer)
- **Osilatör:** Dahili Osilatör (8MHz - `INTRC_IO`)

## 🔌 Pin Bağlantı Şeması

| Bileşen      | Pin Adı        | PIC Pini | Açıklama                               |
| ------------ | -------------- | -------- | -------------------------------------- |
| **Girişler** | BTN_SIGNAL     | PIN_B0   | Sinyal gönderme tuşu (Mors girişi)     |
|              | BTN_UPLOAD     | PIN_B1   | Onaylama ve Hafızaya Kaydetme          |
|              | BTN_DELETE     | PIN_B2   | Silme tuşu (Karakter veya Harf)        |
|              | BTN_RESET      | PIN_B3   | Ekranı ve Tamponu Temizle (Soft Reset) |
|              | BTN_HARD_RESET | PIN_B4   | EEPROM Formatlama (Hard Reset)         |
| **Çıkışlar** | LED            | PIN_A0   | Sinyal LED'i                           |
|              | BUZZER         | PIN_A1   | Sinyal Sesi                            |
| **LCD**      | RS             | PIN_D1   | Register Select                        |
|              | RW             | PIN_D2   | Read/Write                             |
|              | EN             | PIN_D3   | Enable                                 |
|              | DATA 4-7       | D4-D7    | LCD Data Yolları                       |

## 🚀 Kullanım Kılavuzu

Sistemi başlattıktan sonra aşağıdaki buton fonksiyonlarını kullanabilirsiniz:

### 1. Mors Girişi (BTN_SIGNAL - B0)

- Kısa ve uzun basışlar `Timer1` ile ayırt edilir.
- Kısa basışlar nokta (`.`), uzun basışlar çizgi (`-`) olarak algılanır.
- Yazılan kod ekranda `<` işaretinin yanında önizlenir.

### 2. Onay ve Kayıt (BTN_UPLOAD - B1)

- Tampon bellekteki Mors kodunu çözer ve harfi metne ekler.
- Eklenen harf otomatik olarak **EEPROM**'a kaydedilir.

### 3. Silme (BTN_DELETE - B2)

- Eğer henüz onaylanmamış bir Mors kodu (nokta/çizgi) varsa, son girilen sembolü siler.
- Eğer Mors kodu yoksa, yazılmış metindeki son harfi siler ve EEPROM'u günceller.

### 4. Sıfırlama (BTN_RESET - B3)

- Ekranı temizler, metin ve mors dizilerini sıfırlar.
- **Not:** Bu işlem EEPROM'daki kayıtlı veriyi silmez, sadece çalışma anındaki (RAM) veriyi temizler.

### 5. Format Atma / Hard Reset (BTN_HARD_RESET - B4)

- Bu tuşa basılı tutulduğunda "FORMAT ATILIYOR!" uyarısı verir.
- **EEPROM'daki tüm kayıtlı metni siler (0 yazar).**
- İşlem bittiğinde sesli uyarı verir ve sistemi sıfırlar.

## 💻 Yazılım ve Derleme

Bu proje **CCS C Compiler** kullanılarak yazılmıştır.

1. `src/main.c` dosyasını CCS C IDE ile açın.
2. `LCD.C` sürücüsünün (driver) derleyici kütüphanesinde veya proje klasöründe olduğundan emin olun.
3. Derleme (Compile) işlemini yapın.
4. Oluşan `main.hex` dosyasını PIC16F887'ye yükleyin.

**Önemli Not:** Proje **Internal Oscillator (Dahili Osilatör)** kullanacak şekilde ayarlanmıştır (`#fuses INTRC_IO`). Harici kristal bağlamanıza gerek yoktur.

---

_Geliştirici: Alihan_
_Düzenleme Tarihi: 2025_
