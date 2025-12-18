#include <16F887.h>
// --- DONANIM AYARLARI (Çalışan Ayarlar) ---
#fuses INTRC_IO, NOWDT, NOPROTECT, NOLVP, NOBROWNOUT, NOPUT, NOMCLR
#use delay(clock = 8000000)

#include <string.h>

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
#define LED_PIN PIN_A0
#define BUZZER_PIN PIN_A1

// --- Butonlar (Active Low) ---
#define BTN_SIGNAL PIN_B0
#define BTN_UPLOAD PIN_B1
#define BTN_DELETE PIN_B2
#define BTN_RESET PIN_B3

// --- Değişkenler ---
char morse_buffer[10];
char text_buffer[21];
int8 morse_index = 0;
int8 text_index = 0;

// Timer ve Buton Durumları
volatile int16 press_counter = 0;
int1 btn_prev_state = 0;
int1 update_needed = 0;

// --- YENİ MORS SİSTEMİ (Senin İstediğin Array Yapısı) ---
// Toplam 36 Karakter (26 Harf + 10 Rakam)
const char *morse_code[36] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",                   // A-J
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",                     // K-T
    "..-", "...-", ".--", "-..-", "-.--", "--..",                                            // U-Z
    "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----." // 0-9
};

const char ascii_char[36] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

// ================= YARDIMCI FONKSİYONLAR =================

// Yeni Çözümleme Fonksiyonu: Listeyi tarayıp eşleşeni bulur
char decode_morse(char *code)
{
    int8 i;
    // Buffer boşsa boşluk dön
    if (strlen(code) == 0)
        return ' ';

    // 36 elemanlı diziyi tara
    for (i = 0; i < 36; i++)
    {
        // strcmp: İki string eşitse 0 döner
        if (strcmp(code, morse_code[i]) == 0)
        {
            return ascii_char[i]; // Eşleşen harfi ver
        }
    }
    return '?'; // Listede yoksa
}

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
        break;
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
    char preview_char;

    // Satır 2: Metin
    lcd_locate(1, 2);
    printf(lcd_putc, "%s", text_buffer);

    // Canlı Önizleme
    if (morse_index > 0)
    {
        preview_char = decode_morse(morse_buffer);
        lcd_putc(preview_char);
        lcd_putc('<');
    }
    else
    {
        lcd_putc("  ");
    }

    // Satır Temizliği
    len = strlen(text_buffer) + (morse_index > 0 ? 2 : 0);
    for (i = len; i < 20; i++)
        lcd_putc(' ');

    // Satır 3: Mors Tamponu
    lcd_locate(1, 3);
    printf(lcd_putc, "%s", morse_buffer);
    len = strlen(morse_buffer);
    for (i = len; i < 20; i++)
        lcd_putc(' ');
}

// ================= TIMER KESMESİ (Tuş Yakalama) =================
#INT_TIMER1
void timer1_isr()
{
    // Timer Reset (10ms)
    set_timer1(63036);

    int1 btn_current = !input(BTN_SIGNAL);

    if (btn_current)
    {
        press_counter++;
        output_high(LED_PIN);
        output_high(BUZZER_PIN);
    }
    else
    {
        output_low(LED_PIN);
        output_low(BUZZER_PIN);

        if (btn_prev_state == 1) // Buton Bırakıldı
        {
            // Debounce (Gürültü önleme)
            if (press_counter > 2)
            {
                // Kısa basış < 200ms -> Nokta
                if (press_counter < 20)
                    morse_buffer[morse_index++] = '.';
                else
                    morse_buffer[morse_index++] = '-';

                morse_buffer[morse_index] = '\0';

                // Taşma Koruması
                if (morse_index >= 9)
                    morse_index = 0;

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
    // Giriş/Çıkış Ayarları
    set_tris_b(0xFF);     // Port B Giriş (Butonlar)
    port_b_pullups(TRUE); // Dahili Pull-up Aktif

    output_drive(LED_PIN);
    output_drive(BUZZER_PIN);
    output_low(LED_PIN);

    lcd_init();
    delay_ms(100);

    // Timer1 Kurulumu
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    set_timer1(63036);
    enable_interrupts(INT_TIMER1);
    enable_interrupts(GLOBAL);

    // Açılış Mesajı
    lcd_locate(1, 1);
    printf(lcd_putc, "Mors Telgraf v4");
    lcd_locate(1, 2);
    printf(lcd_putc, "SD KART: YOK     ");
    delay_ms(1000);

    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "MOD: MANUEL GIRIS");
    update_lcd();

    while (TRUE)
    {
        if (update_needed)
        {
            update_lcd();
            update_needed = 0;
        }

        // --- YÜKLE TUŞU (Onay / Boşluk) ---
        if (!input(BTN_UPLOAD))
        {
            while (!input(BTN_UPLOAD))
                ; // Bırakılmasını bekle

            if (morse_index > 0)
            {
                // Bufferda harf varsa, çöz ve ekle
                char final_char = decode_morse(morse_buffer);
                if (text_index < 20 && final_char != '?')
                {
                    text_buffer[text_index++] = final_char;
                    text_buffer[text_index] = '\0';
                }
                morse_index = 0;
                morse_buffer[0] = '\0';
                // Harf onay sesi
                output_high(BUZZER_PIN);
                delay_ms(50);
                output_low(BUZZER_PIN);
            }
            else
            {
                // Buffer boşsa -> BOŞLUK Karakteri Ekle
                if (text_index < 20)
                {
                    text_buffer[text_index++] = ' ';
                    text_buffer[text_index] = '\0';
                }
            }
            update_lcd();
        }

        // --- SİL TUŞU ---
        if (!input(BTN_DELETE))
        {
            while (!input(BTN_DELETE))
                ;

            if (morse_index > 0)
                morse_buffer[--morse_index] = '\0'; // Harfi sil
            else if (text_index > 0)
                text_buffer[--text_index] = '\0'; // Metni sil

            update_lcd();
        }

        // --- RESET TUŞU ---
        if (!input(BTN_RESET))
        {
            while (!input(BTN_RESET))
                ;
            text_index = 0;
            text_buffer[0] = '\0';
            morse_index = 0;
            morse_buffer[0] = '\0';
            output_high(BUZZER_PIN);
            delay_ms(200);
            output_low(BUZZER_PIN);
            update_lcd();
        }
    }
}