#include <Arduino.h>
#include <Wire.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// ================= MPU =================
#define MPU 0x68
int16_t ax, ay, az;
int16_t gx, gy, gz;

float yaw = 0;
float gyroZ_offset = 0;
unsigned long prevTime;

// ================= TACHO =================
#define S1 16
#define S2 17
#define S3 18
#define S4 19

const int HOLES = 20;

volatile unsigned long count[4] = {0,0,0,0};
volatile unsigned long lastPulse[4] = {0,0,0,0};

// ================= MOTOR =================
const int FL_IN1=14, FL_IN2=12, FL_PWM=13;
const int RL_IN1=4,  RL_IN2=2,  RL_PWM=15;
const int FR_IN1=26, FR_IN2=27, FR_PWM=25;
const int RR_IN1=33, RR_IN2=32, RR_PWM=5;

const int BASE_SPEED = 100;

// ================= SELF-CORRECTING SPEED =================
int left_offset = 0;
int right_offset = 0;

const float CORRECTION_GAIN = 1.5;

// ================= CHILD: IMU VELOCITY (independent) =================
float child_ax_offset = 0;
float child_imu_velocity = 0;
float child_imu_distance = 0;
float child_SCALE = 1.0;
bool  child_scale_calibrated = false;
int   child_run_number = 0;
unsigned long child_prevTime = 0;
const float CHILD_WHEEL_DIAMETER = 0.065;  // 6.5 cm

// ================= ISR =================
void IRAM_ATTR isr1(){ if(micros()-lastPulse[0]>3000){count[0]++; lastPulse[0]=micros();}}
void IRAM_ATTR isr2(){ if(micros()-lastPulse[1]>3000){count[1]++; lastPulse[1]=micros();}}
void IRAM_ATTR isr3(){ if(micros()-lastPulse[2]>3000){count[2]++; lastPulse[2]=micros();}}
void IRAM_ATTR isr4(){ if(micros()-lastPulse[3]>3000){count[3]++; lastPulse[3]=micros();}}

// ================= MPU =================
void readMPU(){
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,14,true);

  ax=(Wire.read()<<8)|Wire.read();
  ay=(Wire.read()<<8)|Wire.read();
  az=(Wire.read()<<8)|Wire.read();

  Wire.read(); Wire.read();

  gx=(Wire.read()<<8)|Wire.read();
  gy=(Wire.read()<<8)|Wire.read();
  gz=(Wire.read()<<8)|Wire.read();
}

// ================= MOTOR =================
void setForward(){
  digitalWrite(FL_IN1,1); digitalWrite(FL_IN2,0);
  digitalWrite(RL_IN1,1); digitalWrite(RL_IN2,0);
  digitalWrite(FR_IN1,1); digitalWrite(FR_IN2,0);
  digitalWrite(RR_IN1,1); digitalWrite(RR_IN2,0);
}

void driveWithOffsets(){
  int leftSpd  = constrain(BASE_SPEED + left_offset, 0, 255);
  int rightSpd = constrain(BASE_SPEED + right_offset, 0, 255);
  // Only front wheels get correction; rear wheels stay at constant speed
  ledcWrite(FL_PWM, leftSpd);
  ledcWrite(RL_PWM, BASE_SPEED);
  ledcWrite(FR_PWM, rightSpd);
  ledcWrite(RR_PWM, BASE_SPEED);
}

void stopMotor(){
  ledcWrite(FL_PWM, 0);
  ledcWrite(RL_PWM, 0);
  ledcWrite(FR_PWM, 0);
  ledcWrite(RR_PWM, 0);
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);
  SerialBT.begin("ESP32_CAR");

  Wire.begin(21,22);

  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  pinMode(S1,INPUT_PULLUP);
  pinMode(S2,INPUT_PULLUP);
  pinMode(S3,INPUT_PULLUP);
  pinMode(S4,INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(S1),isr1,CHANGE);
  attachInterrupt(digitalPinToInterrupt(S2),isr2,CHANGE);
  attachInterrupt(digitalPinToInterrupt(S3),isr3,CHANGE);
  attachInterrupt(digitalPinToInterrupt(S4),isr4,CHANGE);

  int pins[]={FL_IN1,FL_IN2,RL_IN1,RL_IN2,FR_IN1,FR_IN2,RR_IN1,RR_IN2};
  for(int p:pins) pinMode(p,OUTPUT);

  ledcAttach(FL_PWM,5000,8);
  ledcAttach(RL_PWM,5000,8);
  ledcAttach(FR_PWM,5000,8);
  ledcAttach(RR_PWM,5000,8);

  SerialBT.println("Send GO");
}

