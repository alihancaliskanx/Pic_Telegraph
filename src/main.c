#include <16F887.h>
#fuses HS, NOWDT, NOPROTECT, NOLVP, NOBROWNOUT, NOPUT
#use delay(clock = 8000000)

#include <string.h>
#include <stdlib.h>

// --- LCD Ayarları ---
#define LCD_RS_PIN PIN_D1
#define LCD_RW_PIN PIN_D2
#define LCD_ENABLE_PIN PIN_D3
#define LCD_DATA4 PIN_D4
#define LCD_DATA5 PIN_D5
#define LCD_DATA6 PIN_D6
#define LCD_DATA7 PIN_D7
#include <LCD.C>

// --- Donanım Pinleri ---
#define SD_CS PIN_C2
#define LED_PIN PIN_A0
#define BUZZER_PIN PIN_A1

// --- Butonlar ---
#define BTN_SIGNAL PIN_B0
#define BTN_UPLOAD PIN_B1
#define BTN_DELETE PIN_B2
#define BTN_RESET PIN_B3
#define BTN_FULLRST PIN_B4

// --- SD Kart Komutları ---
#define CMD0 0
#define CMD8 8
#define CMD17 17
#define CMD24 24
#define CMD55 55
#define ACMD41 41

// --- Değişkenler ---
char morse_buffer[10];
char text_buffer[21];
int8 morse_index = 0;
int8 text_index = 0;
int32 sector_addr = 0;

// Timer ve Buton Durumları
volatile int16 press_counter = 0; // Basılma süresi sayacı
int1 btn_prev_state = 0;          // Butonun önceki durumu
int1 update_needed = 0;           // Ekran güncelleme bayrağı

// --- Mors Alfabesi ---
const char *morse_code[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
    "..-", "...-", ".--", "-..-", "-.--", "--..",
    "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."};

