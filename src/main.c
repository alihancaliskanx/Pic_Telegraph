#include <16F887.h>
// Configuration bits: Internal oscillator, No Watchdog (managed manually), No protection
#fuses INTRC_IO, NOWDT, NOPROTECT, NOLVP, NOBROWNOUT, PUT, NOMCLR
#use delay(clock = 8000000) // System clock: 8MHz

// Bluetooth UART configuration
// Changed stream name from BT_MODUL to BT_MODULE
#use rs232(baud = 9600, parity = N, xmit = PIN_C6, rcv = PIN_C7, bits = 8, stream = BT_MODULE)

#include <string.h>
#include <stdlib.h>

// --- LCD Pin Definitions ---
#define LCD_RS_PIN PIN_D1
#define LCD_RW_PIN PIN_D2
#define LCD_ENABLE_PIN PIN_D3
#define LCD_DATA4 PIN_D4
#define LCD_DATA5 PIN_D5
#define LCD_DATA6 PIN_D6
#define LCD_DATA7 PIN_D7
#include <LCD.C>

// --- Output Pins ---
#define LED_PIN PIN_A0      // Status LED
#define BUZZER_PIN PIN_A1   // Feedback Buzzer

// --- Input Button Definitions ---
#define BTN_SIGNAL PIN_B0   // Morse input button
#define BTN_UPLOAD PIN_B1   // Send/Confirm button
#define BTN_DELETE PIN_B2   // Backspace/Delete button
#define BTN_RESET PIN_B3    // Reset/Clear button
#define BTN_MODE PIN_B4     // Mode switch button

// --- Global Variables ---
char morse_buffer[10];      // Buffer to store current dots/dashes
char text_buffer[21];       // Buffer to store decoded text message
int8 morse_index = 0;
int8 text_index = 0;

char rx_temp_buffer[40];    // Temporary buffer for incoming Bluetooth data
char rx_display_buffer[25]; // Buffer for text to be displayed on LCD line 4
int8 rx_temp_index = 0;
int1 rx_data_ready = 0;     // Flag indicating new data arrived

volatile int16 press_counter = 0; // Timer to measure how long a button is pressed
volatile int16 idle_counter = 0;  // Timer to measure inactivity
#define SLEEP_TIMEOUT 3000        // Inactivity limit before sleep

int1 btn_prev_state = 0;    // Previous state of the signal button
int1 update_needed = 0;     // Flag to trigger screen refresh

volatile int8 scroll_tick = 0; // Timer for scrolling text
int1 scroll_now = 0;           // Flag to trigger scroll step
int8 scroll_pos = 0;           // Current scroll position

int1 app_mode = 0; // 0 = Message Mode, 1 = Command Mode

// Morse Code Lookup Tree (Binary Heap Structure)
// Left child = Dot, Right child = Dash
const char morse_tree[64] = {
    0, 0, 'E', 'T', 'I', 'A', 'N', 'M', 'S', 'U', 'R', 'W', 'D', 'K', 'G', 'O',
    'H', 'V', 'F', 0, 'L', 0, 'P', 'J', 'B', 'X', 'C', 'Y', 'Z', 'Q', 0, 0,
    '5', '4', 0, '3', 0, 0, 0, '2', 0, 0, 0, 0, 0, 0, 0, '1',
    '6', 0, 0, 0, 0, 0, 0, 0, '7', 0, 0, 0, '8', 0, '9', '0'};

// Delay function that keeps resetting the Watchdog Timer
void wdt_delay_ms(int16 time)
{
    int16 i;
    for (i = 0; i < time; i++)
    {
        delay_ms(1);
        restart_wdt();
    }
}

// Save the current text message to EEPROM (Permanent memory)
void save_text_to_eeprom()
{
    int8 i;
    write_eeprom(0, text_index);
    for (i = 0; i < text_index; i++)
    {
        write_eeprom(i + 1, text_buffer[i]);
        restart_wdt();
    }
}

