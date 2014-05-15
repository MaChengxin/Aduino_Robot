//#include <LiquidCrystal.h>
#define DEBUG

//IMPORTANT!!!!!!!!!!!!!!!!!!!!
//Dont't forget to Modify "delay(theta)"  in Turn LEFT and RIGHT function  and the data in "avoidence"
//#include <Wire.h>
//#include <HMC5883L.h>
#include "Thread.h"
#include "ThreadController.h"
#include "TimerOne.h"
//#include <wiring_analog.c>
//#include "DueTimer.h"
//#include <TimerOne.cpp>
//HMC5883L compass;


////Motor Pin Define
#define motorR_A  36
#define motorR_B  34
#define motorL_A  32
#define motorL_B  30

////Constants
#define sonic_timeout 4400
#define pi 3.14159
#define threadholdDist_R  40
#define threadholdDist_L  40
#define threadholdDist_M  40
#define threadholdDist_passWidth  20
#define threadholdDist_Microphone  1000

//Error Tolerance
#define TOLERANCE_Course 2  //(degrees)

//Flag 
bool FLAG_AVOIDENCE_ON = 0;
bool FLAG_L_ON =0 ;
bool FLAG_M_ON =0 ;
bool FLAG_R_ON =0 ;
bool FLAG_calWidth = 0;
bool FLAG_SAVE_WIDTH = 0;
bool FLAG_GET_COURSE = 0;
bool FLAG_PASS_FINISH_LINE=0;

//Start Course
int startCousre = 0;


//LiquidCrystal lcd(52,50,38,36,34,32);
//Pin Define
const int trig1 = 50;
const int echo1= 52;
const int trig2 = 46;
const int echo2= 48;
const int trig3 = 44;
const int echo3= 42;
#define FRpin A8
#define microphonePin A8
int inter_t = 1000;
int time = 0;
float temp = 25;
float sonic_v = 1/((331.5+0.6*temp)*100/1000000);


//The thread to excute with specific time
class SensorThread: public Thread
{
public:
	float distance1,distance2,distance3;
        float width;
        float course;
        
        //Controller will will manage "run()" function 
	void run(){
          #ifdef DEBUG
              Serial.println("run");
             #endif
	  distance1 = getDistance(1);
          distance2 = getDistance(2);
          distance3 = getDistance(3);
          
          //Get the course when needed
          if(FLAG_GET_COURSE){
            course = getCourse();
            #ifdef DEBUG
              Serial.print("COURSE: ");
              Serial.println(course);
             #endif
          }
          
          //Calculate the width when needed
          if(FLAG_calWidth){
              width = calEntranceWidth(distance1,distance3);
              FLAG_SAVE_WIDTH = width>threadholdDist_passWidth;
          }
          // Serial.println("run2");
          
          //Turn on flag when the value is smaller than threshold
          FLAG_R_ON = distance1<threadholdDist_R;
          FLAG_M_ON = distance2<threadholdDist_M;
          FLAG_L_ON = distance3<threadholdDist_L;
          FLAG_AVOIDENCE_ON = FLAG_R_ON|FLAG_M_ON|FLAG_R_ON;
             #ifdef DEBUG
              Serial.println(distance1);
              Serial.println(distance2);
              Serial.println(distance3);
             #endif
	 runned();
	}
};

//Create instance of Class to get sensor data
SensorThread sensorReader = SensorThread();
Thread microphoneThread = Thread();

//Create controller to manage thread
ThreadController controller = ThreadController();


//The funtion that will be called when interrupt happened
void timerCallback(){
    #ifdef DEBUG
              Serial.println("timerCallback ");
    #endif
  //microphoneThread.run();
  //sensorReader.run();

  //Run the controller to manage the thread
  controller.run();
}

float getCourse(){
  //return getHeading();
}






//Avoidence MODE////////////////////////////////
  ////                  Solution 
//// bit  L M R     
////1     0 0 1      Turn Left
////2     0 1 0      Turn Left (By default)
////3     0 1 1      Turn Left
////4     1 0 0      Turn Right
////5     1 0 1      Turn Left (By default)
////6     1 1 0      Turn Right
////7     1 1 1      Turn Left (By default)
/////////////////////////////////////////////////

