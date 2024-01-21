#include <Arduino.h>

// MPU6050 mpu(Wire);

void setup_mpu_6050_registers(){
  //Activate the MPU-6050
  Wire.beginTransmission(0x68);                       //Start communicating with the MPU-6050
  Wire.write(0x6B);                                   //Send the requested starting register
  Wire.write(0x00);                                   //Set the requested starting register
  Wire.endTransmission();                             //End the transmission

  //Configure the accelerometer (+/-8g)               // Other confifuration
  Wire.beginTransmission(0x68);                  
  Wire.write(0x1C);                         
  Wire.write(0x10);                    
  Wire.endTransmission();               
  //Configure the gyro (500dps full scale)
  Wire.beginTransmission(0x68);                                
  Wire.write(0x1B);                         
  Wire.write(0x08);            
  Wire.endTransmission();

}

void read_mpu_6050_data(){                            //Subroutine for reading the raw gyro and accelerometer data
  yield();
  Wire.beginTransmission(0x68);                       //Start communicating with the MPU-6050
  Wire.write(0x3B);                                   //Send the requested starting register
  Wire.endTransmission(false);                        //End the transmission
  Wire.requestFrom((int) 0x68,(int) 14, (int) true);  //Request 14 bytes from the MPU-6050

  ax = Wire.read()<<8|Wire.read();                    //Add the low and high byte to the acc_x variable
  ay = Wire.read()<<8|Wire.read();                    //Add the low and high byte to the acc_y variable
  az = Wire.read()<<8|Wire.read();                    //Add the low and high byte to the acc_z variable
  rawTemp = Wire.read()<<8|Wire.read();           //Add the low and high byte to the temperature variable
  gx = Wire.read()<<8|Wire.read();                    //Add the low and high byte to the gyro_x variable
  gy = Wire.read()<<8|Wire.read();                    //Add the low and high byte to the gyro_y variable
  gz = Wire.read()<<8|Wire.read();                    //Add the low and high byte to the gyro_z variable

}

void set_last_read_angle_data(unsigned long time, float x, float y, float z, float x_gyro, float y_gyro, float z_gyro) {
  last_read_time = time;
  last_x_angle = x;
  last_y_angle = y;
  //last_z_angle = z;
  last_gyro_x_angle = x_gyro;
  last_gyro_y_angle = y_gyro;
  //last_gyro_z_angle = z_gyro;
}

void calibrate_sensors() {
  int num_readings = 200;
   
  read_mpu_6050_data();                       // Discard the first reading
  for (int i = 0; i < num_readings; i++) {    // Read and average the raw values
    read_mpu_6050_data();
    base_x_gyro += gx;
    base_y_gyro += gy;
    base_z_gyro += gz;
    base_x_accel += ax;
    base_y_accel += ay;
    base_y_accel += az;
  }
  base_x_gyro /= num_readings;
  base_y_gyro /= num_readings;
  base_z_gyro /= num_readings;
  base_x_accel /= num_readings;
  base_y_accel /= num_readings;
  base_z_accel /= num_readings;
  
  set_last_read_angle_data(millis(), 0, 0, 0, 0, 0, 0);
}

void calc_mpu_angle () {
  read_mpu_6050_data();
  t_now = millis();   // Get time of last raw data read
  yield();
  // Remove offsets and scale gyro data  
  float gyro_x = (gx - base_x_gyro)/GYRO_FACTOR;
  float gyro_y = (gy - base_y_gyro)/GYRO_FACTOR;
  //float gyro_z = (gz - base_z_gyro)/GYRO_FACTOR;
  float accel_x = ax; // - base_x_accel;
  float accel_y = ay; // - base_y_accel;
  float accel_z = az; // - base_z_accel;

  float accel_angle_y = atan(-1*accel_x/sqrt(pow(accel_y,2) + pow(accel_z,2)))*RADIANS_TO_DEGREES;
  float accel_angle_x = atan(accel_y/sqrt(pow(accel_x,2) + pow(accel_z,2)))*RADIANS_TO_DEGREES;
  //float accel_angle_z = 0;
  yield();
  // Compute the (filtered) gyro angles
  float dt =(t_now - last_read_time)/1000.0;
  float gyro_angle_x = gyro_x*dt + last_x_angle;
  float gyro_angle_y = gyro_y*dt + last_y_angle;
  //float gyro_angle_z = gyro_z*dt + last_z_angle;

  // Compute the drifting gyro angles
  float unfiltered_gyro_angle_x = gyro_x*dt + last_gyro_x_angle;
  float unfiltered_gyro_angle_y = gyro_y*dt + last_gyro_y_angle;
  //float unfiltered_gyro_angle_z = gyro_z*dt + last_gyro_z_angle;   

  // Apply the complementary filter to figure out the change in angle - choice of alpha is
  // estimated now.  Alpha depends on the sampling rate...
  //const float alpha = 0.96;
  float angle_x = 0.98*gyro_angle_x + 0.02*accel_angle_x;
  float angle_y = 0.98*gyro_angle_y + 0.02*accel_angle_y;
  //float angle_z = gyro_angle_z;  //Accelerometer doesn't give z-angle

  // Update the saved data with the latest values
  set_last_read_angle_data(t_now, angle_x, angle_y, 0, unfiltered_gyro_angle_x, unfiltered_gyro_angle_y, 0); 
  yield();
}

void mpu6050Init()
{
  byte error;

  Wire.beginTransmission(0x68);   // MPU6050 installed?
  error = Wire.endTransmission();
  if (error) {
    Serial.println ("MPU6050 not found");
  }
  else {
    Serial.println ("Initializing MPU6050...");
    setup_mpu_6050_registers();  
    Serial.print("Calibrating gyro");  

    calibrate_sensors();
    mputread = millis();
  }
}

void mpuMeasurement(){
  roll = sin((last_x_angle - roll_offset) * DEGREES_TO_RADIANS)*100;
  pitch = sin((last_y_angle - pitch_offset) * DEGREES_TO_RADIANS)*100;
  temperature = (rawTemp/340.)+36.53;
}

float ReadBatteryVoltage()
{
  /*
  The D32 has a on-board voltage divider see it here in the battery section of the schematic: https://wiki.wemos.cc/_media/products:d32:sch_d32_v1.0.0.pdf
  Assuming a bare board ESp32 the ADC input range is 0-3.3v for a reading of 0-4095, then the reading range would be 3.3/4095*ADC Reading, so when the ADC 
  reading was 4095 the voltage value would be 3.3v.
  Now let's add in the on-board voltage divider of two 100K series resistors with the ADC at their junction, but first let's assume the ESP32 has an infinitely
  high input impedance and does not load the voltage divider, then the voltage range would be Vadc = 6.6/4095*ADC Reading, now for an ADC reading of 4095 
  the voltage value is 6.6vBut in practice the ESP32 ADC input impedance is ~390K, so now the voltage divider is 100K in series with  390K in parallel with 100K, 
  this has the effect of increasing the voltage division and so the ratio has to be increased to compensate.So if the series R was R1 and the R to ground was 
  R2 and the ADC input impedance was Radc, then:
  
  Ratio = 3.3*(R1+(R2*Radc/(R2+390)))/((R2*Radc)/(R2+Radc))
  Example:Ratio = 7.445 =3.3*(100+(100*390/(100+390)))/((100*390)/(100+390))
  */

  return analogRead(35) / 4096.0 * 7.445; //From example
//   return analogRead(35) / 4096.0 * 8.348717949; //self calculated constant
}

double round2(double value) {
  return (int)(value * 100 + 0.5) / 100.0;
}