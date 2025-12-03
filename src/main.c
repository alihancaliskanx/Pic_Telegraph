#include <16F887.h>
// Sigorta ayarlarını güncelledim: BROWNOUT kapalı (Voltaj düşerse reset atmasın)
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
#include "LCD.C"

// --- Donanım Pinleri ---
#define SD_CS PIN_C2
#define LED_PIN PIN_A0
#define BUZZER_PIN PIN_A1

// --- Butonlar (Active HIGH) ---
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

// --- Mors Alfabesi (Harfler ve Rakamlar) ---
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

// ================= SPI / SD KART =================
char spi_transfer(char data_out)
{
    return spi_read(data_out);
}

char sd_send_command(char cmd, int32 arg)
{
    char response, retry = 0;
    output_low(SD_CS);
    spi_transfer(cmd | 0x40);
    spi_transfer(make8(arg, 3));
    spi_transfer(make8(arg, 2));
    spi_transfer(make8(arg, 1));
    spi_transfer(make8(arg, 0));
    if (cmd == CMD0)
        spi_transfer(0x95);
    else if (cmd == CMD8)
        spi_transfer(0x87);
    else
        spi_transfer(0xFF);
    while ((response = spi_transfer(0xFF)) == 0xFF)
    {
        if (retry++ > 200)
            break;
    }
    return response;
}

int8 sd_init(void)
{
    int8 i, response;
    output_high(SD_CS);
    for (i = 0; i < 10; i++)
        spi_transfer(0xFF);
    if (sd_send_command(CMD0, 0) != 1)
    {
        output_high(SD_CS);
        return 1;
    }
    output_high(SD_CS);
    sd_send_command(CMD8, 0x1AA);
    output_high(SD_CS);
    int16 timeout = 0;
    do
    {
        sd_send_command(CMD55, 0);
        output_high(SD_CS);
        response = sd_send_command(ACMD41, 0x40000000);
        output_high(SD_CS);
        if (timeout++ > 1000)
            return 2;
    } while (response != 0);
    return 0;
}

int8 sd_write_block(int32 address, char *data, int8 len)
{
    int16 i;
    if (sd_send_command(CMD24, address) == 0)
    {
        spi_transfer(0xFF);
        spi_transfer(0xFE);
        for (i = 0; i < 512; i++)
        {
            if (i < len)
                spi_transfer(data[i]);
            else
                spi_transfer(0x20); // Boşluk
        }
        spi_transfer(0xFF);
        spi_transfer(0xFF);
        if ((spi_transfer(0xFF) & 0x1F) != 0x05)
        {
            output_high(SD_CS);
            return 1;
        }
        while (spi_transfer(0xFF) == 0x00)
            ;
        output_high(SD_CS);
        return 0;
    }
    output_high(SD_CS);
    return 1;
}

// ================= EKRAN AYARLARI =================

// 4 Satırlı LCD için Özel İmleç Fonksiyonu
void lcd_locate(int8 x, int8 y)
{
    int8 address;
    switch (y)
    {
    case 1:
        address = 0x00;
        break;
    case 2:
        address = 0x40;
        break;
    case 3:
        address = 0x14;
        break; // 3. Satır Adresi
    case 4:
        address = 0x54;
        break;
    default:
        address = 0x00;
        break;
    }
    address += x - 1;
    lcd_send_byte(0, 0x80 | address);
}

void update_lcd()
{
    int8 i, len;

    // 2. Satır: Mesaj (ASCII)
    lcd_locate(1, 2);
    printf(lcd_putc, "%s", text_buffer);
    len = strlen(text_buffer);
    for (i = len; i < 20; i++)
        lcd_putc(' ');

    // 3. Satır: Mors Kodu
    lcd_locate(1, 3);
    printf(lcd_putc, "%s", morse_buffer);
    len = strlen(morse_buffer);
    for (i = len; i < 20; i++)
        lcd_putc(' ');
}

