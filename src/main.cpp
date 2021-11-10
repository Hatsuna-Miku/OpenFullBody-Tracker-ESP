/*
    OpenFullBody Code is placed under the MIT license
    Copyright (c) 2021 Eiren Rain

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include "Wire.h"
#include "ota.h"
#include "sensor.h"
#include "configuration.h"
#include "wifihandler.h"
#include "udpclient.h"
#include "defines.h"
#include "credentials.h"
#include <i2cscan.h>
#include "serialcommands.h"
#include "ledstatus.h"
#include "EEPROM.h"

int stat;

#if IMU == IMU_BNO080 || IMU == IMU_BNO085
    BNO080Sensor sensor{};
    #if defined(PIN_IMU_INT_2)
        #define HAS_SECOND_IMU true
        BNO080Sensor sensor2{};
    #endif
#elif IMU == IMU_BNO055
    BNO055Sensor sensor{};
#elif IMU == IMU_MPU9250
    MPU9250Sensor sensor{};
#elif IMU == IMU_MPU6500 || IMU == IMU_MPU6050
    MPU6050Sensor sensor{};
    #if defined(HAS_SECOND_IMU)
        MPU6050Sensor sensor2{};
    #endif
#elif IMU == IMU_BMI160
    BMI160Sensor sensor{};
#else
    #error Unsupported IMU
#endif
#ifndef HAS_SECOND_IMU
    EmptySensor sensor2{};
#endif

bool isCalibrating = false;
bool blinking = false;
unsigned long blinkStart = 0;
unsigned long now_ms, last_ms = 0; //millis() timers
unsigned long last_battery_sample = 0;
bool secondImuActive = false;

void commandRecieved(int command, void * const commandData, int commandDataLength)
{
    switch (command)
    {
    case COMMAND_CALLIBRATE:
        isCalibrating = true;
        break;
    case COMMAND_SEND_CONFIG:
        sendConfig(getConfigPtr(), PACKET_CONFIG);
        break;
    case COMMAND_BLINK:
        blinking = true;
        blinkStart = now_ms;
        break;
    }
}

float vol2per(float);

void setup()
{
    //wifi_set_sleep_type(NONE_SLEEP_T);
    // Glow diode while loading
    #ifdef STARTUPRESET
    void(* resetFunc) (void) = 0;
    EEPROM.begin(4);
    stat=EEPROM.read(0);
    Serial.printf("Status:%d/n",stat);
    EEPROM.write(0,stat+1);
    EEPROM.commit();
    if(stat%2==0)
    {
        Serial.printf("Restarting...");
        resetFunc();
    }
    #endif

    pinMode(LOADING_LED, OUTPUT);
    pinMode(CALIBRATING_LED, OUTPUT);
    #ifdef ONE_BUTTON_STARTUP
        pinMode(13,OUTPUT);
        pinMode(12,INPUT);
        digitalWrite(13,HIGH);
        while(digitalRead(12)==0);
    #endif
    digitalWrite(CALIBRATING_LED, HIGH);
    digitalWrite(LOADING_LED, LOW);
    delay(10);
    Serial.begin(serialBaudRate);
    setUpSerialCommands();
#if IMU == IMU_MPU6500 || IMU == IMU_MPU6050 || IMU == IMU_MPU9250
    I2CSCAN::clearBus(PIN_IMU_SDA, PIN_IMU_SCL); // Make sure the bus isn't suck when reseting ESP without powering it down
    // Do it only for MPU, cause reaction of BNO to this is not investigated yet
#endif
    // join I2C bus
    Wire.begin(PIN_IMU_SDA, PIN_IMU_SCL);
#ifdef ESP8266
    Wire.setClockStretchLimit(150000L); // Default streatch limit 150mS
#endif
    Wire.setClock(I2C_SPEED);

    getConfigPtr();
    setConfigRecievedCallback(setConfig);
    setCommandRecievedCallback(commandRecieved);
    // Wait for IMU to boot
    delay(500);
    
    // Currently only second BNO08X is supported
#if IMU == IMU_BNO080 || IMU == IMU_BNO085
    #ifdef HAS_SECOND_IMU
        uint8_t first = I2CSCAN::pickDevice(BNO_ADDR_1, BNO_ADDR_2, true);
        uint8_t second = I2CSCAN::pickDevice(BNO_ADDR_2, BNO_ADDR_1, false);
        if(first != second) {
            sensor.setupBNO080(0, first, PIN_IMU_INT);
            sensor2.setupBNO080(1, second, PIN_IMU_INT_2);
            secondImuActive = true;
        } else {
            sensor.setupBNO080(0, first, PIN_IMU_INT);
        }
    #else
    sensor.setupBNO080(0, I2CSCAN::pickDevice(BNO_ADDR_1, BNO_ADDR_2, true), PIN_IMU_INT);
    #endif
#endif
#if IMU == IMU_BMI160
    sensor.setupBMI160(0, I2CSCAN::pickDevice(BMI_ADDR, BMI_ADDR, true), PIN_IMU_INT);
#endif
#if IMU == IMU_MPU6050 && HAS_SECOND_IMU
    if (I2CSCAN::isI2CExist(0x68) && I2CSCAN::isI2CExist(0x69)) {
        sensor2.setSecond();
        secondImuActive = true;
    } else {
        Serial.println("[ERR] HAS_SECOND_IMU defined but can't find I2C devices 0x68 and 0x69. Running in single sensor mode.");
    }
#endif

    sensor.motionSetup();
#ifdef HAS_SECOND_IMU
    if(secondImuActive)
        sensor2.motionSetup();
#endif

    setUpWiFi();
    otaSetup(otaPassword);
    digitalWrite(LOADING_LED, HIGH);
}

// AHRS loop

void loop()
{
    #ifdef ONE_BUTTON_STARTUP
    if(digitalRead(12)==0)
    {
        digitalWrite(2,LOW);
        delay(200);
        digitalWrite(2,HIGH);
        delay(200);
        digitalWrite(2,LOW);
        delay(200);
        digitalWrite(2,HIGH);
        delay(200);
        digitalWrite(2,LOW);
        delay(200);
        digitalWrite(2,HIGH);
        delay(200);
        digitalWrite(13,LOW);
    }
    #endif
    ledStatusUpdate();
    serialCommandsUpdate();
    wifiUpkeep();
    otaUpdate();
    clientUpdate(&sensor, &sensor2);
    if (isCalibrating)
    {
        sensor.startCalibration(0);
        //sensor2.startCalibration(0);
        isCalibrating = false;
    }
#ifndef UPDATE_IMU_UNCONNECTED
        if(isConnected()) {
#endif
    sensor.motionLoop();
#ifdef HAS_SECOND_IMU
    sensor2.motionLoop();
#endif
#ifndef UPDATE_IMU_UNCONNECTED
        }
#endif
    // Send updates
    now_ms = millis();
#ifndef SEND_UPDATES_UNCONNECTED
    if(isConnected()) {
#endif
        sensor.sendData();
#ifdef HAS_SECOND_IMU
        sensor2.sendData();
#endif
#ifndef SEND_UPDATES_UNCONNECTED
    }
#endif
#ifdef PIN_BATTERY_LEVEL
    if(now_ms - last_battery_sample >= batterySampleRate) {
        last_battery_sample = now_ms;
        float battery = ((float) analogRead(PIN_BATTERY_LEVEL)) * batteryADCMultiplier;
        battery = vol2per(battery);
        sendFloat(battery, PACKET_BATTERY_LEVEL);
    }
#endif
}

float vol2per(float vol)
{
    if(vol>=4.2)
    {
        return 100.0;
    }
    else if(vol<4.2 && vol>=4.06)
    {
        return 90.0+((vol-4.06)/(4.2-4.06)*10.0);
    }
    else if(vol<4.06 && vol>=3.98)
    {
        return 80.0+((vol-3.98)/(4.06-3.98)*10.0);
    }
    else if(vol<3.98 && vol>=3.92)
    {
        return 70.0+((vol-3.92)/(-3.92)*10.0);
    }
    else if(vol<3.92 && vol>=3.87)
    {
        return 60.0+((vol-3.87)/(-3.87)*10.0);
    }
    else if(vol<3.87 && vol>=3.82)
    {
        return 50.0+((vol-3.82)/(-3.82)*10.0);
    }
    else if(vol<3.82 && vol>=3.79)
    {
        return 40.0+((vol-3.79)/(-3.79)*10.0);
    }
    else if(vol<3.79 && vol>=3.77)
    {
        return 30.0+((vol-3.77)/(-3.77)*10.0);
    }
    else if(vol<3.77 && vol>=3.74)
    {
        return 20.0+((vol-3.74)/(-3.74)*10.0);
    }
    else if(vol<3.74 && vol>=3.68)
    {
        return 10.0+((vol-3.68)/(-3.68)*10.0);
    }
    else if(vol<3.68 && vol>=3.45)
    {
        return 5.0+((vol-3.45)/(-3.45)*5.0);
    }
    else if(vol<3.45 && vol>=3.10)
    {
        return 0.0+((vol-3.10)/(-3.10)*5.0);
    }
    return -1;
}