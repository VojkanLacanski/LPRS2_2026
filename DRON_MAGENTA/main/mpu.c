#include <stdio.h>
#include <math.h>

#include "driver/i2c.h"    //config, read, write
#include "driver/ledc.h"   //za mototre
#include "driver/gpio.h"   //GPIO_PULLUP_ENABLE

#include "freertos/FreeRTOS.h"  //makroi
#include "freertos/task.h"      //pravljenje taskova
#include "mpu.h"                //definicije pinova, adrese MPU6050

#define ESC_FREQ 400  //50Hz je univerzalna frekvencija koju vecina ESC prepoznaje ali mi radimo sa 400
#define ESC_PERIOD_US (1000000.0f / ESC_FREQ)           //period  PWM signala na osnovu frekvencije- 2500 mikros

extern volatile float global_kp;
extern volatile float global_ki;
extern volatile float global_kd;
extern volatile float global_kp_yaw;
extern volatile float global_ki_yaw;
extern volatile float global_kd_yaw;
float accel_x_offset, accel_y_offset = 0;
float gyro_x_offset, gyro_y_offset = 0, gyro_z_offset = 0;

uint32_t calculate_duty(int microseconds)
{
    return (uint32_t)((microseconds / ESC_PERIOD_US) * 8192.0);     //T = 1 / f -> T = 1 / 50 = 0,02s = 20000us (period)
                                            //PWM rezolucija 13bit
}
 
//inicijalizacija I2C konfiguracije
void i2c_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,                //inicira komunikaciju (slave salje podatke)
        .sda_io_num = I2C_MASTER_SDA_IO,        //SDA pin -data
        .scl_io_num = I2C_MASTER_SCL_IO,        //SCL pin -clock
        .sda_pullup_en = GPIO_PULLUP_ENABLE,    //ukljucenje otpornika koji drzi liniju na HIGH dok uredjaji ne spoje na GND da bi poslali 0 
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ  //brzina clock-a
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);  //ucitava konfiguraciju u I2C kontroler  (hardver) koji koristim - I2C_NUM_0
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);      //drajver postaje aktivan
}                     //broj ,rezim rada, receive, transmit, interrupt


//pomocna funkcija za I2C komunikaciju (adresa registra, vrednost)
void mpu6050_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};

    //sladnje podataka MPU6050
    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM,     //broj porta na koji se salje
        MPU6050_ADDR,       
        data,              //registar i vrednost
        2,                 //velicina u bajtovima
        pdMS_TO_TICKS(100)
    );

    if(ret != ESP_OK)
    {
        printf("MPU write error reg=0x%02X ret=%d\n", reg, ret);
    }
}


//pozivajne senzora, budjenje
void mpu6050_init()
{
    uint8_t data[2] = {0x6B, 0x00};   // podesavanje 0x6B registra- izadji iz sleep mode
    i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, data, 2, pdMS_TO_TICKS(100));  //port, adresa uredjaja, bafer za niz koji saljem, velicina, cekanje taska da se zavrsi

    // DLPF filter - smanjuje sum gyro/accel signala, oko 44Hz gyro / 42Hz accel,
    mpu6050_write_reg(0x1A, 0x02);
    //mpu6050_write_reg(0x1A, 0x00);
}


int16_t make_int16(uint8_t high, uint8_t low)
{
    return (int16_t)((high << 8) | low);
}


void mpu6050_calibrate()
{
    int16_t ax, ay, az, gx, gy, gz;
    long sum_ax = 0;
    long sum_ay = 0;
    //kalibracija za az nije potrebna jer je greska u odnosu na silu zemljine teze (16384.0f) zanemarljiva

    long sum_gx = 0;
    long sum_gy = 0;
    long sum_gz = 0;

    int samples = 4000;

    for(int i = 0; i < samples; i++) 
    {
        mpu6050_read(&ax, &ay, &az, &gx, &gy, &gz);

        sum_ax += ax;
        sum_ay += ay;

        sum_gx += gx;
        sum_gy += gy;
        sum_gz += gz;

        vTaskDelay(pdMS_TO_TICKS(2)); // Mali razmak između čitanja
    }

    accel_x_offset = (float)sum_ax / samples;
    accel_y_offset = (float)sum_ay / samples;

    gyro_x_offset = (float)sum_gx / samples;
    gyro_y_offset = (float)sum_gy / samples;
    gyro_z_offset = (float)sum_gz / samples;
}