// Load the saved text message from EEPROM
void load_text_from_eeprom()
{
    int8 i;
    text_index = read_eeprom(0);
    if (text_index > 20)
        text_index = 0;
    for (i = 0; i < text_index; i++)
    {
        text_buffer[i] = read_eeprom(i + 1);
        restart_wdt();
    }
    text_buffer[text_index] = '\0';
}

// Save the received Bluetooth message to EEPROM
void save_bt_to_eeprom()
{
    int8 i, len;
    len = strlen(rx_display_buffer);
    write_eeprom(50, len);
    for (i = 0; i < len; i++)
    {
        write_eeprom(51 + i, rx_display_buffer[i]);
        restart_wdt();
    }
}

// Load the last Bluetooth message from EEPROM
void load_bt_from_eeprom()
{
    int8 i, len;
    len = read_eeprom(50);
    if (len > 20)
        len = 0;

    for (i = 0; i < len; i++)
    {
        rx_display_buffer[i] = read_eeprom(51 + i);
        restart_wdt();
    }
    rx_display_buffer[len] = '\0';
}

// Algorithm to convert dots/dashes string into a character
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
            return '?'; // Error/Unknown
    }
    if (morse_tree[index] == 0)
        return '?';
    return "morse_tree"[index];
}

// Function to position the cursor on the LCD (Handles memory addresses)
void lcd_locate(int8 x, int8 y)
{
    int8 address;
    switch (y)
    {
    case 1: address = 0x00; break;
    case 2: address = 0x40; break;
    case 3: address = 0x14; break;
    case 4: address = 0x54; break;
    default: address = 0x00; break;
    }
    address += x - 1;
    lcd_send_byte(0, 0x80 | address);
}

// Scrolls the text received from Bluetooth on the 4th line
void update_scroll_line()
{
    int8 len, i, current_char_idx;
    int8 gap_size = 4;

    if (rx_display_buffer[0] == '\0')
    {
        lcd_locate(1, 4);
        printf(lcd_putc, "                    ");
        return;
    }

    len = strlen(rx_display_buffer);
    lcd_locate(1, 4);

    for (i = 0; i < 20; i++)
    {
        current_char_idx = (scroll_pos + i) % (len + gap_size);
        if (current_char_idx < len)
            lcd_putc(rx_display_buffer[current_char_idx]);
        else
            lcd_putc(' ');
    }
}

// Refresh the full LCD screen content
void update_lcd()
{
    int8 i, len;
    char preview_char;

    lcd_locate(1, 1);
    // Display current mode (Message vs Command)
    if (app_mode == 0)
        printf(lcd_putc, "MODE: MESSAGE       ");
    else
        printf(lcd_putc, "MODE: COMMAND       ");

    lcd_locate(1, 2);
    // Display current text message
    printf(lcd_putc, "%s", text_buffer);

    // Show preview of the character currently being typed
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

    // Clear the rest of the line
    len = strlen(text_buffer) + (morse_index > 0 ? 2 : 0);
    for (i = len; i < 20; i++)
        lcd_putc(' ');

    lcd_locate(1, 3);
    // Display current dots and dashes
    printf(lcd_putc, "%s", morse_buffer);
    len = strlen(morse_buffer);
    for (i = len; i < 20; i++)
        lcd_putc(' ');

    // Clear line 4 if empty (scroll function handles it otherwise)
    if (rx_display_buffer[0] == '\0')
    {
        lcd_locate(1, 4);
        printf(lcd_putc, "                    ");
    }
}

// Send data via Bluetooth (NMEA 0183 style format)
void send_nmea_packet()
{
    char type_char;
    int8 checksum = 0;
    int8 i, len;
    char char_to_send;

    if (app_mode == 0)
        type_char = 'M'; // Message packet
    else
        type_char = 'K'; // Command packet

    fprintf(BT_MODULE, "$%c,", type_char);
    checksum = type_char ^ ',';

    len = strlen(text_buffer);
    for (i = 0; i < len; i++)
    {
        char_to_send = text_buffer[i];
        // Replace spaces with commas in command mode
        if (app_mode == 1 && char_to_send == ' ')
            char_to_send = ',';
        fputc(char_to_send, BT_MODULE);
        checksum ^= char_to_send;
    }
    fprintf(BT_MODULE, "*%02X\r\n", checksum);
}

