//Trellis Libraries
#include <Wire.h>
#include "Adafruit_Trellis.h"
//MP3Shield Libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>



//Trellis Declarations

//IDs for each 16x16 Grid
Adafruit_Trellis matrix0 = Adafruit_Trellis();
Adafruit_Trellis matrix1 = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix0, &matrix1);

// Qty of Trellis Grids
#define NUMTRELLIS 2
#define numKeys (NUMTRELLIS * 16)

//Intterupt Pin
#define INTPIN A2

//Variables for random blinking
int randOn = 0;                   // Initialize a variable for the ON time
int randled1 = 0;                 // Initialize a variable for the random LED on function
int randled2 = 0;                 // Initialize a variable for the random LED off function
long sleepTime;                   // Timer to shut off LEDs
long currentTime;				          // Time on
long previousTime;				        // Compared time
int Shutdown;					            // Flag to initiate Shutdown


//MP3 Shield Declarations

//File name conversions
String filetype, Song;

// These are the pins used for the music maker shield
//#define BREAKOUT_RESET 9        // VS1053 reset pin (output)
//#define BREAKOUT_CS    10       // VS1053 chip select pin (output)
//#define BREAKOUT_DCS   8        // VS1053 Data/command select pin (output)
#define SHIELD_RESET  -1          // VS1053 reset pin (unused!)
#define SHIELD_CS      7          // VS1053 chip select pin (output)
#define SHIELD_DCS     6          // VS1053 Data/command select pin (output)
#define CARDCS         4	        // Card chip select pin
#define DREQ 		       3          // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);


//LED Declarations

#define ledRpin 2
#define ledBpin 5
#define ledGpin 8
int brightness = 0;               // how bright the LED is
int fadeAmount = 5;               // how many points to fade the LED by


//On/Mode Button Declarations

#define buttonPin 9
int ButtonValue = 0;
int LEDBlink = 1;

void setup() {
  Serial.begin(9600);
  Serial.println("Gino Starwars Soundboard");


//Setup for Trellis
  //File name concatenation
  filetype = String(".wav");
  Song = String();
  
  //Random Seed for Blinking LEDS
  randomSeed (analogRead (0));
  
  // INT pin requires a pullup
  pinMode(INTPIN, INPUT);
  digitalWrite(INTPIN, HIGH);
  pinMode(5, OUTPUT);
  
  // Addreses for the panels
  trellis.begin(0x70, 0x71);
  
  // light up all the LEDs in order
  for (uint8_t i=0; i<numKeys; i++) {
    trellis.setLED(i);
    trellis.writeDisplay();    
    delay(50);
  }
    // then turn them off randomly
  for (uint8_t i=0; i<21; i++) { 
   randled2 = random (0, 31);
   trellis.clrLED(randled2); 
   trellis.writeDisplay();    
   delay(50);
  }

  
  
//Setup for MP3 Shield

  if (! musicPlayer.begin()) {    // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
  SD.begin(CARDCS);    // initialise the SD card
  musicPlayer.setVolume(40,40); // Set volume (lower numbers == louder volume)
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int for background audio playing

//LED Setup
pinMode(ledRpin, OUTPUT);
pinMode(ledGpin, OUTPUT);
pinMode(ledBpin, OUTPUT);

//button Setup
pinMode(buttonPin, INPUT);

}
void loop() {
  delay(30); // 30ms delay is required, for trellis functionality
  currentTime = millis();
  int ModeNumber = digitalRead(buttonPin);

//When Mode Button is pressed, the sleep timer is reset to 0, shift the button value by +32 and change the mode (1-4)
  if (ModeNumber == 1){   
    sleepTime = 0;
    ButtonValue = ButtonValue + 32;
    LEDBlink = LEDBlink + 1;
    if (LEDBlink == 5){
      LEDBlink = 1;
    }
    //Blink LED # of times to corresponding mode
    for (uint8_t b=1; b<=LEDBlink; b++) {
    digitalWrite(ledBpin, HIGH);
    delay(250);
    digitalWrite(ledBpin, LOW);
    delay (250);
    }
      //resets button value to 0 when returning to mode 1
      if (ButtonValue > 100){
        ButtonValue=0;
        LEDBlink=1;
      }
    }
//Random Trellis LED Blinking when not being used 
   if (sleepTime < 180000){
     sleepTime = ((currentTime - previousTime) + sleepTime);
     previousTime = currentTime;
     Shutdown = 1;
     randOn = random (100, 900); 			// generate ON time between 0.1 and 0.9 seconds
     randled1 = random (0, 31);   			// generate random LED to turn on
     randled2 = random (0, 31);    			// generate random LED to turn off
     trellis.setLED(randled1);            
     trellis.writeDisplay(); 
     delay(randOn);                
     trellis.clrLED(randled2);            
     trellis.writeDisplay();                
   }
//if no button pressed in 3 minutes turn off all LEDs
  else{
	 if (sleepTime > 180000 && Shutdown == 1){
        musicPlayer.playFullFile("SLEEP.wav");        
        for (uint8_t i=0; i<=numKeys; i++) { 
        trellis.clrLED(i);
        trellis.writeDisplay();     
        delay(50);
        Shutdown = 0;
        Serial.println(i);
     } 
	 }
  }
 //Blue LED Breathes when in sleep mode
 if (sleepTime > 180000 && Shutdown == 0){ 
    analogWrite(ledBpin, brightness);
    brightness = brightness + fadeAmount;
      if (brightness <= 0 || brightness >= 255) {
        fadeAmount = -fadeAmount;
      }
    delay(30);
 }
// If a button was just pressed or released...
    if (trellis.readSwitches()) {
      sleepTime = 0;                  //reset sleep timer
      for (uint8_t i=0; i<numKeys; i++) {    // go through every button
  if (trellis.justPressed(i)) {  			 // if it was pressed, turn it on
    trellis.setLED(i);
  } 
  
  if (trellis.justReleased(i)) {			 // if it was released, play audio file and turn it off
    trellis.clrLED(i);
    delay(50);
    i=i+ButtonValue;
    Song = i + filetype;					    //Concatenate the strings to filename, convert to char and inject in to play command
    int str_len = Song.length() + 1; 		 
    char FileName[str_len];
    Song.toCharArray(FileName, str_len);
    digitalWrite(ledBpin, LOW);
    digitalWrite(ledGpin, HIGH);
    musicPlayer.playFullFile(FileName);
    delay(500);
    digitalWrite(ledGpin, LOW);
    Serial.print("   i  ");  Serial.println(i);
  }
      }
      trellis.writeDisplay();				 // tell the trellis to set the LEDs we requested
     
    }
  }

