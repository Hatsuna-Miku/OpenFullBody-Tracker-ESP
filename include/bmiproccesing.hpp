#include <DFRobot_BMI160.h>
#define poolsize 256

DFRobot_BMI160 bmi160(D1,D2);
const int8_t i2c_addr = 0x69;
int fetchimu(int16_t* data,int16_t* calibration)
{
  int16_t AG[6]={0}; 
  int i;
  int result = bmi160.getAccelGyroData(AG);
  if(result!=0)
  {
      return 1;
  }
  else
  {
    for(i=0;i<3;i++)
    {
      AG[i] -= calibration[i];
    }
    for(i=0;i<3;i++)
    {
      data[i+3] = 0;
    }
    for(i=0;i<6;i++)
    {
      data[i] += AG[i];
      //data[i] /= 2;
    }
  }
  return 0;
}

void connectimu()
{
  //init the hardware bmin160  
  if (bmi160.softReset() != BMI160_OK){
    Serial.println("reset false");
    while(1);
  }
  
  //set and init the bmi160 i2c address
  if (bmi160.I2cInit(i2c_addr) != BMI160_OK){
    Serial.println("init false");
    while(1);
  }
  pinMode(2,OUTPUT);
}

int calibrateimu(int16_t* data)
{
  int16_t AG[6]={0}; 
  long pool[3][poolsize];
  int i,j;
  int result = bmi160.getAccelGyroData(AG);
  if(result!=0)
  {
      return 1;
  }
  else
  {
    digitalWrite(2,LOW);
    for(j=0;j<poolsize;j++)
    {
      bmi160.getAccelGyroData(AG);
      for(i=0;i<3;i++)
      {
        pool[i][j] = AG[i];
      }
    }
    for(i=0;i<3;i++)
    {
      for(j=0;j<poolsize;j++)
      {
        data[i] += pool[i][j];
      }
      data[i] /= poolsize;
    }
  }
  digitalWrite(2,HIGH);
  return 0;
}