// Factory Reset: Wipes all data
void full_wipe_reset()
{
    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "DELETING ALL   "); // Feedback to user
    output_high(BUZZER_PIN);
    wdt_delay_ms(500);
    output_low(BUZZER_PIN);

    write_eeprom(0, 0);  // Clear text length
    write_eeprom(50, 0); // Clear BT msg length

    text_index = 0;
    text_buffer[0] = '\0';

    morse_index = 0;
    morse_buffer[0] = '\0';

    rx_display_buffer[0] = '\0';
    scroll_pos = 0;

    lcd_putc('\f');
    update_lcd();
}

// Process incoming Bluetooth NMEA packets
void process_incoming_nmea()
{
    char *ptr_start;
    char *ptr_end;
    int8 len;
    char packet_type;
    char payload[25];
    int1 param_val = 0;
    int8 i;
    int8 comma_index = 255;

    // Supported remote commands
    char cmd_rst[] = "rst";
    char cmd_led[] = "led_set";
    char cmd_buz[] = "buzzer_set";
    char cmd_hrst[] = "hard_reset";

    if (rx_temp_buffer[0] == '$')
    {
        packet_type = rx_temp_buffer[1];
        ptr_start = strchr(rx_temp_buffer, ',');
        ptr_end = strchr(rx_temp_buffer, '*');

        // Check if packet format is valid ($...*)
        if (ptr_start != 0 && ptr_end != 0 && ptr_end > ptr_start)
        {
            len = (int8)(ptr_end - ptr_start) - 1;
            if (len > 24)
                len = 24;

            strncpy(payload, ptr_start + 1, len);
            payload[len] = '\0';

            if (packet_type == 'M') // Text Message received
            {
                strcpy(rx_display_buffer, payload);
            }
            else if (packet_type == 'K') // Command received
            {
                // Parse parameters (e.g., cmd,1)
                for (i = 0; i < len; i++)
                {
                    if (payload[i] == ',')
                    {
                        comma_index = i;
                        break;
                    }
                }

                if (comma_index != 255)
                {
                    payload[comma_index] = '\0';
                    if (payload[comma_index + 1] == '1')
                        param_val = 1;
                }

                // Execute remote commands
                if (strcmp(payload, cmd_rst) == 0)
                {
                    reset_cpu();
                }
                else if (strcmp(payload, cmd_led) == 0)
                {
                    if (param_val) output_high(LED_PIN);
                    else output_low(LED_PIN);
                    rx_display_buffer[0] = '\0';
                }
                else if (strcmp(payload, cmd_buz) == 0)
                {
                    if (param_val) output_high(BUZZER_PIN);
                    else output_low(BUZZER_PIN);
                    rx_display_buffer[0] = '\0';
                }
                else if (strcmp(payload, cmd_hrst) == 0)
                {
                    full_wipe_reset();
                    reset_cpu();
                }
                else
                {
                    strcpy(rx_display_buffer, "UNKNOWN CMD");
                }
            }
        }
        else
        {
            if (packet_type == 'M')
                strcpy(rx_display_buffer, "FORMAT ERROR");
        }
    }
    scroll_pos = 0;
}

