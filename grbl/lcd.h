#ifndef LCD_H
#define LCD_H

#define LCD_START_LINE1  0x00
#define LCD_START_LINE2  0x40

#define LCD_DELAY_BOOTUP   16000
#define LCD_DELAY_INIT      5000
#define LCD_DELAY_INIT_REP    64
#define LCD_DELAY_INIT_4BIT   64
#define LCD_DELAY_BUSY_FLAG    4
#define LCD_DELAY_ENABLE_PULSE 1

#define LCD_CLR               0
#define LCD_ENTRY_MODE        2
#define LCD_ENTRY_INC         1
#define LCD_DDRAM             7
#define LCD_BUSY              7

#define LCD_DISP_OFF             0x08
#define LCD_DISP_ON              0x0C

#define LCD_FUNCTION_4BIT_2LINES 0x28

#define LCD_MODE_DEFAULT         ((1<<LCD_ENTRY_MODE)|(1<<LCD_ENTRY_INC))

#define lcd_puts_P(__s) lcd_puts_p(PSTR(__s))

void lcd_init();
void lcd_clrscr(void);
void lcd_gotoxy(uint8_t x, uint8_t y);
void lcd_putc(char c);
void lcd_puts_p(const char *progmem_s);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);

void lcd_position();
void lcd_state();
void lcd_state2();
void lcd_clear();
void lcd_file(const char *s);
void lcd_error(uint8_t status_code);

#endif
