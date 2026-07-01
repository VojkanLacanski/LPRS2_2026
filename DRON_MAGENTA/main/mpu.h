#pragma once
#include <stdint.h>

#define I2C_MASTER_SCL_IO     15                //port MCU na koji se povezuje sat
#define I2C_MASTER_SDA_IO     18                //port MCU za data
#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_FREQ_HZ    400000           //frekvencija rada clock-a - FAST MODE
#define MPU6050_ADDR          0x68             //AD0 pin = 0 / GND
#define MIN_IDLE_PWM          1190
#define MAX_PWM               2000
#define DEADZONE              1002

// extern - "ova promenljiva postoji negde drugo, ali je koristim ovde"
extern volatile int throttle;
extern volatile long last_packet_time;

typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_error;
} pid_controller_t;

void i2c_init();
void mpu6050_init(void);
void mpu6050_calibrate();
void mpu6050_read(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz);
int16_t make_int16(uint8_t high, uint8_t low);

uint32_t calculate_duty(int microseconds);
float pid_update(pid_controller_t *pid, float setpoint, float measured, float dt);

void flight_control_task(void *pvParameter);