// Enter Low Power Sleep Mode
void enter_sleep_mode()
{
    lcd_putc('\f');
    lcd_locate(1, 1);
    printf(lcd_putc, "SLEEP MODE...");
    wdt_delay_ms(500);
    lcd_send_byte(0, 0x08); // Turn off LCD

    output_low(LED_PIN);
    output_low(BUZZER_PIN);

    while (TRUE)
    {
        restart_wdt();
        sleep(); // CPU Sleep

        // Wake up if any button is pressed
        if (!input(BTN_SIGNAL) || !input(BTN_UPLOAD) || !input(BTN_DELETE) || !input(BTN_RESET) || !input(BTN_MODE))
        {
            delay_ms(20); // Debounce
            if (!input(BTN_SIGNAL) || !input(BTN_UPLOAD) || !input(BTN_DELETE) || !input(BTN_RESET) || !input(BTN_MODE))
            {
                break;
            }
        }
    }

    lcd_init(); // Re-init LCD
    update_lcd();
    idle_counter = 0;
}

// Interrupt: Bluetooth Data Received (UART)
#INT_RDA
void serial_isr()
{
    char incoming;
    if (kbhit(BT_MODULE))
    {
        incoming = fgetc(BT_MODULE);

        if (incoming == '$') // Start of packet
        {
            rx_temp_index = 0;
        }
        if (incoming == '\n' || incoming == '\r') // End of packet
        {
            rx_temp_buffer[rx_temp_index] = '\0';
            rx_data_ready = 1;
        }
        else
        {
            if (rx_temp_index < 38)
            {
                rx_temp_buffer[rx_temp_index] = incoming;
                rx_temp_index++;
            }
        }
    }
}

// Interrupt: Timer1 (Handles Morse Input Timing)
#INT_TIMER1
void timer1_isr()
{
    set_timer1(63036);
    if (idle_counter < 32000)
        idle_counter++;

    int1 btn_current = !input(BTN_SIGNAL);

    if (btn_current)
    {
        idle_counter = 0;
        press_counter++; // Increment while button is held
        output_high(LED_PIN);
        output_high(BUZZER_PIN);
    }
    else
    {
        output_low(LED_PIN);
        output_low(BUZZER_PIN);
        if (btn_prev_state == 1) // Button released
        {
            idle_counter = 0;
            if (press_counter > 2)
            {
                // Determine if it was a Dot or Dash based on duration
                if (press_counter < 30)
                    morse_buffer[morse_index++] = '.';
                else
                    morse_buffer[morse_index++] = '-';

                if (morse_index > 9)
                    morse_index = 9;

                morse_buffer[morse_index] = '\0';
                update_needed = 1;
            }
            press_counter = 0;
        }
    }
    btn_prev_state = btn_current;
}

// Interrupt: Timer0 (Handles Text Scrolling Speed)
#INT_TIMER0
void timer0_isr()
{
    scroll_tick++;
    if (scroll_tick > 60)
    {
        scroll_tick = 0;
        scroll_now = 1;
    }
}

