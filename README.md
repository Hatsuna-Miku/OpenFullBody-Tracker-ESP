OpenFullBody 的第一个版本Fork自SlimeVR项目。
# 为ESP系列MCU定制的的易追追踪器固件
# OpenFullBody Tracker firmware for ESP

这个固件可以让ESP8266/ESP32与不同IMU结合起来作为全身追踪传感器在VR环境下使用
Firmware for ESP8266 / ESP32 microcontrollers and different IMU sensors to use them as a vive-like trackers in VR.

需要 [易追服务端](https://github.com/Hatsuna-Miku/OpenFullBody-Server) 来解算姿态以及配合SteamVR进行使用。应该可以兼容 [owoTrack](https://github.com/abb128/owo-track-driver) ，但是不保证。
Requires [OpenFullBody Server](https://github.com/Hatsuna-Miku/OpenFullBody-Server) to work with SteamVR and resolve pose. Should be compatible with [owoTrack](https://github.com/abb128/owo-track-driver), but is not guaranteed.

## 兼容性
## Compatibility

对下列IMU进行过测试并确保兼容（编译之前改源文件来选择）
Compatible and tested with these IMUs (select during compilation):
* BNO085, BNO086
  * 充分利用其自带的运动处理器功能，在适合的磁环境下能为AR和VR应用提供最稳定的姿态追踪。
  * Using any fusion in internal DMP. Best results with ARVR Stabilized Game Rotation Vector or ARVR Stabilized Rotation Vector if in good magnetic environment
* BNO080
  * 充分利用其自带的运动处理器功能，没有BNO085那么稳定，但是效果依然很棒。
  * Using any fusion in internal DMP. Doesn't have BNO085's ARVR stabilization, but still gives good results.
* BNO055
  * 按理来说应该和BNO080差不太多，但是更便宜。没有完全测试过。
  * Should be roughly equal BNO080, but cheaper. Not tested thoroughly, please report your results on Discord if you're willing to try.
* MPU-9250
  * 用算法对九轴数据融合（陀螺仪，加速度传感器以及磁传感器），需要良好的磁环境。
  * Using Mahony sensor fusion of Gyroscope, Magnetometer and Accelerometer, requires good magnetic environment.
  * 注意：暂时不能对其进行校准，因为服务端还没开发出对应的指令，正在努力开发这一块。可以用作MPU-6050的下属磁传感器。
  * NOTE: Currently can't be calibrated due to lack of proper commands from the server. Work in progress. Can be ran as MPU-6050 below w/o magnetometer.
* MPU-6500 & MPU-6050
  * 利用了自带的运动处理器功能融合六轴数据（陀螺仪和加速度传感器），可以单独使用MPU-9250的磁传感器功能，漂移比较严重。
  * Using internal DMP to fuse Gyroscope and Accelerometer, can be used with MPU-9250 to use it without accelerometer, can drift substantially.
  * 注意：目前版本下MPU上电时会自动校准。切记把它平放在地上，一定不要乱动它，直到它校准完成（需要花个几秒钟）。**校准完成时，ESP的LED会闪烁5次**
  * NOTE: Currently the MPU will auto calibrate when powered on. You *must* place it on the ground and *DO NOT* move it until calibration is complete (for a few seconds). **LED on the ESP will blink 5 times after calibration is over.**

这个固件在ESP8266和ESP32上都能跑，在上传固件之前请根据你和IMU的连接方式修改 defines.h 里的引脚定义。
Firmware can work with both ESP8266 and ESP32. Please edit defines.h and set your pinout properly according to how you connected the IMU.