//Motor A HIGH --> Forward
void avoidence(){
    uint8_t situation = FLAG_R_ON+(FLAG_M_ON<<1)+(FLAG_L_ON<<2);
    uint8_t thetaRate = 100;
    uint8_t stepCount = 5;
    uint8_t stepDist = 50;
    //0 is Left
    bool lastReaction = 0;//Turn Left by Default
    //Keep turning until the ditstance against obstacle is far enough for left and right
    while (FLAG_AVOIDENCE_ON)
    {
        switch(situation){
        case(1):
            turnLeft(thetaRate);//Should be modify to theta rather than time to delay
            lastReaction = 0;
            break;
         case(2):
         //Matain the same turning direction
           if(lastReaction=0)
             turnLeft(thetaRate);
           else
             turnRight(thetaRate);//Should be modify to theta rather than time to delay
            break;
         case(3):
            turnLeft(thetaRate);//Should be modify to theta rather than time to delay
            lastReaction = 0;
            break;
         case(4):
            turnRight(thetaRate);//Should be modify to theta rather than time to delay
            lastReaction = 1;
            break;
         case(5):
           if(lastReaction=0)
             turnLeft(thetaRate);
           else
             turnRight(thetaRate);//Should be modify to theta rather than time to delay
            break;
         case(6):
            turnRight(thetaRate);//Should be modify to theta rather than time to delay
            lastReaction = 1;
            break;
         case(7):
             if(lastReaction=0)
             turnLeft(thetaRate);
           else
             turnRight(thetaRate);//Should be modify to theta rather than time to delay
            break;
            
        }
 
    //Go straight and (turn on witdth calculation)
    //FLAG_calWidth = 1;
    
    //If there is no danger, go straight for a few step befor fix direction
      for (uint8_t i =0;(i<stepCount)&&(!FLAG_AVOIDENCE_ON);i++){
        goStraight(stepDist);
      }
    
    //Fix the Direction
       if (!fixDirection())
           return;
    }
    

}
void goStraight(uint32_t duration){
       digitalWrite(motorR_A,HIGH);
       digitalWrite(motorR_B,LOW);
       digitalWrite(motorL_A,HIGH);
       digitalWrite(motorL_B,LOW);
       delay (duration);
       digitalWrite(motorR_A,LOW);
       digitalWrite(motorR_B,LOW);
       digitalWrite(motorL_A,LOW);
       digitalWrite(motorL_B,LOW);
}
void turnLeft(uint32_t theta){
       digitalWrite(motorR_A,HIGH);
       digitalWrite(motorR_B,LOW);
       digitalWrite(motorL_A,LOW);
       digitalWrite(motorL_B,HIGH);
       delay(theta);
       digitalWrite(motorR_A,LOW);
       digitalWrite(motorR_B,LOW);
       digitalWrite(motorL_A,LOW);
       digitalWrite(motorL_B,LOW);
}
void turnRight(uint32_t theta){
       digitalWrite(motorR_A,LOW);
       digitalWrite(motorR_B,HIGH);
       digitalWrite(motorL_A,HIGH);
       digitalWrite(motorL_B,LOW);
       delay(theta);
       digitalWrite(motorR_A,LOW);
       digitalWrite(motorR_B,LOW);
       digitalWrite(motorL_A,LOW);
       digitalWrite(motorL_B,LOW);
}

//Return 0 if sucessfully turn back to original course
//Return 1 if encounter "FLAG_AVOIDENCE_ON =1"
bool fixDirection(){
  //Turn on the flag to get the current course, turn off when not used to save resources
  FLAG_GET_COURSE =1;
  
  //Make sure we can get the latest data
  delay(20);
  int difference = sensorReader.course-startCousre;
  //keep fixing direction if there is no danger
  while(TOLERANCE_Course < abs(difference)){
      if (!FLAG_AVOIDENCE_ON){
            if (difference<0)
                turnRight (100);
            else 
                turnLeft (100);
      }
      else{ 
            //If there is dangers detected, then stop fixing direction and return
            FLAG_GET_COURSE =0;
            return 1;
            
      }
      //For every loop while fixing direction, update the difference between current direction and target direction
      difference = sensorReader.course-startCousre;
  }
 
   //Turn off course fetching to save resources
    FLAG_GET_COURSE =0;
  return 0;
  


}