void mpu6050_read_raw(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t reg = 0x3B;             //pocetak citanja je od registra ACCEL_XOUT_H.
    uint8_t data[14];

    //funkcija za brzo ocitavanje podataka u memoriji, olaksava i2c
    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MPU6050_ADDR,
        &reg,
        1,
        data,
        14,
        pdMS_TO_TICKS(100)
    );

    //osiguranje da u slucaju neprocitanih podataka se ne posalje djubre iz memorije
    if(ret != ESP_OK)
    {
        printf("MPU read error ret=%d\n", ret);

        *ax = 0;
        *ay = 0;
        *az = 0;
        *gx = 0;
        *gy = 0;
        *gz = 0;
        return;
    }

    *ax = make_int16(data[0],  data[1]);
    *ay = make_int16(data[2],  data[3]);
    *az = make_int16(data[4],  data[5]);

    *gx = make_int16(data[8],  data[9]);
    *gy = make_int16(data[10], data[11]);
    *gz = make_int16(data[12], data[13]);
}


void mpu6050_read(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz)
{
    int16_t raw_ax, raw_ay, raw_az;
    int16_t raw_gx, raw_gy, raw_gz;

    mpu6050_read_raw(&raw_ax, &raw_ay, &raw_az, &raw_gx, &raw_gy, &raw_gz);

    *ax = raw_ax - (int16_t)accel_x_offset;
    *ay = raw_ay - (int16_t)accel_y_offset;

    //z osi ne skidamo offset
    *az = raw_az;

    *gx = raw_gx - (int16_t)gyro_x_offset;
    *gy = raw_gy - (int16_t)gyro_y_offset;
    *gz = raw_gz - (int16_t)gyro_z_offset;
    
}

//PID KONTROLER
float pid_update(pid_controller_t *pid, float setpoint, float measured, float dt)
{ 
    float error = setpoint - measured;                    //stepen(greska) = koliko hocu - koliko je izmereno

    if (fabs(error) < 0.2f)            //provera da li ima potrebe za korekcijom- ako je minimalna promena ne treba korekcija
    {
        pid->integral = 0; // Resetuj integral dok smo u mirnoj zoni
        return 0;
    }

    pid->integral += error * dt;                          //akumulacija greske- P ispod krive, drift, POTENCIJALNO UVESTI GRANICE (RESENO)

    if(pid->integral > 50.0f) 
        pid->integral = 50.0f;

    if(pid->integral < -50.0f) 
        pid->integral = -50.0f;

    float derivative = (error - pid->prev_error) / dt;   //koliko se brzo menja greska, stepen/s

    pid->prev_error = error;    //trenutna greska postaje prethodna za sledecu iteraciju

    //korekcija je zbir doprinosa sve 3 komponente
    float result= pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;

    return result;
}