// --- Main Program ---
void main()
{
    setup_oscillator(OSC_8MHZ); // Set clock to 8MHz

    setup_wdt(WDT_OFF); // Disable WDT initially

    set_tris_b(0xFF); // Set PORTB as Inputs
    port_b_pullups(TRUE);
    output_drive(LED_PIN);
    output_drive(BUZZER_PIN);

    // Startup beep
    output_high(BUZZER_PIN);
    delay_ms(50);
    output_low(BUZZER_PIN);

    lcd_init();
    delay_ms(100);

    // Restore data from memory
    load_text_from_eeprom();
    load_bt_from_eeprom();

    // Timer Setup
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    set_timer1(63036);
    setup_timer_0(T0_INTERNAL | T0_DIV_256);

    // Enable Interrupts
    enable_interrupts(INT_TIMER1);
    enable_interrupts(INT_TIMER0);
    enable_interrupts(INT_RDA);
    enable_interrupts(GLOBAL);

    lcd_locate(1, 1);
    printf(lcd_putc, "Morse Telegraph"); // English Title
    delay_ms(1000);

    lcd_putc('\f');
    update_lcd();

    setup_wdt(WDT_2304MS); // Enable Watchdog

    while (TRUE)
    {
        restart_wdt();

        // Handle scrolling text
        if (scroll_now)
        {
            scroll_now = 0;
            if (rx_display_buffer[0] != '\0')
            {
                scroll_pos++;
                update_scroll_line();
            }
        }

        // Handle incoming Bluetooth data
        if (rx_data_ready)
        {
            process_incoming_nmea();
            save_bt_to_eeprom();
            rx_data_ready = 0;
            update_lcd();
            idle_counter = 0;
        }

        // Check for inactivity sleep
        if (idle_counter > SLEEP_TIMEOUT)
        {
            enter_sleep_mode();
        }

        // Update LCD if needed
        if (update_needed)
        {
            update_lcd();
            update_needed = 0;
        }

        // Shortcut: Force Sleep (Mode + Reset)
        if (!input(BTN_MODE) && !input(BTN_RESET))
        {
            delay_ms(50);
            if (!input(BTN_MODE) && !input(BTN_RESET))
            {
                enter_sleep_mode();
                while (!input(BTN_MODE) || !input(BTN_RESET))
                    restart_wdt();
            }
        }

        // Switch Mode (Mode pressed, Reset released)
        if (!input(BTN_MODE) && input(BTN_RESET))
        {
            idle_counter = 0;
            delay_ms(50);
            if (!input(BTN_MODE))
            {
                app_mode = !app_mode;
                update_lcd();
                while (!input(BTN_MODE))
                    restart_wdt();
            }
        }

        // Upload Button Logic
        if (!input(BTN_UPLOAD))
        {
            idle_counter = 0;
            int16 hold_counter = 0;

            // Detect long press vs short press
            while (!input(BTN_UPLOAD) && hold_counter < 50)
            {
                delay_ms(10);
                restart_wdt();
                hold_counter++;
            }

            // Long Press: Send Message
            if (hold_counter >= 50)
            {
                if (text_index > 0)
                {
                    send_nmea_packet();
                    output_high(BUZZER_PIN);
                    wdt_delay_ms(100);
                    output_low(BUZZER_PIN);
                }
                while (!input(BTN_UPLOAD))
                    restart_wdt();

                // Clear buffers
                text_index = 0;
                text_buffer[0] = '\0';
                morse_index = 0;
                morse_buffer[0] = '\0';
                save_text_to_eeprom();

                lcd_putc('\f');
                lcd_locate(1, 1);
                printf(lcd_putc, "DATA SENT"); // English feedback
                wdt_delay_ms(1000);
                lcd_putc('\f');
                update_lcd();
            }
            // Short Press: Add decoded char to text
            else
            {
                while (!input(BTN_UPLOAD))
                    restart_wdt();
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
        }

        // Delete Button (Backspace)
        if (!input(BTN_DELETE))
        {
            delay_ms(20);
            if (!input(BTN_DELETE))
            {
                idle_counter = 0;
                if (morse_index > 0)
                    morse_buffer[--morse_index] = '\0'; // Remove dot/dash
                else if (text_index > 0)
                {
                    text_buffer[--text_index] = '\0'; // Remove character
                    save_text_to_eeprom();
                }
                update_lcd();
                while (!input(BTN_DELETE))
                    restart_wdt();
            }
        }

        // Reset Button
        if (!input(BTN_RESET) && input(BTN_MODE))
        {
            idle_counter = 0;
            int16 reset_hold = 0;

            while (!input(BTN_RESET))
            {
                delay_ms(10);
                restart_wdt();
                reset_hold++;

                // Long Press: Factory Reset
                if (reset_hold > 200)
                {
                    full_wipe_reset();
                    while (!input(BTN_RESET))
                        restart_wdt();
                    break;
                }
            }

            // Short Press: Clear current text
            if (reset_hold > 5 && reset_hold <= 200)
            {
                text_index = 0;
                text_buffer[0] = '\0';
                morse_index = 0;
                morse_buffer[0] = '\0';
                update_lcd();
                while (!input(BTN_RESET))
                    restart_wdt();
            }
        }
    }
}