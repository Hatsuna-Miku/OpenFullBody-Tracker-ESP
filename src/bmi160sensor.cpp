
#include "sensor.h"
#include "udpclient.h"
#include <i2cscan.h>
#include "calibration.h"
#include "configuration.h"
#if IMU == IMU_BMI160
#define poolsize 256


void BMI160Sensor::motionLoop()
{
    int i;
    int result = imu.getAccelGyroData(raw.AccelGyro);
    for(i=0;i<6;i++)
    {
        if(i<=3)
        {
            AG.AccelGyro[i] = raw.AccelGyro[i] / 32768.0 * 2000/360*2*PI - calibrationdata[i];
        }
        else
        {
            AG.AccelGyro[i] = raw.AccelGyro[i]/32768.0*8;
        }
    }
    for(i=0;i<3;i++)
    {
        pose[i] += AG.Accel[i];
        Serial.print(pose[i]);Serial.print(" ");
    }
    Serial.print("\n");
    quaternion.set_euler_xyz(pose);
    newData = true;
}

void BMI160Sensor::motionSetup()
{
    int i;
  //init the hardware bmin160  
  if (imu.softReset() != BMI160_OK){
    Serial.println("reset false");
    while(1);
  }
  
  //set and init the bmi160 i2c address
  if (imu.I2cInit(BMI_ADDR) != BMI160_OK){
    Serial.println("init false");
    while(1);
  }
  for(i=0;i<3;i++)
    {
        calibrationdata[i] = 0;
    }
    startCalibration(CALIBRATION_TYPE_INTERNAL_ACCEL);
  pinMode(2,OUTPUT);
}

void BMI160Sensor::startCalibration(int calibrationType)
{
    int i,j;
    int result = imu.getAccelGyroData(raw.AccelGyro);
    digitalWrite(2,LOW);
    for(j=0;j<poolsize;j++)
    {
        imu.getAccelGyroData(raw.AccelGyro);
        for(i=0;i<3;i++)
        {
            calibrationdata[i] += raw.AccelGyro[i]/32768*2000/360*2*PI;
        }
        ESP.wdtFeed();
    }
    for(i=0;i<3;i++)
    {
        calibrationdata[i] /= poolsize;
    }
    digitalWrite(2,HIGH);
    switch(calibrationType) {
        case CALIBRATION_TYPE_INTERNAL_ACCEL:
            sendCalibrationFinished(CALIBRATION_TYPE_INTERNAL_ACCEL, 0, PACKET_RAW_CALIBRATION_DATA);
            break;
        case CALIBRATION_TYPE_INTERNAL_GYRO:
            sendCalibrationFinished(CALIBRATION_TYPE_INTERNAL_ACCEL, 0, PACKET_RAW_CALIBRATION_DATA);
            break;
    }
}

void BMI160Sensor::setupBMI160(uint8_t sensorId, uint8_t addr, uint8_t intPin) {
    this->addr = addr;
    this->intPin = intPin;
    this->sensorId = sensorId;
    this->sensorOffset = {Quat(Vector3(0, 0, 1), sensorId == 0 ? IMU_ROTATION : SECOND_IMU_ROTATION)};
}

void BMI160Sensor::sendData() {
    if(newData) {
        sendQuat(&quaternion, PACKET_ROTATION);
        newData = false;
    }
}
#endif