void flight_control_task(void *pvParameter)
{
    int16_t ax, ay, az, gx, gy, gz;         //sirovo iz senzora

    float roll = 0, pitch = 0;
    float alpha = 0.98f;         //double sporiji, drzi se float-a

    pid_controller_t pid_roll;          //rotacija levo-desno
    pid_roll.integral = 0;
    pid_roll.prev_error = 0;

    pid_controller_t pid_pitch;      //rotacija napred-nazad
    pid_pitch.integral = 0;
    pid_pitch.prev_error = 0;

    pid_controller_t pid_yaw;      //rotacija oko vertikalne ose
    pid_yaw.integral = 0;
    pid_yaw.prev_error = 0;

    float dt = 0.010f; //vreme izmedju merenja 1/100Hz

    TickType_t last_wake = xTaskGetTickCount();      //sitna vremenska jedinica, broj tikova od pocetka sistema
    //petlja radi na 10ms

    //promenljive za softverski filtar
    float lpf_faktor = 0.04f; 
    float ax_prev = 0.0f;
    float ay_prev = 0.0f;
    float az_prev = 0.0f;


    while(1)
    {
        //citanje vrednosti sa senzora
        mpu6050_read(&ax, &ay, &az, &gx, &gy, &gz);

        //AZURIRANJE PID PARAMETARA U REALNOM VREMENU
        pid_roll.kp = global_kp;
        pid_roll.ki = global_ki;
        pid_roll.kd = global_kd;

        pid_pitch.kp = global_kp;
        pid_pitch.ki = global_ki;
        pid_pitch.kd = global_kd;

        pid_yaw.kp = global_kp_yaw;
        pid_yaw.ki = global_ki_yaw;
        pid_yaw.kd = global_kd_yaw;

        //FAIL-SAFE - nije se desila isporuka paketa
        if((xTaskGetTickCount() - last_packet_time) > pdMS_TO_TICKS(5000))
        {
            if(throttle > 1000)
            {
               
                throttle -= 2;   // Pa ga lagano spusti do kraja
                
            }

            if(throttle < 1000)
                throttle = 1000;
        }

        //pretvaranje raw vrednosti u fizicke vrednosti

        // 1. Primena softverskog NP filtera
        float ax_filt = (lpf_faktor * ax) + ((1.0f - lpf_faktor) * ax_prev);
        float ay_filt = (lpf_faktor * ay) + ((1.0f - lpf_faktor) * ay_prev);
        float az_filt = (lpf_faktor * az) + ((1.0f - lpf_faktor) * az_prev);

        // Čuvanje vrednosti za sledeći krug
        ax_prev = ax_filt;
        ay_prev = ay_filt;
        az_prev = az_filt;

        // 2. Pretvaranje u ubrzanje (sada koristiš filtrirane podatke)
        float accelX = ax_filt / 16384.0f;
        float accelY = ay_filt / 16384.0f;
        float accelZ = az_filt / 16384.0f;

        float gyroX = gx / 131.0f;
        float gyroY = gy / 131.0f;
        float gyroZ = gz / 131.0f;

        float roll_acc = atan2f(accelY, accelZ) * 57.3f ;   //radijani u stepene
        float pitch_acc = atan2f(-accelX, sqrtf(accelY*accelY + accelZ*accelZ)) * 57.3f ;

        roll = alpha * (roll + gyroX * dt) + (1-alpha) * roll_acc;
        pitch = alpha * (pitch + gyroY * dt) + (1-alpha) * pitch_acc;

        if (throttle < 1050) 
        {
            pid_roll.integral = 0;
            pid_pitch.integral = 0;
            pid_yaw.integral = 0;
        }


        float roll_corr = pid_update(&pid_roll, 0.0f, roll, dt);      //korekcija u zavisnosti od zeljene vrendnosti, izmerenog roll i proteklog vremena
        float pitch_corr = pid_update(&pid_pitch, 0.0f, pitch, dt);
        float yaw_corr = pid_update(&pid_yaw, 0.0f, gyroZ, dt);

        int base;

        if(throttle > DEADZONE)
        {
            //LINEARNO MAPIRANJE: 
            //pretvaramo ulaznih 1000-2000 u izlaznih 1190-2000
            //formula: output = out_min + (input - in_min) * (out_max - out_min) / (in_max - in_min)
            //resavamo problem kalibracije ESC-a zbog koje nam se motori pale tek na 1200 nezavisno jedan od drugog
            base = MIN_IDLE_PWM + (throttle - 1000) * (MAX_PWM - MIN_IDLE_PWM) / (2000 - 1000);
        }else{
            base = 1000;
        }

        if(base > DEADZONE)
        {
            int m1 = base + pitch_corr + roll_corr - yaw_corr;  //zadnji levi
            int m2 = base - pitch_corr + roll_corr + yaw_corr;  //prednji levi
            int m3 = base - pitch_corr - roll_corr - yaw_corr;  //prednji desni
            int m4 = base + pitch_corr - roll_corr + yaw_corr;  //zadnji desni

            if(m1 < 1000) 
                m1 = 1000;   
            if(m1 > 2000) 
                m1 = 2000;

            if(m2 < 1000) 
                m2 = 1000; 
            if(m2 > 2000) 
                m2 = 2000;

            if(m3 < 1000) 
                m3 = 1000;
            if(m3 > 2000) 
                m3 = 2000;

            if(m4 < 1000) 
                m4 = 1000;
            if(m4 > 2000) 
                m4 = 2000;

            ledc_set_duty(LEDC_LOW_SPEED_MODE, 0, calculate_duty(m1));       //nacin azuriranja hardvera, ESC signal spor pa nema poente HIGH SPEED
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 1, calculate_duty(m2));
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 2, calculate_duty(m3));
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 3, calculate_duty(m4));

            for(int i=0;i<4;i++)
                ledc_update_duty(LEDC_LOW_SPEED_MODE, i);
        }else{
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 0, calculate_duty(1000));
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 1, calculate_duty(1000));
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 2, calculate_duty(1000));
            ledc_set_duty(LEDC_LOW_SPEED_MODE, 3, calculate_duty(1000));

            for(int i=0;i<4;i++)
                ledc_update_duty(LEDC_LOW_SPEED_MODE, i);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    }
}