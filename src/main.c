#include <16F887.h>
#fuses INTRC_IO, NOWDT, NOPROTECT, NOLVP, NOBROWNOUT, NOPUT, NOMCLR
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

char morse_buffer[10];
char text_buffer[21];
int8 morse_index = 0;
int8 text_index = 0;

volatile int16 press_counter = 0;
int1 btn_prev_state = 0;
int1 update_needed = 0;

const char morse_tree[64] = {
    0, 0, 'E', 'T', 'I', 'A', 'N', 'M', 'S', 'U', 'R', 'W', 'D', 'K', 'G', 'O',
    'H', 'V', 'F', 0, 'L', 0, 'P', 'J', 'B', 'X', 'C', 'Y', 'Z', 'Q', 0, 0,
    '5', '4', 0, '3', 0, 0, 0, '2', 0, 0, 0, 0, 0, 0, 0, '1',
    '6', 0, 0, 0, 0, 0, 0, 0, '7', 0, 0, 0, '8', 0, '9', '0'};

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

#INT_TIMER1
void timer1_isr()
{
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
        if (btn_prev_state == 1)
        {
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
    set_tris_b(0xFF);
    port_b_pullups(TRUE);

    output_drive(LED_PIN);
    output_drive(BUZZER_PIN);

    lcd_init();
    delay_ms(100);

    // Timer1 Ayarı
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    set_timer1(63036);
    enable_interrupts(INT_TIMER1);
    enable_interrupts(GLOBAL);

    lcd_locate(1, 1);
    printf(lcd_putc, "Mors Telgraf");

    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "MOD: YAZIM");
    update_lcd();

    while (TRUE)
    {
        if (update_needed)
        {
            update_lcd();
            update_needed = 0;
        }
        if (!input(BTN_UPLOAD))
        {
            while (!input(BTN_UPLOAD))
                ;

            if (morse_index > 0)
            {
                char final_char = decode_morse(morse_buffer);
                if (text_index < 20 && final_char != '?')
                {
                    text_buffer[text_index++] = final_char;
                    text_buffer[text_index] = '\0';
                }
                morse_index = 0;
                morse_buffer[0] = '\0';
                update_lcd();
            }
            else
            {
            }
        }

        if (!input(BTN_DELETE))
        {
            while (!input(BTN_DELETE))
                ;

            if (morse_index > 0)
                morse_buffer[--morse_index] = '\0';
            else if (text_index > 0)
                text_buffer[--text_index] = '\0';

            update_lcd();
        }

        if (!input(BTN_RESET))
        {
            while (!input(BTN_RESET))
                ;
            text_index = 0;
            text_buffer[0] = '\0';
            morse_index = 0;
            morse_buffer[0] = '\0';
            update_lcd();
        }
    }
}