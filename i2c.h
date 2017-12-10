#ifndef I2C_H__
#define I2C_H__
#include <stdbool.h>

#define SLAVE_ADDR 0x53
#define BUF_SIZE 4095


uint8_t buf[BUF_SIZE];

void i2c_setup();
void i2c_idle();
bool i2c_send(uint8_t data);
uint8_t i2c_recv();
void i2c_ack();
void i2c_nack();
void i2c_start();
void i2c_restart();
void i2c_stop();

void master_try_read();
void master_handle_read(const uint16_t len, uint8_t data[len]);
void master_write(uint8_t data[], uint16_t len);
#endif // I2C_H__
