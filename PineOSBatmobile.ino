/*PineOS-Batmobile
Written by Scott Pack (@scottjpack)
Available at https://www.github.com/scottjpack

This is a vast improvement on the previous PineOS version

Circuit Components:
Arduino Nano V3 (I used the sainsmart)
2x red LED (tail-lights)
2x white LED (head-lights)
~25A Electronic Speed Controller
Inrunner brushless motor
    Example: Turnigy Motor Brushless Inrunner 2430 5800kv
2x 45 degree bevel gears
1x QRD1114 Phototransistor
1x 10k resistor
1x 2k resistor
1x tactile button

Attach to pins as shown in pin declarations.
Feel free to reach out if you have questions!


*/

#include <Servo.h>

//ESC Range
#define MAX_SIGNAL 2000
#define MIN_SIGNAL 700

#define START_TRANS 3

//Pin References
#define HEADLIGHT 2
#define MOTOR 3
#define TAILLIGHT 4
#define ARM_BTN A5
#define IR_BOUNCE A6

//State Declarations
#define FRESH 1
#define DISARMED 2
#define ARMING 3
#define ARMED 4
#define RUNNING 5

Servo motor;
int state;
int rot_count = 0;
bool wheel_point = false;

void set_motor_speed(int duty_cycle)
{
 //Takes 0-100.
 if (duty_cycle > 100){
  duty_cycle = 100;
 } 
 if (duty_cycle < 0){
  duty_cycle = 0; 
 }
  int delta = ((MAX_SIGNAL-MIN_SIGNAL)*duty_cycle/100);
  Serial.print("Starting motor at ");
  Serial.print(duty_cycle);
  Serial.println("% duty cycle.");
  motor.writeMicroseconds(MIN_SIGNAL + delta);
}


void taillights(boolean state) {
 digitalWrite(TAILLIGHT, state);
}

void headlights(boolean state) {
 digitalWrite(HEADLIGHT, state);
}

void double_flash(){
 // Flashing is used to show when the car is arming
 // and ready to be placed on the track.
 headlights(false);
 taillights(false);
 delay(250);
 headlights(true);
 taillights(true);
 delay(250);
 headlights(false);
 taillights(false);
 delay(250);
 headlights(true);
 taillights(true);
 delay(250);
 headlights(false);
 taillights(false);
}

void setup() {
  //Set initial state and set up serial for debugging
  state = FRESH;
  Serial.begin(9600);

  //Set up pin modes.
  pinMode(HEADLIGHT,OUTPUT);
  pinMode(TAILLIGHT,OUTPUT);
  pinMode(MOTOR,OUTPUT);
  pinMode(ARM_BTN, INPUT_PULLUP);
  pinMode(IR_BOUNCE, INPUT);

  //Calibrate the ESC
  motor.attach(MOTOR);
  motor.writeMicroseconds(MAX_SIGNAL);
  delay(2000);
  motor.writeMicroseconds(MIN_SIGNAL);
  delay(2000);

}

void lights_flutter_on() {
 headlights(true);
 delay(50);
 headlights(false);
 delay(50);
 headlights(true);
 delay(50);
 headlights(false);
 delay(100);
 headlights(true);
 delay(50);
 headlights(false);
 delay(200);
 headlights(true); 
}

void loop() {
  //Check the state and act on appropriate inputs.
  if (state == FRESH){
     //This is only run on first power-on
     lights_flutter_on();
     Serial.println("PineOS MK2: Chicks love the car...");
     Serial.println("Written by Scott Pack");
     state = DISARMED;
  }
  else if (state == DISARMED){
    //Read and debounce the switch
    //If button is down, go into ARMING
    int arm_val = analogRead(ARM_BTN);
    if (arm_val < 50){
     Serial.println("Moving from DISARMED to ARMING");
     state = ARMING;
     double_flash();
     double_flash();
     double_flash();
    }
  }
  else if (state == ARMING){
    //just sit here until the button is released, then wait a few seconds. 
    int arm_val = analogRead(ARM_BTN);
    if (arm_val < 50){
     state = ARMING;
    }
    else{
      Serial.println("Moving from ARMING to ARMED");
      state = ARMED;
      rot_count = 0;
      headlights(true);
    }
  }
  else if (state == ARMED){
    //When tail-lights are on, it means it's armed and ready to go.
    taillights(true);
    
    int ir_val = analogRead(IR_BOUNCE);
    bool new_wheel_point = false;
    bool valid = true;
    
    //My phototransistor (QRD1114) was getting intermittent readings at 0, so I limited bottom range at 800.
    //First time using a phototransistor, so I'm not sure how common this is.
    if (ir_val <= 1005 && ir_val > 800) 
    {
      new_wheel_point = false;
    }
    else if (ir_val > 1005) 
    {
      new_wheel_point = true;
    }
    else{
     //bad reading, disregard 
     valid = false;
    }

    //If there was a wheel transition, increase the rotation figure and change the wheel point
    if ((new_wheel_point^wheel_point) && valid){
      rot_count++;
      Serial.print("ARMED ROTATION COUNT: ");
      Serial.println(rot_count);
      wheel_point = new_wheel_point;
    }
    
    
    //Once getting to the appropriate number of transitions, move to RUNNING 
    //Recommended is to modify your wheel so half is blocked and half is open.  This will result in the clearest transitions.
    if (rot_count > START_TRANS){
      Serial.println("Moving from ARMED to RUNNING");
      state = RUNNING;
    }
  }
  else if (state == RUNNING){
      //Turn off those brake-lights, we're driving!
      taillights(false);
      
      Serial.println("RUNNING!");   
      
      //Default here is long and slow, good for testing.
      //Depending on how aggressive you want your car, increase the duty cycle and decrease the delay.
      //No mechanism to detect when the car has stopped, so it's all down to timing.
      set_motor_speed(10);
      delay(2000);
      state = DISARMED;
      set_motor_speed(0); 
      Serial.println("Moving from RUNNING to DISARMED");
  }
 
}
