char *fixed_to_string(uint16_t num, char *buf);
void display_update();
void display_string(int line, char *s);
void display_init();
uint8_t spi_send_recv(uint8_t data);
void delay(int cyc);
static const uint8_t const font[];
char textbuffer[4][16];
#define DISPLAY_VDD_PORT PORTF
#define DISPLAY_VDD_MASK 0x40
#define DISPLAY_VBATT_PORT PORTF
#define DISPLAY_VBATT_MASK 0x20
#define DISPLAY_COMMAND_DATA_PORT PORTF
#define DISPLAY_COMMAND_DATA_MASK 0x10
#define DISPLAY_RESET_PORT PORTG
#define DISPLAY_RESET_MASK 0x200
void enable_interrupt(void);

