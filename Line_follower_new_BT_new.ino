#include <SoftwareSerial.h>
//for Line Following Robot using 2IR sensors
int lm1 = 7; //left motor output 1
int lm2 = 6; //left motor output 2
int rm1 = 8; //right motor output 1
int rm2 = 9; //right motor output 2
int sl = 13;  //sensor 1 input (left)
int sr = 12;  //sensor 2 input (right)
int smid = 2;  //sensor middle input (middle)
int SlV = 0;
int SrV = 0;
int SmidV = 0;
int led = A0;
int sprm = 10; //motor speed
int splm = 5;
int bt = 0; //trigger to initiate line follower
int defspeed = 150; //default speed
int mspeed = defspeed;
int line_follower_delay = 10;
int line_follower_stop_delay = 5;
SoftwareSerial Bluetooth(4, 3); // RX, TX  connect Bluetooth RX to 3, TX to 4

//send 'z' when action complete
void setup()
{
  pinMode(lm1, OUTPUT);
  pinMode(lm2, OUTPUT);
  pinMode(rm1, OUTPUT);
  pinMode(rm2, OUTPUT);
  pinMode(splm, OUTPUT);
  pinMode(sprm, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(sl, INPUT);
  pinMode(sr, INPUT);
  pinMode(smid, INPUT);
  Serial.begin(9600);
  Bluetooth.begin(9600);
  sTOP();
  read();
}
void loop()
{
  // set speed
  analogWrite(splm, defspeed); //full speed at 255
  analogWrite(sprm, defspeed);

  char c;

  if (Bluetooth.available())
  {
    c = Bluetooth.read();
    switch (c)
    {
      case 'w':   //move forward
        ForWard();
        break;

      case 's': //move Backward
        BackWard();
        break;

      case 'a': //move left
        slowLeft();
        break;

      case 'd': //move right
        slowRight();
        break;

      case 'q': //move left sharp
        Left();
        break;

      case 'e': //move right sharp
        Right();
        break;

      case 'o': //STOP
        sTOP();
        bt = 0;
        break;

      case 'n': //reduce speed
        if (mspeed > 5)
        {
          mspeed = mspeed - 5;
        }
        break;

      case 'm': //increase speed
        if (mspeed < 220)
        {
          mspeed = mspeed + 5;
        }
        break;

      case 'l': //reset speed to default 100
        mspeed = defspeed;
        break;

      case 'i': //IR Initiate
        bt = 1;
        break;
    }
  }
  if (bt == 1)
  {
    delay(line_follower_stop_delay);
    read();
    if (SrV == 0 && SlV == 0)
    {
      line_follower_forward();
    }
    if (SrV == 1 && SlV == 1)
    {
      if (SmidV == 0)
      {
        line_follower_halt();
      }
      if (SmidV == 1)
      {
        line_follower_stop_reset();
      }
    }
    if (SrV == 1 && SlV == 0)
    {
      line_follower_right();
    }
    if (SrV == 0 && SlV == 1)
    {
      line_follower_left();
    }
    if (SrV == 0 && SlV == 0 && SmidV == 0)
    {
      sTOP();
    }
  }
}
void ForWard()
{
  digitalWrite(lm1, HIGH);
  digitalWrite(lm2, LOW);
  digitalWrite(rm1, HIGH);
  digitalWrite(rm2, LOW);
}
void BackWard()
{
  digitalWrite(lm1, LOW);
  digitalWrite(lm2, HIGH);
  digitalWrite(rm1, LOW);
  digitalWrite(rm2, HIGH);
}
void Left()
{
  digitalWrite(lm1, LOW);
  digitalWrite(lm2, HIGH);
  digitalWrite(rm1, HIGH);
  digitalWrite(rm2, LOW);
}
void Right()
{
  digitalWrite(lm1, HIGH);
  digitalWrite(lm2, LOW);
  digitalWrite(rm1, LOW);
  digitalWrite(rm2, HIGH);
}
void slowLeft()
{
  digitalWrite(lm1, LOW);
  digitalWrite(lm2, LOW);
  digitalWrite(rm1, HIGH);
  digitalWrite(rm2, LOW);
}
void slowRight()
{
  digitalWrite(lm1, HIGH);
  digitalWrite(lm2, LOW);
  digitalWrite(rm1, LOW);
  digitalWrite(rm2, LOW);
}
void sTOP()
{
  digitalWrite(lm1, LOW);
  digitalWrite(lm2, LOW);
  digitalWrite(rm1, LOW);
  digitalWrite(rm2, LOW);
}
void read()
{
  SlV = digitalRead(sl);
  SrV = digitalRead(sr);
  SmidV = digitalRead(smid);
}
void line_follower_forward()
{
  ForWard();
  delay(line_follower_delay);
  sTOP();
}
void line_follower_right()
{
  Right();
  delay(line_follower_delay);
  sTOP();
}
void line_follower_left()
{
  Left();
  delay(line_follower_delay);
  sTOP();
}
void line_follower_halt()
{
  sTOP();
  delay(5000);
  ForWard();
  delay(100);
  sTOP();
}
void line_follower_stop_reset()
{
  ForWard();
  delay(100);
  sTOP();
  bt = 0;
}
