//motor pin defines
#define rmf 6
#define rmb 7
#define lmf 8
#define lmb 9
#define rms 10
#define lms 5

//declaration of necessary variable to line follow
int s[6], total, sensor_position;
int threshold = 531; //set threshold as like i've shown in video
float avg;
int position[6] = { 1, 2, 3, 4, 5 };
int set_point = 3;



int kp = 0.007, kd = 0.5, PID_Value, P, D;
float error, previous_error;
int r_base_speed = 65, left_motor_speed, right_motor_speed, turn_speed = 70;
int l_base_speed = 75;
char t;

void setup() {
  //motor driver pins as output
  pinMode(lmf, OUTPUT);
  pinMode(lmb, OUTPUT);
  pinMode(rmf, OUTPUT);
  pinMode(rmb, OUTPUT);
  //button pin as input

  Serial.begin(9600);
}

void loop() {

  //display_value();

  PID_LINE_FOLLOW();  //line follow using PID_Value

   //motor(l_base_speed,r_base_speed);
  
}

void Sensor_reading() {
  sensor_position = 0;
  total = 0;
  for (byte i = 0; i < 5; i++) {  //snesor data read from A0, A1, A2, A6, A7
    if (i < 5) {
      s[i] = analogRead(i);
    }
    if (s[i] > threshold) s[i] = 0;  //analog value to digital conversion
    else s[i] = 1;
    sensor_position += s[i] * position[i];
    total += s[i];
  }
  if (total) avg = sensor_position / total;  //average value
}

void PID_LINE_FOLLOW() {

  while (1) {
    Sensor_reading();
    if (total == 0) { 
      if (t != 's') {
        if (t == 'r') motor(turn_speed, -turn_speed); 
        else motor(-turn_speed, turn_speed);          
        while (!s[2]) Sensor_reading();
      }
    }

    Sensor_reading();
    error = set_point - avg;
    D = kd * (error - previous_error);
    P = error * kp;
    PID_Value = P + D;
    previous_error = error;

    //adjusting motor speed to keep the robot in line
    right_motor_speed = r_base_speed - PID_Value;  //right motor speed
    left_motor_speed = l_base_speed + PID_Value;  //left motor speed
    motor(left_motor_speed, right_motor_speed);               //when robot in straight position

    

    if (s[4] == 0 && s[0] == 1) t = 'l';
    if (s[4] == 1 && s[0] == 0) t = 'r';
    

    else if (total == 5) { 
      Sensor_reading();
      if (total == 5) {
        motor(0, 0);
        while (total == 5) Sensor_reading();
      } else if (total == 0) t = 's';
    }
  }
}

void display_value() {  //display the analog value of sensor in serial monitor
  for (byte i = 0; i < 5; i++) {
    if (i < 5) {
      s[i] = analogRead(i);
      if (s[i] > threshold) s[i] = 0;  //analog value to digital conversion
      else s[i] = 1;
    }
    Serial.print(String(s[i]) + " ");
  }
  Serial.println();
  delay(50);
}

//motor run function
void motor(int a, int b) {
  a = constrain(a, -255, 255);
  b = constrain(b, -255, 255);

  digitalWrite(lmf, a > 0);
  digitalWrite(lmb, a < 0);
  digitalWrite(rmf, b > 0);
  digitalWrite(rmb, b < 0);

  analogWrite(lms, abs(a));
  analogWrite(rms, abs(b));
}