const char ascii_char[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

// ================= SPI / SD KART (Aynı) =================
char spi_transfer(char data_out) { return spi_read(data_out); }

char sd_send_command(char cmd, int32 arg)
{
    char response, retry = 0;
    output_low(SD_CS);
    spi_transfer(cmd | 0x40);
    spi_transfer(make8(arg, 3));
    spi_transfer(make8(arg, 2));
    spi_transfer(make8(arg, 1));
    spi_transfer(make8(arg, 0));
    if (cmd == CMD0) spi_transfer(0x95);
    else if (cmd == CMD8) spi_transfer(0x87);
    else spi_transfer(0xFF);
    while ((response = spi_transfer(0xFF)) == 0xFF) { if (retry++ > 200) break; }
    return response;
}

int8 sd_init(void)
{
    int8 i, response;
    output_high(SD_CS);
    for (i = 0; i < 10; i++) spi_transfer(0xFF);
    if (sd_send_command(CMD0, 0) != 1) { output_high(SD_CS); return 1; }
    output_high(SD_CS); sd_send_command(CMD8, 0x1AA); output_high(SD_CS);
    int16 timeout = 0;
    do {
        sd_send_command(CMD55, 0); output_high(SD_CS);
        response = sd_send_command(ACMD41, 0x40000000); output_high(SD_CS);
        if (timeout++ > 1000) return 2;
    } while (response != 0);
    return 0;
}

int8 sd_write_block(int32 address, char *data, int8 len)
{
    int16 i;
    if (sd_send_command(CMD24, address) == 0) {
        spi_transfer(0xFF); spi_transfer(0xFE);
        for (i = 0; i < 512; i++) {
            if (i < len) spi_transfer(data[i]); else spi_transfer(0x20);
        }
        spi_transfer(0xFF); spi_transfer(0xFF);
        if ((spi_transfer(0xFF) & 0x1F) != 0x05) { output_high(SD_CS); return 1; }
        while (spi_transfer(0xFF) == 0x00); output_high(SD_CS); return 0;
    }
    output_high(SD_CS); return 1;
}

// ================= YARDIMCI FONKSİYONLAR =================

char decode_morse(char *code)
{
    int8 i;
    // Eğer buffer boşsa boşluk dön
    if(strlen(code) == 0) return ' '; 

    for (i = 0; i < 36; i++)
    {
        if (strcmp(code, morse_code[i]) == 0)
            return ascii_char[i];
    }
    return '?'; // Bulunamazsa ? döner
}

// 4 Satırlı LCD için Özel İmleç Fonksiyonu
void lcd_locate(int8 x, int8 y)
{
    int8 address;
    switch (y) {
        case 1: address = 0x00; break;
        case 2: address = 0x40; break;
        case 3: address = 0x14; break;
        case 4: address = 0x54; break;
        default: address = 0x00; break;
    }
    address += x - 1;
    lcd_send_byte(0, 0x80 | address);
}

// EKRAN GÜNCELLEME (ÖNİZLEMELİ)
void update_lcd()
{
    int8 i, len;
    char preview_char;

    // --- 2. Satır: Kesinleşmiş Metin + [Önizleme] ---
    lcd_locate(1, 2);
    printf(lcd_putc, "%s", text_buffer); // Kaydedilmiş metni yaz
    
    // Eğer şu an mors giriliyorsa, yanına tahmini karakteri yaz
    if(morse_index > 0) {
        preview_char = decode_morse(morse_buffer);
        // Önizlemeyi belirtmek için yanıp sönen bir imleç veya farklı karakter kullanabilirsin
        // Burada karakteri direkt gösteriyoruz.
        lcd_putc(preview_char); 
        lcd_putc('<'); // İmleç efekti
    } else {
        lcd_putc(' ');
        lcd_putc(' ');
    }

    // Satırın geri kalanını temizle
    len = strlen(text_buffer) + (morse_index > 0 ? 2 : 0);
    for (i = len; i < 20; i++) lcd_putc(' ');

    // --- 3. Satır: Mors Kodu (Görsel Kontrol) ---
    lcd_locate(1, 3);
    printf(lcd_putc, "%s", morse_buffer);
    len = strlen(morse_buffer);
    for (i = len; i < 20; i++) lcd_putc(' ');
}

// ================= TIMER KESMESİ (10ms) =================
#INT_TIMER1
void timer1_isr()
{
    // Timer1 Reset (10ms için)
    // 8MHz Osc -> 2MHz Fosc/4. Prescaler 8 -> 250kHz (4us). 
    // 10ms = 10000us. 10000 / 4 = 2500 tick.
    // 65536 - 2500 = 63036
    set_timer1(63036);

    // Buton Durumu Oku
    int1 btn_current = input(BTN_SIGNAL);

    if (btn_current) // Butona Basılıysa
    {
        press_counter++;
        output_high(LED_PIN);
        output_high(BUZZER_PIN);
    }
    else // Buton Bırakıldıysa veya Basılmıyorsa
    {
        output_low(LED_PIN);
        output_low(BUZZER_PIN);

        // Düşen Kenar (Buton az önce bırakıldı)
        if (btn_prev_state == 1) 
        {
            if (morse_index < 6) 
            {
                // 25 birim = 250ms (Nokta/Çizgi Eşiği)
                if (press_counter < 25) 
                    morse_buffer[morse_index++] = '.';
                else 
                    morse_buffer[morse_index++] = '-';
                
                morse_buffer[morse_index] = '\0';
                
                // Ana döngüye ekranı güncelle emri ver
                update_needed = 1; 
            }
            press_counter = 0;
        }
    }
    btn_prev_state = btn_current;
}

// ================= ANA PROGRAM =================
void main()
{
    setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_CLK_DIV_64);
    port_b_pullups(FALSE);
    set_tris_b(0xFF);

    output_drive(LED_PIN);
    output_drive(BUZZER_PIN);
    output_low(LED_PIN);
    output_low(BUZZER_PIN);

    lcd_init();
    delay_ms(100);

    // Timer1 Kurulumu (10ms interrupt)
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    set_timer1(63036);
    enable_interrupts(INT_TIMER1);
    enable_interrupts(GLOBAL);

    lcd_locate(1, 1);
    printf(lcd_putc, "Canli Telgraf v2.0");

    lcd_locate(1, 2);
    if (sd_init() == 0) {
        printf(lcd_putc, "SD Kart: OK        ");
        setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_CLK_DIV_16);
    } else {
        printf(lcd_putc, "SD Kart: HATA!     ");
    }

    delay_ms(1000);
    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "Mod: CANLI CEVIRI   ");
    update_lcd();

    while (TRUE)
    {
        // --- EKRAN GÜNCELLEME (Sinyal gelince çalışır) ---
        if(update_needed)
        {
            update_lcd();
            update_needed = 0;
        }

        // --- YÜKLE TUŞU (Onaylama İşlemi) ---
        if (input(BTN_UPLOAD))
        {
            // Debounce
            while(input(BTN_UPLOAD)); 
            
            // Eğer bufferda bir mors kodu varsa onu onayla ve metne ekle
            if (morse_index > 0)
            {
                char final_char = decode_morse(morse_buffer);
                
                // Metin bufferına ekle
                if (text_index < 20 && final_char != '?')
                {
                    text_buffer[text_index++] = final_char;
                    text_buffer[text_index] = '\0';
                }
                
                // Mors bufferını temizle (Sıradaki harf için hazırla)
                morse_index = 0;
                morse_buffer[0] = '\0';
                
                // Onay Sesi
                output_high(BUZZER_PIN); delay_ms(50); output_low(BUZZER_PIN);
                update_lcd();
            }
            else // Eğer mors boşsa ve basıldıysa, SD karta kaydet (Opsiyonel)
            {
                // SD Karta kayıt kodu buraya eklenebilir veya 
                // metne boşluk eklemek için kullanılabilir.
                // Şimdilik boşluk ekleyelim:
                if (text_index < 20) {
                    text_buffer[text_index++] = ' ';
                    text_buffer[text_index] = '\0';
                    update_lcd();
                }
            }
        }

        // --- SİL TUŞU ---
        if (input(BTN_DELETE))
        {
            while(input(BTN_DELETE)); // Bırakılmasını bekle
            
            // Önce yazılmakta olan mors kodunu sil
            if (morse_index > 0) {
                morse_buffer[--morse_index] = '\0';
            }
            // Mors kodu yoksa, yazılmış metinden sil
            else if (text_index > 0) {
                text_buffer[--text_index] = '\0';
            }
            update_lcd();
            delay_ms(100);
        }

        // --- RESET ---
        if (input(BTN_RESET))
        {
            while(input(BTN_RESET));
            text_index = 0; text_buffer[0] = '\0';
            morse_index = 0; morse_buffer[0] = '\0';
            update_lcd();
        }
    }
}