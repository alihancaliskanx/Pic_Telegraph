#include <16F887.h>
// WDT Eklendi
#fuses INTRC_IO, WDT, NOPROTECT, NOLVP, NOBROWNOUT, NOPUT, NOMCLR
#use delay(clock = 8000000)

#include <string.h>
#include <stdlib.h>

#define LCD_RS_PIN PIN_D1
#define LCD_RW_PIN PIN_D2
#define LCD_ENABLE_PIN PIN_D3
#define LCD_DATA4 PIN_D4
#define LCD_DATA5 PIN_D5
#define LCD_DATA6 PIN_D6
#define LCD_DATA7 PIN_D7
#include <LCD.C>

#define LED_PIN PIN_A0
#define BUZZER_PIN PIN_A1

#define BTN_SIGNAL PIN_B0
#define BTN_UPLOAD PIN_B1
#define BTN_DELETE PIN_B2
#define BTN_RESET PIN_B3
#define BTN_HARD_RESET PIN_B4 // YENİ: Hard Reset Pini

char morse_buffer[10];
char text_buffer[21];
int8 morse_index = 0;
int8 text_index = 0;

volatile int16 press_counter = 0;
volatile int16 idle_counter = 0;
#define SLEEP_TIMEOUT 3000

int1 btn_prev_state = 0;
int1 update_needed = 0;

const char morse_tree[64] = {
    0, 0, 'E', 'T', 'I', 'A', 'N', 'M', 'S', 'U', 'R', 'W', 'D', 'K', 'G', 'O',
    'H', 'V', 'F', 0, 'L', 0, 'P', 'J', 'B', 'X', 'C', 'Y', 'Z', 'Q', 0, 0,
    '5', '4', 0, '3', 0, 0, 0, '2', 0, 0, 0, 0, 0, 0, 0, '1',
    '6', 0, 0, 0, 0, 0, 0, 0, '7', 0, 0, 0, '8', 0, '9', '0'};

void save_text_to_eeprom()
{
    int8 i;
    write_eeprom(0, text_index);
    for (i = 0; i < text_index; i++)
    {
        write_eeprom(i + 1, text_buffer[i]);
    }
}

void load_text_from_eeprom()
{
    int8 i;
    text_index = read_eeprom(0);
    if (text_index > 20)
        text_index = 0;
    for (i = 0; i < text_index; i++)
    {
        text_buffer[i] = read_eeprom(i + 1);
    }
    text_buffer[text_index] = '\0';
}

char decode_morse(char *code)
{
    int8 i, len;
    int8 index = 1;
    len = strlen(code);
    if (len == 0)
        return ' ';

    for (i = 0; i < len; i++)
    {
        index = index << 1;
        if (code[i] == '-')
            index |= 1;
        if (index >= 64)
            return '?';
    }
    if (morse_tree[index] == 0)
        return '?';
    return morse_tree[index];
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

    lcd_locate(1, 2);
    printf(lcd_putc, "%s", text_buffer);

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

    len = strlen(text_buffer) + (morse_index > 0 ? 2 : 0);
    for (i = len; i < 20; i++)
        lcd_putc(' ');

    lcd_locate(1, 3);
    printf(lcd_putc, "%s", morse_buffer);
    len = strlen(morse_buffer);
    for (i = len; i < 20; i++)
        lcd_putc(' ');
}

void enter_sleep_mode()
{
    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "UYKU MODU...");
    delay_ms(500);
    lcd_send_byte(0, 0x08);

    output_low(LED_PIN);
    output_low(BUZZER_PIN);

    while (TRUE)
    {
        setup_wdt(WDT_2304MS);
        sleep();
        // B4 de uyandırma tuşu olarak eklendi
        if (!input(BTN_SIGNAL) || !input(BTN_UPLOAD) || !input(BTN_DELETE) || !input(BTN_RESET) || !input(BTN_HARD_RESET))
        {
            break;
        }
    }

    setup_wdt(WDT_18MS);
    lcd_init();
    lcd_locate(1, 1);
    printf(lcd_putc, "MOD: YAZIM");
    update_lcd();
    idle_counter = 0;
}

