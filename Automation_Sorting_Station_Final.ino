// Simple software SPI test for ebay 128x64 oled.

#include "SSD1306Ascii.h"
#include "SSD1306AsciiSoftSpi.h"

#include <Servo.h>

#include <SoftwareSerial.h>
SoftwareSerial Bluetooth(44, 45); // RX, TX  connect Bluetooth RX to 45, TX to 44

//defining ultrasonc sensor pins
#define echoPin_proximity 22
#define trigPin_proximity 23
#define echoPin_height 26
#define trigPin_height 27

// pin definitions OLED
#define CS_PIN    39
#define RST_PIN   40
#define DC_PIN    38
#define MOSI_PIN 37
#define CLK_PIN  36

SSD1306AsciiSoftSpi oled;

//for Line Following Robot using 2IR sensors
int lm1 = 29; //left motor output 1
int lm2 = 28; //left motor output 2
int rm1 = 30; //right motor output 1
int rm2 = 31; //right motor output 2
int sprm = 3; //motor speed
int splm = 2;
int defspeed = 120; //default speed
int mspeed = defspeed;
int conveyor_motor_delay = 12;
int conveyor_motor_stop_delay = 10;

// defines variables for ultrasonic sensor
long duration_proximity; // variable for the duration of sound wave travel
float distance_proximity; // variable for the distance measurement
long duration_height;
float distance_height;

//defining thresholds for ultrasonic
float object_on_conveyor = 42;
float object_detected = 4.5;
float small = 2.85;
float medium = 5.5;
float large = 7.5;
float base_height = 11.2;
float object_height = 0;

//define servo motors
Servo servo_base;
Servo servo_dumper;
Servo robo_arm_base;
Servo robo_arm_link;
Servo robo_arm_gripper;
int pos = 0;
int servo_speed_delay = 5; //control speed of sorting servo
int arm_speed_delay = 8; //control speed of arm


//inventory of detected objects
int num_small = 0;
int num_medium = 0;
int num_large = 0;

int truck_capacity = 3;
int truck_busy = 0;
char truck_task_status = 'x'; //set to z when line follower completes action

void setup()
{
  pinMode(trigPin_proximity, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin_proximity, INPUT); // Sets the echoPin as an INPUT
  pinMode(trigPin_height, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin_height, INPUT); // Sets the echoPin as an INPUT

  pinMode(lm1, OUTPUT); //conveyor motor pins
  pinMode(lm2, OUTPUT);
  pinMode(rm1, OUTPUT);
  pinMode(rm2, OUTPUT);
  pinMode(splm, OUTPUT);
  pinMode(sprm, OUTPUT);

  servo_base.attach(4);
  servo_dumper.attach(5);
  robo_arm_base.attach(6);
  robo_arm_link.attach(7);
  robo_arm_gripper.attach(8);

  Serial.begin(9600); // Serial Communication is starting with 9600 of baudrate speed
  //Serial1.begin(9600); //initiate bluetooth
  Bluetooth.begin(9600);
  delay(200);
  conveyor_stop();
  delay(500);
  initiate_servo();
  delay(750);


  //Initiate OLED display
  // Use next line if no RST_PIN or reset is not required.
  // oled.begin(&Adafruit128x64, CS_PIN, DC_PIN, CLK_PIN, MOSI_PIN);
  oled.begin(&Adafruit128x64, CS_PIN, DC_PIN, CLK_PIN, MOSI_PIN, RST_PIN);
  oled.setFont(System5x7);
  uint32_t m = micros();
  oled_display();
  oled.set2X();
  oled.println("   Empty");

  delay(400);
}



void loop()
{

  //  if (Bluetooth.available())
  //    {
  //      truck_task_status = Bluetooth.read();
  //      Serial.print("truck_task_status is :  ");
  //      Serial.println(truck_task_status);
  //      if (truck_task_status == "z")
  //      {
  //        oled_display();
  //        oled.set2X();
  //        oled.println("   Empty");
  //        truck_busy = 0;
  //        truck_task_status = 'x';
  //      }
  //    }
  // if (truck_busy == 0)
  // {
  check_proximity();
  initiate_conveyor_speed();

  if (distance_proximity > object_detected)
  {
    conveyor_forward();
    delay(conveyor_motor_delay);
    conveyor_stop();
    delay(conveyor_motor_stop_delay);
  }

  if (distance_proximity <= object_detected)
  {
    conveyor_stop();
    check_height();
    delay(2000);

    if (object_height <= small)
    {
      num_small = num_small + 1;
      oled_display();
      oled.set2X();
      oled.println("   Small");
      servo_90();
      delay(1500);
      servo_dumping_action();
      delay(1500);
      oled_display();
      oled.set2X();
      oled.println("   Empty");
      servo_90_return();
      delay(1500);
      if (num_small > 0  &&  num_small % truck_capacity == 0)
      {
        Bluetooth.write("i");
        //truck_busy = 1;
        conveyor_stop();
        oled_display();
        oled.set2X();
        oled.println("   BUSY");
        delay(20000);//waiting fro line follower to finish
        oled_display();
        oled.set2X();
        oled.println("   Empty");
        conveyor_forward();
      }
    }
    if (object_height > medium)
    {
      num_large = num_large + 1;
      oled_display();
      oled.set2X();
      oled.println("   Large");
      delay(500);
      robotic_arm_action();
      oled_display();
      oled.set2X();
      oled.println("   Empty");
      delay(500);
      conveyor_forward();
    }
    if (object_height > small && object_height <= medium)
    {
      num_medium = num_medium + 1;
      oled_display();
      oled.set2X();
      oled.println("   Medium");
      servo_180();
      delay(2000);
      servo_dumping_action();
      delay(2000);
      oled_display();
      oled.set2X();
      oled.println("   Empty");
      servo_180_return();
      delay(2000);
      conveyor_forward();
    }
  }
}
//}

