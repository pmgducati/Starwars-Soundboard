//Trellis Libraries
#include <Wire.h>
#include "Adafruit_Trellis.h"
//MP3Shield Libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
//Screen Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
long currentTime;				  // Time on
long previousTime;				  // Compared time
int Shutdown;					  // Flag to initiate Shutdown
int Battlevel;					  // Battery Level Monitor



//MP3 Shield Declarations

//File name conversions
String filetype, Song;

// These are the pins used for the music maker shield
#define BREAKOUT_RESET 9    // VS1053 reset pin (output)
#define BREAKOUT_CS    10   // VS1053 chip select pin (output)
#define BREAKOUT_DCS   8    // VS1053 Data/command select pin (output)
#define SHIELD_RESET  -1    // VS1053 reset pin (unused!)
#define SHIELD_CS      7    // VS1053 chip select pin (output)
#define SHIELD_DCS     6    // VS1053 Data/command select pin (output)
#define CARDCS         4	// Card chip select pin
#define DREQ 		   3    // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);
  
  
  
 //Screen Declarations
/* #define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
 */
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

void setup() {
  Serial.begin(9600);
  //Serial.println("Gino Starwars Soundboard");

  
  
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
  Serial.println("Adafruit VS1053 Simple Test");

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
  SD.begin(CARDCS);    // initialise the SD card
  musicPlayer.setVolume(50,50); // Set volume (lower numbers == louder volume)
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int for background audio playing



//Screen Setup   

//   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize screen via address

}
void loop() {
  delay(30); // 30ms delay is required, for trellis functionality
  currentTime = millis();
  Battlevel = analogRead(A0);
  float voltage = Battlevel * (5.00 / 1023.00) * 2; //convert the value to a true voltage.
  Serial.print("Voltage: "); Serial.print(voltage); Serial.println("V"); 

  // text display tests - Stock code from adafruit 
//  display.setTextSize(1);
//  display.setTextColor(WHITE);
//  display.setCursor(0,0);
//  display.println("Hello, world!");
//  display.setTextColor(BLACK, WHITE); // 'inverted' text
//  display.println(3.141592);
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  display.print("0x"); display.println(0xDEADBEEF, HEX);
//  display.display();
  
  
   //Random LED Blinking when not being used 
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
  else{
//if no button pressed turn off all LEDs
	 if (sleepTime > 180000 && Shutdown == 1){
        musicPlayer.playFullFile("SHUTDOWN.wav");        
     }
        for (uint8_t i=0; i<numKeys; i++) { 
        trellis.clrLED(i);
        trellis.writeDisplay();     
        delay(50);
        Shutdown = 0;
     }
  }
// If a button was just pressed or released...
    if (trellis.readSwitches()) {
      sleepTime = 0;
      for (uint8_t i=0; i<numKeys; i++) {    // go through every button
  if (trellis.justPressed(i)) {  			 // if it was pressed, turn it on
    trellis.setLED(i);
  } 
  
  if (trellis.justReleased(i)) {			 // if it was released, play audio file and turn it off
    trellis.clrLED(i);
    delay(50);
    Song = i + filetype;					 //Concatenate the strings to filename, convert to char and inject in to play command
    int str_len = Song.length() + 1; 		 //Im sure there is a better way to do this ;)
    char FileName[str_len];
    Song.toCharArray(FileName, str_len);
    musicPlayer.playFullFile(FileName);
  }
      }
      trellis.writeDisplay();				 // tell the trellis to set the LEDs we requested
     
    }
  }

