***

# Canlı Telgraf v2.0 (PIC16F887)

Bu proje, PIC16F887 mikrodenetleyicisi kullanılarak geliştirilmiş bir **Mors Alfabesi Çözücü ve Kayıt Sistemi**dir. Kullanıcı tarafından girilen Mors kodlarını (nokta/çizgi) anlık olarak metne (ASCII) çevirir, LCD ekranda gösterir ve istenildiğinde bir SD karta kayıt yapar.

## 📋 Özellikler

* **Canlı Çeviri:** Girilen nokta (.) ve çizgileri (-) eş zamanlı olarak analiz eder ve karşılık gelen harfi/rakamı bulur.
* **SD Kart Desteği:** Oluşturulan metinleri SPI protokolü üzerinden SD karta (Sector 0) kaydeder.
* **LCD Arayüzü:** 4 satırlı LCD ekran desteği ile girilen Mors kodunu, çevrilen metni ve sistem durumunu gösterir.
* **Geri Bildirim:** Her tuş basımında LED ve Buzzer ile görsel/işitsel geri bildirim sağlar.
* **Akıllı Düzenleme:** Hatalı girilen Mors kodunu veya yanlış harfi silme özelliği.
* **Zamanlayıcı (Timer) Tabanlı Okuma:** Tuşa basış sürelerini Timer1 kesmesi ile hassas bir şekilde ölçer.

## 🛠 Donanım Gereksinimleri

* **Mikrodenetleyici:** PIC16F887
* **Ekran:** HD44780 uyumlu LCD Ekran (20x4 önerilir)
* **Depolama:** SD/MMC Kart ve Modülü
* **Giriş:** 5 Adet Push Buton
* **Çıkış:** 1x LED, 1x Buzzer (Aktif Buzzer)
* **Osilatör:** 8MHz Harici Kristal (HS Osilatör)

## 🔌 Pin Bağlantı Şeması

| Bileşen | Pin Adı | PIC Pini | Açıklama |
| :--- | :--- | :--- | :--- |
| **Girişler** | BTN_SIGNAL | PIN_B0 | Sinyal gönderme tuşu (Mors girişi) |
| | BTN_UPLOAD | PIN_B1 | Onaylama ve SD Karta Yükleme tuşu |
| | BTN_DELETE | PIN_B2 | Silme tuşu |
| | BTN_RESET | PIN_B3 | Satır/Ekran sıfırlama |
| | BTN_FULLRST | PIN_B4 | Fabrika ayarlarına dönüş (SD Kartı siler) |
| **Çıkışlar** | LED | PIN_A0 | Sinyal LED'i |
| | BUZZER | PIN_A1 | Sinyal Sesi |
| **SD Kart** | SD_CS | PIN_C2 | Chip Select |
| | SPI (SCK/SDO/SDI)| Donanımsal | SPI Pinleri (C3, C4, C5) |
| **LCD** | RS | PIN_D1 | Register Select |
| | RW | PIN_D2 | Read/Write |
| | EN | PIN_D3 | Enable |
| | DATA 4-7 | D4-D7 | LCD Data Yolları |

## 🚀 Kullanım Kılavuzu

Sistemi başlattıktan sonra aşağıdaki buton fonksiyonlarını kullanabilirsiniz:

### 1. Mors Girişi (BTN_SIGNAL - B0)
* **Kısa Basış (< 250ms):** Nokta (`.`) ekler.
* **Uzun Basış (> 250ms):** Çizgi (`-`) ekler.
* *Maksimum 6 karakterlik mors dizisi girilebilir.*

### 2. Onay ve Kayıt (BTN_UPLOAD - B1)
* **Kısa Basış:** Tampon bellekteki (buffer) Mors kodunu çözer ve harfi metne ekler. Eğer Mors kodu boşsa metne "Boşluk" ekler.
* **Uzun Basış (> 500ms):** Yazılan tüm metni SD karta kaydeder ve yeni bir sektöre geçer.

### 3. Silme (BTN_DELETE - B2)
* Eğer henüz onaylanmamış bir Mors kodu giriliyorsa, son girilen nokta veya çizgiyi siler.
* Eğer Mors kodu yoksa, metindeki son harfi siler.

### 4. Sıfırlama (BTN_RESET - B3)
* Ekranı temizler, metin ve mors tamponlarını sıfırlar. Yeni bir satıra başlar.

### 5. Tam Sıfırlama (BTN_FULLRST - B4)
* **Dikkat:** SD Kartın 0. sektörünü tamamen siler (formatlar) ve sistemi yeniden başlatır.

## 💻 Yazılım ve Derleme

Bu proje **CCS C Compiler** kullanılarak yazılmıştır.

1.  `src/main.c` dosyasını CCS C ile açın.
2.  `LCD.C` dosyasının proje dizininde veya kütüphane yolunda olduğundan emin olun.
3.  Derleme (Compile) işlemini yapın.
4.  Oluşan `main.hex` dosyasını PIC16F887'ye yükleyin.

**Proteus Simülasyonu:**
Proje klasöründe bulunan `Telgraf_1.0.pdsprj` dosyası ile devreyi Proteus üzerinde simüle edebilirsiniz. Simülasyon için `Disk/sdcard.mmc` dosyasının SD kart bileşenine yüklü olduğundan emin olun.

## ⚙️ Teknik Detaylar

* **SPI Ayarları:** SPI Master modu, `SPI_CLK_DIV_64` (Başlangıç) -> `SPI_CLK_DIV_16` (Başarılı init sonrası).
* **Timer1:** 10ms'lik kesmeler oluşturarak buton ark ve basılı tutma sürelerini (debounce/duration) yönetir.
* **Sigorta Ayarları (Fuses):** `HS, NOWDT, NOPROTECT, NOLVP, NOBROWNOUT, NOPUT`

---
*Geliştirici: Alihan*
*Versiyon: 2.0*