void check_height()
{
  digitalWrite(trigPin_height, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_height, LOW);
  duration_height = pulseIn(echoPin_height, HIGH);
  distance_height = duration_height * 0.034 / 2;
  object_height = base_height - distance_height;
  Serial.print("  |  Object_height: ");
  Serial.print(distance_height);
  Serial.print("     ");
  Serial.print(object_height);
  Serial.print(" cm   ");
}

void check_proximity()
{
  // Clears the trigPin condition
  digitalWrite(trigPin_proximity, LOW);
  digitalWrite(trigPin_height, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin_proximity, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_proximity, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration_proximity = pulseIn(echoPin_proximity, HIGH);
  // Calculating the distance
  distance_proximity = duration_proximity * 0.034 / 2;// Speed of sound wave divided by 2 (go and back)
  // Displays the distance on the Serial Monitor
  Serial.print("Distance_proximity: ");
  Serial.print(distance_proximity);
  Serial.print(" cm   ");
  Serial.println();
}

void initiate_conveyor_speed()
{
  // set speed
  analogWrite(splm, defspeed); //full speed at 255
  analogWrite(sprm, defspeed);
}

void conveyor_stop()
{
  digitalWrite(lm1, LOW);
  digitalWrite(lm2, LOW);
  digitalWrite(rm1, LOW);
  digitalWrite(rm2, LOW);
}
void conveyor_forward()
{
  digitalWrite(lm1, HIGH);
  digitalWrite(lm2, LOW);
  digitalWrite(rm1, HIGH);
  digitalWrite(rm2, LOW);
}
void conveyor_backward()
{
  digitalWrite(lm1, LOW);
  digitalWrite(lm2, HIGH);
  digitalWrite(rm1, LOW);
  digitalWrite(rm2, HIGH);
}
void initiate_servo()
{
  servo_base.write(0);
  servo_dumper.write(0);
  robo_arm_link.write(15);
  robo_arm_base.write(90);
  robo_arm_gripper.write(180); //gripper close
}
void servo_90()
{
  for (pos = 0; pos <= 90; pos += 1)
  {
    servo_base.write(pos);
    delay(servo_speed_delay);
  }
  pos = 0;
}
void servo_180()
{
  for (pos = 0; pos <= 180; pos += 1)
  {
    servo_base.write(pos);
    delay(servo_speed_delay);
  }
  pos = 0;
}
void servo_180_return()
{
  for (pos = 180; pos >= 0; pos -= 1)
  {
    servo_base.write(pos);
    delay(servo_speed_delay);
  }
  pos = 0;
}
void servo_90_return()
{
  for (pos = 90; pos >= 0; pos -= 1)
  {
    servo_base.write(pos);
    delay(servo_speed_delay);
  }
  pos = 0;
}
void servo_dumping_action()
{
  for (pos = 0; pos <= 27; pos += 1)
  {
    servo_dumper.write(pos);
    delay(5);
  }
  delay(1500);
  for (pos = 27; pos >= 0; pos -= 1)
  {
    servo_dumper.write(pos);
    delay(5);
  }
  pos = 0;
}
void robotic_arm_action()
{
  delay(400);
  robo_arm_gripper.write(140); //gripper open
  delay(500);
  for (pos = 15; pos <= 50; pos += 1)  //robo link move up
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  delay(500);
  for (pos = 90; pos <= 158; pos += 1) //robo base move half toward pick-up
  {
    robo_arm_base.write(pos);
    delay(arm_speed_delay);
  }
  delay(750);
  for (pos = 50; pos >= 15; pos -= 1) //robo link half down
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  delay(500);
  for (pos = 158; pos <= 180; pos += 1) //robo base reach pick-up
  {
    robo_arm_base.write(pos);
    delay(arm_speed_delay);
  }
  delay(750);
  for (pos = 15; pos >= 0; pos -= 1) //robo link reach object
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  delay(500);
  robo_arm_gripper.write(180); //gripper close
  delay(750);
  for (pos = 0; pos <= 15; pos += 1) //robo link move half up
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  delay(500);
  for (pos = 180; pos >= 158; pos -= 1) //robo base leave pick-up
  {
    robo_arm_base.write(pos);
    delay(arm_speed_delay);
  }
  delay(750);
  for (pos = 15; pos <= 50; pos += 1) //robo link move up
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  delay(500);
  for (pos = 158; pos >= 0; pos -= 1) //robo base reaches drop
  {
    robo_arm_base.write(pos);
    delay(arm_speed_delay);
  }
  delay(1500);
  for (pos = 50; pos >= 0; pos -= 1) //robo link move down
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  delay(500);
  robo_arm_gripper.write(140); //gripper open
  delay(750);
  for (pos = 0; pos <= 50; pos += 1) //robo link move up
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  delay(500);
  for (pos = 0; pos <= 90; pos += 1) //robo base reaches initial
  {
    robo_arm_base.write(pos);
    delay(arm_speed_delay);
  }
  delay(750);
  for (pos = 50; pos >= 15; pos -= 1) //robo link reaches initial
  {
    robo_arm_link.write(pos);
    delay(arm_speed_delay);
  }
  robo_arm_gripper.write(180); //gripper close
  delay(400);
  pos = 0;
}
void oled_display()
{
  oled.clear();
  oled.set1X();
  oled.println("   Object Count\n");
  oled.print("   Small: ");
  oled.println(num_small);
  oled.print("   Medium:  ");
  oled.println(num_medium);
  oled.print("   Large:  ");
  oled.println(num_large);
  oled.print("   Height:  ");
  oled.print(object_height);
  oled.println(" cm");
}