#INT_TIMER1
void timer1_isr()
{
    set_timer1(63036);

    if (idle_counter < 32000)
    {
        idle_counter++;
    }

    int1 btn_current = !input(BTN_SIGNAL);

    if (btn_current)
    {
        idle_counter = 0;
        press_counter++;
        output_high(LED_PIN);
        output_high(BUZZER_PIN);
    }
    else
    {
        output_low(LED_PIN);
        output_low(BUZZER_PIN);
        if (btn_prev_state == 1)
        {
            idle_counter = 0;
            if (press_counter > 2)
            {
                if (press_counter < 30)
                    morse_buffer[morse_index++] = '.';
                else
                    morse_buffer[morse_index++] = '-';

                morse_buffer[morse_index] = '\0';
                update_needed = 1;
            }
            press_counter = 0;
        }
    }
    btn_prev_state = btn_current;
}

void main()
{
    setup_wdt(WDT_2304MS);

    set_tris_b(0xFF);
    port_b_pullups(TRUE);
    output_drive(LED_PIN);
    output_drive(BUZZER_PIN);

    restart_wdt();
    lcd_init();
    restart_wdt();
    delay_ms(100);

    load_text_from_eeprom();

    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    set_timer1(63036);
    enable_interrupts(INT_TIMER1);
    enable_interrupts(GLOBAL);

    lcd_locate(1, 1);
    printf(lcd_putc, "Mors Telgraf");
    restart_wdt();
    delay_ms(1000);

    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "MOD: YAZIM");
    update_lcd();

    while (TRUE)
    {
        restart_wdt();

        if (idle_counter > SLEEP_TIMEOUT)
        {
            enter_sleep_mode();
        }

        if (update_needed)
        {
            update_lcd();
            update_needed = 0;
        }

        if (!input(BTN_UPLOAD))
        {
            idle_counter = 0;
            while (!input(BTN_UPLOAD))
            {
                restart_wdt();
            }

            if (morse_index > 0)
            {
                char final_char = decode_morse(morse_buffer);
                if (text_index < 20 && final_char != '?')
                {
                    text_buffer[text_index++] = final_char;
                    text_buffer[text_index] = '\0';
                    save_text_to_eeprom();
                }
                morse_index = 0;
                morse_buffer[0] = '\0';
                update_lcd();
            }
        }

        if (!input(BTN_DELETE))
        {
            idle_counter = 0;
            while (!input(BTN_DELETE))
            {
                restart_wdt();
            }

            if (morse_index > 0)
                morse_buffer[--morse_index] = '\0';
            else if (text_index > 0)
            {
                text_buffer[--text_index] = '\0';
                save_text_to_eeprom();
            }
            update_lcd();
        }

        // --- NORMAL RESET (Sadece ekranı temizler) ---
        if (!input(BTN_RESET))
        {
            idle_counter = 0;
            while (!input(BTN_RESET))
            {
                restart_wdt();
            }

            text_index = 0;
            text_buffer[0] = '\0';
            morse_index = 0;
            morse_buffer[0] = '\0';
            // Not: Normal reset sadece RAM'i siler, EEPROM'u ellemiyor (eski kodun gibi)
            // Eğer normal resette de eeprom silinsin istiyorsan buraya save_text_to_eeprom(); ekle
            update_lcd();
        }

        // --- HARD RESET (B4 - EEPROM TEMİZLEME) ---
        if (!input(BTN_HARD_RESET))
        {
            idle_counter = 0;

            // Kullanıcıya bilgi ver
            lcd_putc('\f');
            lcd_locate(1, 1);
            printf(lcd_putc, "FORMAT ATILIYOR!");
            output_high(BUZZER_PIN); // Sesli uyarı
            delay_ms(200);
            output_low(BUZZER_PIN);

            // Tuş bırakılana kadar bekle
            while (!input(BTN_HARD_RESET))
            {
                restart_wdt();
            }

            // EEPROM TEMİZLİĞİ
            int8 i;
            // İlk 30 adresi 0 yapıyoruz (text uzunluğu ve karakterler)
            for (i = 0; i < 30; i++)
            {
                write_eeprom(i, 0);
                restart_wdt(); // Yazma işlemi sırasında WDT resetle
            }

            // Değişkenleri sıfırla
            text_index = 0;
            text_buffer[0] = '\0';
            morse_index = 0;
            morse_buffer[0] = '\0';

            // Ekranı güncelle
            lcd_putc('\f');
            lcd_locate(1, 1);
            printf(lcd_putc, "MOD: YAZIM");
            update_lcd();

            // İşlem bitti sesi
            output_high(BUZZER_PIN);
            delay_ms(50);
            output_low(BUZZER_PIN);
            delay_ms(50);
            output_high(BUZZER_PIN);
            delay_ms(50);
            output_low(BUZZER_PIN);
        }
    }
}