// ================= LOOP =================
void loop(){

  if(SerialBT.available()){
    String cmd=SerialBT.readStringUntil('\n');
    cmd.trim();

    if(cmd=="GO"){

      // CHILD: track run number
      child_run_number++;

      SerialBT.println("Keep still...");
      delay(3000);

      // PARENT: gyro calibration
      gyroZ_offset = 0;
      for(int i=0;i<200;i++){
        readMPU();
        gyroZ_offset += gz;
        delay(5);
      }
      gyroZ_offset /= 200;

      // CHILD: accel calibration (separate pass, does not touch parent)
      child_ax_offset = 0;
      for(int i=0;i<500;i++){
        readMPU();
        child_ax_offset += ax;
        delay(2);
      }
      child_ax_offset /= 500;

      // PARENT: reset yaw and time
      yaw = 0;
      prevTime = millis();

      // CHILD: reset velocity and distance
      child_imu_velocity = 0;
      child_imu_distance = 0;
      child_prevTime = millis();

      // PARENT: reset counts
      for(int i=0;i<4;i++) count[i]=0;

      // PARENT: show PWM
      int leftSpd  = constrain(BASE_SPEED + left_offset, 0, 255);
      int rightSpd = constrain(BASE_SPEED + right_offset, 0, 255);
      SerialBT.print("Left PWM: "); SerialBT.print(leftSpd);
      SerialBT.print(" | Right PWM: "); SerialBT.println(rightSpd);

      // PARENT: start motors
      setForward();
      driveWithOffsets();

      SerialBT.println("Running...");

      unsigned long start = millis();

      while(millis() - start < 5000){

        readMPU();

        // PARENT: yaw integration (unchanged)
        float gyroZ=(gz-gyroZ_offset)/131.0;

        unsigned long now=millis();
        float dt=(now-prevTime)/1000.0;
        prevTime=now;

        yaw += gyroZ * dt;

        // CHILD: IMU velocity integration (reads ax, writes only child_ vars)
        float child_dt = (now - child_prevTime) / 1000.0;
        child_prevTime = now;

        float accel = (ax - child_ax_offset) / 16384.0 * 9.81;
        if(fabs(accel) < 0.05) accel = 0;
        child_imu_velocity += accel * child_dt;
        child_imu_velocity *= 0.995;
        if(child_imu_velocity < 0) child_imu_velocity = 0;

        // CHILD: accumulate IMU distance
        child_imu_distance += child_imu_velocity * child_dt;

        // PARENT: original delay
        delay(20);
      }

      // PARENT: stop motors
      stopMotor();

      // PARENT: RPM calc (unchanged)
      float timeSec = 5.0;

      float rpm1 = (count[0]/(float)HOLES)/timeSec*60;
      float rpm2 = (count[1]/(float)HOLES)/timeSec*60;
      float rpm3 = (count[2]/(float)HOLES)/timeSec*60;
      float rpm4 = (count[3]/(float)HOLES)/timeSec*60;

      // PARENT: yaw correction (unchanged)
      int correction = (int)(CORRECTION_GAIN * yaw);
      left_offset  += correction;
      right_offset -= correction;

      // ============ OUTPUT ============
      SerialBT.println("===================================");
      SerialBT.print("Run #"); SerialBT.println(child_run_number);
      SerialBT.println("-----------------------------------");

      // Yaw & correction
      SerialBT.print("Yaw:        "); SerialBT.print(yaw, 3); SerialBT.println(" deg");
      SerialBT.print("Correction: L="); SerialBT.print(left_offset);
      SerialBT.print("  R="); SerialBT.println(right_offset);
      SerialBT.println("-----------------------------------");

      // RPMs
      SerialBT.println("RPM:");
      SerialBT.print("  FR: "); SerialBT.print(rpm1, 1);
      SerialBT.print("   RL: "); SerialBT.println(rpm3, 1);
      SerialBT.print("  FL: "); SerialBT.print(rpm2, 1);
      SerialBT.print("   RR: "); SerialBT.println(rpm4, 1);
      SerialBT.println("-----------------------------------");

      // Tyre radius from RPM ratios (FR = 3.25 cm reference)
      const float FR_RADIUS = 3.25;  // cm
      float r_fl = (rpm3 > 0 && rpm1 > 0) ? (rpm3 / rpm1) * FR_RADIUS : 0;
      float r_rl = (rpm3 > 0 && rpm2 > 0) ? (rpm3 / rpm2) * FR_RADIUS : 0;
      float r_rr = (rpm3 > 0 && rpm4 > 0) ? (rpm3 / rpm4) * FR_RADIUS : 0;

      SerialBT.println("Tyre Radius (cm):");
      SerialBT.print("  FR: "); SerialBT.print(r_fl, 2);
      SerialBT.print("   RL: "); SerialBT.println(FR_RADIUS, 2);
      SerialBT.print("  FL: "); SerialBT.print(r_rl, 2);
      SerialBT.print("   RR: "); SerialBT.println(r_rr, 2);
      SerialBT.println("-----------------------------------");

      // IMU calibration on 1st GO
      if(child_run_number == 1){
        float fr_revs = count[2] / (float)HOLES;
        float tacho_distance = fr_revs * PI * CHILD_WHEEL_DIAMETER;

        if(child_imu_distance > 0.01 && tacho_distance > 0.01){
          child_SCALE = tacho_distance / child_imu_distance;
          child_scale_calibrated = true;
         
        } else {
          SerialBT.println("Calibration failed. Send GO again.");
        }
      }

      // 2nd GO+ — IMU-only speed
      if(child_scale_calibrated && child_run_number >= 2){
        float final_v = child_imu_velocity * child_SCALE;
      
      }

      SerialBT.println("===================================");
      SerialBT.println("Send GO");
    }
  }
}