char decode_morse(char *code)
{
    int8 i;
    for (i = 0; i < 36; i++)
    { // 36 Karakter (Harf+Rakam)
        if (strcmp(code, morse_code[i]) == 0)
            return ascii_char[i];
    }
    return '?';
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

    lcd_locate(1, 1);
    printf(lcd_putc, "Telgraf Sistemi v1.5");

    lcd_locate(1, 2);
    printf(lcd_putc, "SD Kart: Bekleniyor");

    if (sd_init() == 0)
    {
        lcd_locate(1, 2);
        printf(lcd_putc, "SD Kart: OK        ");
        setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_CLK_DIV_16);
    }
    else
    {
        lcd_locate(1, 2);
        printf(lcd_putc, "SD Kart: HATA!     ");
    }

    morse_buffer[0] = '\0';
    text_buffer[0] = '\0';
    delay_ms(1000);

    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "Hazir: Sinyal Bekleniyor");
    update_lcd();

    while (TRUE)
    {

        // --- 1. SİNYAL TUŞU ---
        if (input(BTN_SIGNAL))
        {
            unsigned int16 press_time = 0;
            output_high(LED_PIN);
            output_high(BUZZER_PIN);

            while (input(BTN_SIGNAL))
            {
                delay_ms(10);
                press_time += 10;
                if (press_time > 3000)
                    break;
            }
            output_low(LED_PIN);
            output_low(BUZZER_PIN);

            if (press_time < 400)
            { // Nokta
                if (morse_index < 5)
                { // Max 5 karakter (Rakamlar için)
                    morse_buffer[morse_index++] = '.';
                    morse_buffer[morse_index] = '\0';
                }
            }
            else
            { // Çizgi
                if (morse_index < 5)
                {
                    morse_buffer[morse_index++] = '-';
                    morse_buffer[morse_index] = '\0';
                }
            }
            update_lcd();
        }

        // --- 2. YÜKLE TUŞU ---
        if (input(BTN_UPLOAD))
        {
            // Butona basıldığını hissetmek için kısa bip
            output_high(BUZZER_PIN);
            delay_ms(30);
            output_low(BUZZER_PIN);

            unsigned int16 hold_time = 0;
            while (input(BTN_UPLOAD))
            {
                delay_ms(10);
                hold_time += 10;
            }

            if (hold_time < 500)
            {
                // KISA BASIM: Mors Çevir (2. Satıra At)
                if (morse_index > 0)
                {
                    char decoded = decode_morse(morse_buffer);
                    if (text_index < 20)
                    {
                        text_buffer[text_index++] = decoded;
                        text_buffer[text_index] = '\0';
                    }
                    morse_index = 0;
                    morse_buffer[0] = '\0';
                    update_lcd();
                }
            }
            else
            {
                // UZUN BASIM: SD Karta Kaydet
                lcd_locate(1, 1);
                printf(lcd_putc, "Durum: Kaydediliyor ");

                if (sd_write_block(sector_addr * 512, text_buffer, text_index) == 0)
                {
                    lcd_locate(1, 1);
                    printf(lcd_putc, "Durum: Kayit OK     ");
                    sector_addr++;
                }
                else
                {
                    lcd_locate(1, 1);
                    printf(lcd_putc, "Durum: Kayit HATA   ");
                }
                delay_ms(1000);
                lcd_locate(1, 1);
                printf(lcd_putc, "Hazir: Sinyal Bekleniyor");
            }
        }

        // --- 3. SİL TUŞU ---
        if (input(BTN_DELETE))
        {
            output_high(BUZZER_PIN);
            delay_ms(30);
            output_low(BUZZER_PIN);
            unsigned int16 hold_time = 0;
            while (input(BTN_DELETE))
            {
                delay_ms(10);
                hold_time += 10;
            }

            if (hold_time < 500)
            {
                if (morse_index > 0)
                    morse_buffer[--morse_index] = '\0';
            }
            else
            {
                if (morse_index > 0)
                {
                    morse_index = 0;
                    morse_buffer[0] = '\0';
                }
                else
                {
                    if (text_index > 0)
                        text_buffer[--text_index] = '\0';
                }
            }
            update_lcd();
        }

        // --- 4. RESET TUŞU ---
        if (input(BTN_RESET))
        {
            while (input(BTN_RESET))
                ;
            morse_index = 0;
            morse_buffer[0] = '\0';
            text_index = 0;
            text_buffer[0] = '\0';
            sector_addr++;
            lcd_locate(1, 1);
            printf(lcd_putc, "Durum: Yeni Satir   ");
            delay_ms(500);
            lcd_locate(1, 1);
            printf(lcd_putc, "Hazir: Sinyal Bekleniyor");
            update_lcd();
        }

        // --- 5. FULL RESET TUŞU ---
        if (input(BTN_FULLRST))
        {
            while (input(BTN_FULLRST))
                ;
            lcd_putc('\f');
            lcd_locate(1, 1);
            printf(lcd_putc, "Sistem Sifirlaniyor");
            morse_index = 0;
            morse_buffer[0] = '\0';
            text_index = 0;
            text_buffer[0] = '\0';
            sector_addr = 0;
            char empty[1] = {0};
            sd_write_block(0, empty, 0);
            delay_ms(1000);
            lcd_putc('\f');
            lcd_locate(1, 1);
            printf(lcd_putc, "Hazir: Sinyal Bekleniyor");
            update_lcd();
        }
    }
}