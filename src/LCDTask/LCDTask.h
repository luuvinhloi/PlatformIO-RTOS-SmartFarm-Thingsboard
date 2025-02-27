#ifndef LCD_TASK_H
#define LCD_TASK_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <array>


// Địa chỉ I2C của màn hình LCD
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2
#define SDA_PIN 21 // SDA - GPIO 21
#define SCL_PIN 22 // SCL - GPIO 22


// Khởi tạo các đối tượng
extern LiquidCrystal_I2C lcd;


#endif