//reading the amplitude of input sound to decide whether enter mode2
void readSound(){
  uint16_t readValue = 0;
    readValue = analogRead(microphonePin);
    if (threadholdDist_Microphone<readValue)
      FLAG_PASS_FINISH_LINE =1;
      #ifdef DEBUG
        Serial.println("readSound");
      #endif
}
/*
void setupHMC5883L(){
  //Setup the HMC5883L, and check for errors
  int error;  
  error = compass.SetScale(1.3); //Set the scale of the compass.
  if(error != 0){Serial.println(compass.GetErrorText(error));} //check if there is an error, and print if so

  error = compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
  if(error != 0) Serial.println(compass.GetErrorText(error)); //check if there is an error, and print if so
}

float getHeading(){
  //Get the reading from the HMC5883L and calculate the heading
  MagnetometerScaled scaled = compass.ReadScaledAxis(); //scaled values from compass.
  float heading = atan2(scaled.YAxis, scaled.XAxis);

  // Correct for when signs are reversed.
  if(heading < 0) heading += 2*PI;
  if(heading > 2*PI) heading -= 2*PI;

  return heading * RAD_TO_DEG; //radians to degrees
}
*/
void setup(){
  Serial.begin(9600);
  //Wire.begin();
 // pinMode(8,OUTPUT);
  //tone(8, 38000); 
 // analogWrite(8,125);
 
 //Set up the pin for sensor and motor
  pinMode(trig1,OUTPUT);
  pinMode(echo1,INPUT);
  pinMode(trig2,OUTPUT);
  pinMode(echo2,INPUT);
  pinMode(trig3,OUTPUT);
  pinMode(echo3,INPUT);
  pinMode(FRpin,INPUT);
  
  pinMode(motorR_A,OUTPUT);
  pinMode(motorR_B,OUTPUT);
  pinMode(motorL_A,OUTPUT);
  pinMode(motorL_B,OUTPUT);
  
  
  
  //Compass
  //compass = HMC5883L();
  //setupHMC5883L(); 
 
 // delay(1000);
  //startCousre = getCourse();
  Serial.print("Start Course: ");
  Serial.println(startCousre);

  
  

 // We have two task to do every interrupt
 // 1.Excecude sensor thread
 // 2.read Microphone amplitude
  microphoneThread.setInterval(20000);
  //microphoneThread.enabled = true;
  microphoneThread.onRun(&readSound);
  sensorReader.setInterval(50000);
  //sensorReader.enabled = true;
  controller.add(&sensorReader);
  controller.add(&microphoneThread);
 // controller.add(&mythread);
        //Interrupt every 10 ms
        Timer1.initialize(100000);
	Timer1.attachInterrupt(timerCallback);	
        //Timer1.start();
        
       
        
    Mode1();
    Mode2();
}

//Get Free Ram space
int free_ram()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void Mode1(){
  #ifdef DEBUG
              Serial.println("MODE 1");
  #endif
  while(!FLAG_PASS_FINISH_LINE){
      if (FLAG_AVOIDENCE_ON){
        avoidence();
        #ifdef DEBUG
              Serial.println("Avoidence");
        #endif
      }
      else{ 
        while (!FLAG_AVOIDENCE_ON){
            goStraight(10);
            #ifdef DEBUG
              Serial.println("Go straight");
            #endif
        }
        
      }
  }
  
}

void Mode2(){

        #ifdef DEBUG
              Serial.println("MODE 2");
            #endif

}

void loop(){

  
  /*
  int rfValue;
  rfValue =  digitalRead(FRpin);
  Serial.println(rfValue);
  */
  
  
  //
  delay(100);
 
  /*
    Serial.println("");
    Serial.print("Distance1: ");
    Serial.println(sensorReader.distance1);
     Serial.print("Distance2: ");
    Serial.println(sensorReader.distance2);
     Serial.print("Distance3: ");
    Serial.println(sensorReader.distance3);
     Serial.println("");
    delay(5);
    */
}
/*
 
void loop(){
 
  Serial.print("Time");
  Serial.println(time);
  time += 1;

  Serial.print("Distance:");
  Serial.println(countDistance(getDistance(1),getDistance(2)));
  
  
  for (uint8_t count =1;count<4;count++){
   
   
   Serial.print("Dist");
   Serial.print(count);
   Serial.print(": ");
   Serial.print(getDistance(count));
   Serial.println(" cm");
   
   //Serial.clear();
   }
   
  delay(1000);
 
}
*/ 
float calEntranceWidth(float distanceA,float distanceB){

  float dist =sqrt(pow(distanceA,2)+pow(distanceB,2)-2*distanceA*distanceB*cos(pi/6));

  return dist;

}
//Will Return value within 80 cm 
float getDistance(uint8_t select){

  uint8_t trig;
  uint8_t echo;

  switch(select){
  case 1:
    {
      trig = trig1;
      echo = echo1;
      break;
    }

  case 2:
    {
      trig = trig2;
      echo = echo2;  
      break;
    }
  case 3:
    {
      trig = trig3;
      echo = echo3;
      break;
    }

  }

  float duration, dist;
  digitalWrite(trig,HIGH);
  delayMicroseconds(1000);
  digitalWrite(trig,LOW);
  duration = pulseIn(echo,HIGH, sonic_timeout);
  dist = duration/2/sonic_v;
  return dist;
}


