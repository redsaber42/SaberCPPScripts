/*
 * "Sonic Screwdriver" created by Daniel (Saber C++) 12/2023. 
 * 
 * You have my permission to use, copy, modify, merge, and distribute this and associated files.
 * I ask only two things:
 *    Leave this header at the top of the file, making it clear where the code originated from. Feel free to add your own name as well if you make any modifications: "created by Daniel (Saber C++) MM/YYYY. Modified by John Doe MM/YYYY."
 *    Keep it completely free. This code is NOT to be sold. Products created that run this code or a modification of it MAY be sold.
 *
 * This software is provided "as is" with absolutely no warranty.
*/


#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <FlashStorage_SAMD.h>
#include <FlashStorage_SAMD.hpp>
#include <IRremote.h>
#include "Sounds.h"
#include <Wire.h>


Adafruit_MPU6050 mpu;

// Constants
#define LED_R 0
#define LED_G 2 // NOTE: I switched G to 2 and B to 1 because I accidentally swapped them while soldering.
#define LED_B 1 // Feel free to undo this if you got it right!

#define IR_OUT 3
#define IR_IN  8

#define SPEAKER 6

#define LIGHT_SENSOR A7

#define GESTURE_DELAY 1000
#define MOVEMENT_THRESHOLD 0.5

#define LONG_PRESS_MAX_TIME 4000 // aim for 2 seconds
#define REG_PRESS_MAX_TIME 1200  // aim for 1 second
#define SHORT_PRESS_MAX_TIME 400 // aim for super short
#define SHORT_PRESS_MIN_TIME 100

enum STATE {
  REMOTE,
  SCANNER,
  TOY
};
#define NUM_STATES 3

enum INPUT_TYPE {
  NONE,
  SHORT_PRESS,
  REG_PRESS,
  LONG_PRESS,
  UP,
  DOWN,
  LEFT,
  RIGHT,
  FORWARD,
  BACKWARD,
  CLOCKWISE,
  COUNTER_CLOCKWISE
};

struct color {
  int r;
  int g;
  int b;

  color(int _r, int _g, int _b) {
    r = _r;
    g = _g;
    b = _b;
  }
};

#define BRIGHTNESS_FLUCTUATION 5
enum SONIC_COLOR {
  RED,
  GREEN,
  BLUE
};
color SONIC_COLORS[3] {
  color(250, 0, 0),
  color(0, 250, 0),
  color(0, 0, 250)
};
#define NUM_SONIC_COLORS 3

color TEMP_COLORS[10] {
  color(255, 255, 255), // WHITE <10
  color(190, 190, 255), // WHITE BLUE 10-19
  color(100, 100, 255), // LIGHT BLUE 20-29
  color(0, 0, 255),     // BLUE 30-39
  color(0, 255, 255),   // BLUE-GREEN 40-49
  color(0, 255, 0),     // GREEN 50-59
  color(128, 255, 0),   // YELLOW-GREEN 60-69
  color(255, 255, 0),   // YELLOW 70-79
  color(255, 128, 128), // ORANGE 80-89
  color(255, 0, 0)      // RED 90+
};


struct IRCommandData {
    IRData receivedIRData;

    // extensions for sendRaw
    uint8_t rawCode[RAW_BUFFER_LENGTH]; // The durations if raw
    uint8_t rawCodeLength; // The length of the code
};

struct remote {
  IRCommandData shortPress;

  IRCommandData up;
  IRCommandData down;
  IRCommandData left;
  IRCommandData right;

  IRCommandData forward;
  IRCommandData backward;

  IRCommandData clockwise;
  IRCommandData counterClockwise;
};


// Saving Remote Information
FlashStorage(storedRemoteOne, remote);
FlashStorage(storedRemoteTwo, remote);
FlashStorage(storedRemoteThree, remote);

remote remoteOne;
remote remoteTwo;
remote remoteThree;


// Variables

const int stateChangeSound[10] PROGMEM = {700,600,800,700,1000,900,1300,1250,1500,1550};
const int stateChangeSoundNum = sizeof(stateChangeSound)/sizeof(stateChangeSound[0]);
int stateChangeIndex = 0;

const int scanSound[3] PROGMEM = {1300,1000,2000};
const int scanSoundNum = sizeof(scanSound)/sizeof(scanSound[0]);
int scanIndex = 0;

bool stateJustChanged;

short lightPressedBrightness = 0;
int brightnesses[100] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
short numBrightnesses = 100;
long lastBrightnessCheckTime = 0;

STATE currentState = SCANNER; // NOTE: Change the default state HERE!
INPUT_TYPE currentInput = NONE;

int currentColor = RED;

// Holds how much time the sonic should make sound and lights for, constantly decreased and at 0 sonic sound/lights turned off
short sonicLightTime = 0;
short sonicSoundTime = 0;
int loopSoundLength = 0;

long lastLoopSoundPlay = 0;
long lastAdjustTime = 0;

float accelOffset[3] = {0, 0, 0};
long lastGestureRecognizedTime;
long lightPressTime = -1;

INPUT_TYPE scannerInput = NONE;



void setup() {
  Serial.begin(9600);

  IrReceiver.begin(IR_IN, DISABLE_LED_FEEDBACK);
  IrSender.begin(IR_OUT);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.print("MPU init successful!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  calibrate(1000, 100);

  loadSavedRemotes();
}

void loadSavedRemotes() {
  storedRemoteOne.read(remoteOne);
  storedRemoteTwo.read(remoteTwo);
  storedRemoteThree.read(remoteThree);

  Serial.println(remoteOne.shortPress.receivedIRData.command);
  Serial.println(remoteTwo.shortPress.receivedIRData.command);
  Serial.println(remoteThree.shortPress.receivedIRData.command);
}


void loop() {
  adjustTimers();

  // NOTE: Button/light presses come second and will therefore override gesture readings if both happen at the same time. This is intentional.
  readGestures();
  checkForLightPresses();

  checkIRReceiver();

  processInput();

  doEffects();
}


void adjustTimers() {
  long timeDiff = millis() - lastAdjustTime;
  lastAdjustTime = millis();

  sonicLightTime -= timeDiff;
  sonicSoundTime -= timeDiff;

  if (sonicLightTime < 0) {
    sonicLightTime = 0;
    setColorByColor(color(0,0,0));
  }
  if (sonicSoundTime < 0) {
    sonicSoundTime = 0;
  }
}


//*
// Examine the movement of the gyroscope and accelerometer and figure out what gesture (if any) is happening
void readGestures() {
  // Get the time
  long currentTime = millis();
  // Get the gyro values and accel for/back
  float gX, gY, gZ, aX;
  getTunedMovementValues(&gX, &gY, &gZ, &aX);

  // Figure out which axis is biggest
  float strongestMovement = max(max(abs(gX), abs(gY)), max(abs(gZ), abs(aX)));
  
  // Figure out if this axis is over the minimum threshold for movement AND if it's been enough time since the last movement
  if (abs(strongestMovement) >= MOVEMENT_THRESHOLD && currentTime - lastGestureRecognizedTime > GESTURE_DELAY) {
    lastGestureRecognizedTime = currentTime;
    
    // Determine which way this movement is going and print out that gesture
    if (strongestMovement == abs(gX)) {
      currentInput = gX > 0 ? CLOCKWISE : COUNTER_CLOCKWISE;
    }
    else if (strongestMovement == abs(gY)) {
      currentInput = gY > 0 ? UP : DOWN;
    }
    else if (strongestMovement == abs(gZ)) {
      currentInput = gZ > 0 ? RIGHT : LEFT;
    }
    else {
      currentInput = aX > 0 ? FORWARD : BACKWARD;
    }
  }
}

// Calculate an offset that the accelerometer has (while not being used) so that can be removed from future calculations
void calibrate(int time, int iterations) {
  float aX, aY, aZ;

  for (int i = 0; i < iterations; i++) {
    getRawAccelValues(&aX, &aY, &aZ);

    accelOffset[0] -= aX / iterations;
    accelOffset[1] -= aY / iterations;
    accelOffset[2] -= aZ / iterations;

    delay(time/iterations);
  }
}

// Get the tuned values of the gyroscope and accelerometer forward/backward (all in ranges of roughly [-1, 1] and inverted as necessary so they make sense with the movement)
void getTunedMovementValues(float* gX, float* gY, float* gZ, float* aX) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Give all 3 axes gyroscope/rotation values
  *gX = -g.gyro.x / 5.0;
  *gY = -g.gyro.y / 5.0;
  *gZ = g.gyro.z / 5.0;
  // Give forward/backward acceleration/movement value
  *aX = -(a.acceleration.x + accelOffset[0]) / 7.0;
}

// Get the raw values of the accelerometer
void getRawAccelValues(float* aX, float* aY, float* aZ) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Give forward/backward acceleration/movement value
  *aX = a.acceleration.x;
  *aY = a.acceleration.y;
  *aZ = a.acceleration.z;
}

// Get the tuned up/down axis acceleration (with gravity removed, roughly in range of [-1,1])
float getTunedVerticalAccel() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  return -(a.acceleration.z + accelOffset[2]) / 40;
}

//*/



// Check the light sensor for 'presses' when it's covered for a certain period of time
void checkForLightPresses() {
  // Tune the sensor based on brightness in area
  int sensorBrightness = analogRead(LIGHT_SENSOR);

  if (millis() - lastBrightnessCheckTime > 10 && lightPressTime == -1) {
    
    float averageBrightness = brightnesses[0] / (numBrightnesses*1.0);
    for (int i = 1; i < numBrightnesses; i++) {
      averageBrightness += brightnesses[i] / (numBrightnesses*1.0);
      brightnesses[i - 1] = brightnesses[i];
    }
    brightnesses[numBrightnesses-1] = sensorBrightness;

    if (averageBrightness < 10)
      lightPressedBrightness = averageBrightness/1.5;
    else if (averageBrightness < 20)
      lightPressedBrightness = averageBrightness/2.0;
    else
      lightPressedBrightness = averageBrightness/3.0;
    
    lastBrightnessCheckTime = millis();
  }
  

  if (sensorBrightness < lightPressedBrightness && lightPressTime == -1) {
    lightPressTime = millis();
  }
  
  if (sensorBrightness > lightPressedBrightness && lightPressTime != -1) {
    long timeDown = millis() - lightPressTime;

    if (timeDown > LONG_PRESS_MAX_TIME) {
      // It's just gotten too dark, so reset the brightnesses array to 0s and rebuild average from the ground up
      for (int i = 0; i < numBrightnesses; i++) {
        brightnesses[i] = 0;
        lightPressedBrightness = 0;
        lightPressTime = -1;
        return;
      }
    }

    if (timeDown < LONG_PRESS_MAX_TIME && timeDown > REG_PRESS_MAX_TIME) {
      currentInput = LONG_PRESS;
    }
    else if (timeDown < REG_PRESS_MAX_TIME && timeDown > SHORT_PRESS_MAX_TIME) {
      currentInput = REG_PRESS;
    }
    else if (timeDown < SHORT_PRESS_MAX_TIME && timeDown > SHORT_PRESS_MIN_TIME) {
      currentInput = SHORT_PRESS;
    }

    lightPressTime = -1;
  }
}

// Send the input to the appropriate function based on the current mode
void processInput() {
  if (currentInput == NONE)
    return;

  // If the input is to switch the mode, do that instead
  if (currentInput == LONG_PRESS) {
    currentState = (currentState == NUM_STATES-1) ? (STATE)0 : (STATE)(currentState + 1);

    // In scanner state light should be constantly on (well, time out eventually for battery)
    // In other states light should flash to let user know what color they're on and that they changed
    sonicLightTime = currentState == SCANNER ? 600000 : 500;
    stateJustChanged = true;

    currentInput = NONE;
    return;
  }

  Serial.print("Received Input: ");
  Serial.println(inputToString(currentInput));

  // Interpret the input based on the current state
  switch (currentState) {
    case REMOTE:
      doRemoteActions();
      break;
    case SCANNER:
      doScannerActions();
      break;
    case TOY:
      doToyActions();
      break;
    default:
      Serial.println("Current State is not an accepted state. Switching to toy state!");
      currentState = TOY;
      doToyActions();
      break;
  }

  // After necessary actions have been performed, clear the current input
  currentInput = NONE;
}

// Send out a saved signal based on the given gesture
void doRemoteActions() {
  // If REG_PRESS, switch which remote is selected (includes changing sonic light color)
  if (currentInput == REG_PRESS) {
    currentColor = (currentColor == NUM_SONIC_COLORS-1) ? 0 : currentColor + 1;
    sonicLightTime = 500;
    currentInput = NONE;
    return;
  }

  // Do an effect to show the user that we're performing the action
  sonicLightTime += 500;
  sonicSoundTime += 500;

  IRData dataForInput;
  // Output the code from the correct remote
  switch (currentColor) {
    case 0:
      sendRemoteOneCode();
      break;
    case 1:
      sendRemoteTwoCode();
      break;
    case 2:
      sendRemoteThreeCode();
      break;
    default:
      Serial.print("Can't send code for remote ");
      Serial.print(currentColor);
      Serial.println(" as that remote doesn't exist.");
      break;
  }

  // Clear out input
  currentInput = NONE;
}
void sendRemoteOneCode() {
  switch (currentInput) {
    case SHORT_PRESS:
      sendIRCode(&remoteOne.shortPress);
      break;
    case UP:
      sendIRCode(&remoteOne.shortPress);
      break;
    case DOWN:
      sendIRCode(&remoteOne.shortPress);
      break;
    case LEFT:
      sendIRCode(&remoteOne.shortPress);
      break;
    case RIGHT:
      sendIRCode(&remoteOne.shortPress);
      break;
    case FORWARD:
      sendIRCode(&remoteOne.shortPress);
      break;
    case BACKWARD:
      sendIRCode(&remoteOne.shortPress);
      break;
    case CLOCKWISE:
      sendIRCode(&remoteOne.shortPress);
      break;
    case COUNTER_CLOCKWISE:
      sendIRCode(&remoteOne.shortPress);
      break;
    default:
      Serial.println("Unknown input, can't send remote one code!");
      break;
  }
}
void sendRemoteTwoCode() {
  switch (currentInput) {
    case SHORT_PRESS:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case UP:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case DOWN:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case LEFT:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case RIGHT:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case FORWARD:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case BACKWARD:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case CLOCKWISE:
      sendIRCode(&remoteTwo.shortPress);
      break;
    case COUNTER_CLOCKWISE:
      sendIRCode(&remoteTwo.shortPress);
      break;
    default:
      Serial.println("Unknown input, can't send remote two code!");
      break;
  }
}
void sendRemoteThreeCode() {
  switch (currentInput) {
    case SHORT_PRESS:
      sendIRCode(&remoteThree.shortPress);
      break;
    case UP:
      sendIRCode(&remoteThree.shortPress);
      break;
    case DOWN:
      sendIRCode(&remoteThree.shortPress);
      break;
    case LEFT:
      sendIRCode(&remoteThree.shortPress);
      break;
    case RIGHT:
      sendIRCode(&remoteThree.shortPress);
      break;
    case FORWARD:
      sendIRCode(&remoteThree.shortPress);
      break;
    case BACKWARD:
      sendIRCode(&remoteThree.shortPress);
      break;
    case CLOCKWISE:
      sendIRCode(&remoteThree.shortPress);
      break;
    case COUNTER_CLOCKWISE:
      sendIRCode(&remoteThree.shortPress);
      break;
    default:
      Serial.println("Unknown input, can't send remote three code!");
      break;
  }
}
void sendIRCode(IRCommandData *commandToSend) {
  if (commandToSend->receivedIRData.protocol == UNKNOWN) {
    // Assume 38 KHz, send the raw info received
    IrSender.sendRaw(commandToSend->rawCode, commandToSend->rawCodeLength, 38);
  }
  else {
    // Use the write function, which does the switch for different protocols
    IrSender.write(&commandToSend->receivedIRData);
  }
}

// Map received signals to received gestures and save them permanently
void doScannerActions() {
  // If REG_PRESS, switch which remote is being adjusted (includes changing sonic light color)
  if (currentInput == REG_PRESS) {
    currentColor = (currentColor == NUM_SONIC_COLORS-1) ? 0 : currentColor + 1;
    sonicLightTime = 600000;
    scannerInput = NONE;
    currentInput = NONE;
    return;
  }

  // Turn the sonic light on so they can see which remote they're on
  // TODO: Add proper sleep behavior! (after 10 minutes?)
  sonicLightTime = 600000;

  // If we don't have a gesture, note which input we just got in variable and start listening for IR
  if (scannerInput == NONE) {
    Serial.println("Got gesture, waiting for IR...");
    scannerInput = currentInput;
    // TODO: FIND SOME WAY to notfiy the user which gesture so they know and can cancel if not the right one
    IrReceiver.start();
  }
}
void checkIRReceiver() {
  // If IR received (must have been listening, but check haveGesture just to be safe) 
  // Then save IR code under input gesture, then remove input gesture and turn of IR listening so we're listening for gesture again
  if (currentState != SCANNER || !IrReceiver.decode() || scannerInput == NONE)
    return;

  Serial.println("Got IR");

  // Store the received code according to its protocol
  IRCommandData receivedCommand;
  receivedCommand.receivedIRData = IrReceiver.decodedIRData;
  // If the protocol is unknown, also store the raw version of the command
  if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    Serial.println("Received IR code with protocol UNKNOWN");
    receivedCommand.rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
    IrReceiver.compensateAndStoreIRResultInArray(receivedCommand.rawCode);
  }
  // If the protocol is know, clear out the flags (so the signal doesn't include a repeating flag or something like that which would be very unhelpful)
  else {
    Serial.print("Received IR code with protocol ");
    Serial.println(receivedCommand.receivedIRData.protocol);
    receivedCommand.receivedIRData.flags = 0;
  }

  // Set the proper remote to have the new value under the proper gesture
  switch (currentColor) {
    case 0:
      saveRemoteOneCode(receivedCommand);
      break;
    case 1:
      saveRemoteTwoCode(receivedCommand);
      break;
    case 2:
      saveRemoteThreeCode(receivedCommand);
      break;
    default:
      Serial.print("Couldn't save code for remote as remote ");
      Serial.print(currentColor);
      Serial.println(" doesn't exist");
      break;
  }
  Serial.print("Saved remote code for input ");
  Serial.println(inputToString(scannerInput));

  // Clear out the input and stop the ir receiver from listening until we have another gesture
  scannerInput = NONE;
  IrReceiver.stop();
  Serial.println("Back to waiting for gesture...");
}
void saveRemoteOneCode(IRCommandData commandToSave) {
  switch (scannerInput) {
    case SHORT_PRESS:
      remoteOne.shortPress = commandToSave;
      break;
    case UP:
      remoteOne.up = commandToSave;
      break;
    case DOWN:
      remoteOne.down = commandToSave;
      break;
    case LEFT:
      remoteOne.left = commandToSave;
      break;
    case RIGHT:
      remoteOne.right = commandToSave;
      break;
    case FORWARD:
      remoteOne.forward = commandToSave;
      break;
    case BACKWARD:
      remoteOne.backward = commandToSave;
      break;
    case CLOCKWISE:
      remoteOne.clockwise = commandToSave;
      break;
    case COUNTER_CLOCKWISE:
      remoteOne.counterClockwise = commandToSave;
      break;
    default:
      Serial.println("Unknown currentInput when trying to store remote one code! Not storing the remote code!");
      break;
  }
  storedRemoteOne.write(remoteOne);
}
void saveRemoteTwoCode(IRCommandData commandToSave) {
  switch (scannerInput) {
    case SHORT_PRESS:
      remoteTwo.shortPress = commandToSave;
      break;
    case UP:
      remoteTwo.up = commandToSave;
      break;
    case DOWN:
      remoteTwo.down = commandToSave;
      break;
    case LEFT:
      remoteTwo.left = commandToSave;
      break;
    case RIGHT:
      remoteTwo.right = commandToSave;
      break;
    case FORWARD:
      remoteTwo.forward = commandToSave;
      break;
    case BACKWARD:
      remoteTwo.backward = commandToSave;
      break;
    case CLOCKWISE:
      remoteTwo.clockwise = commandToSave;
      break;
    case COUNTER_CLOCKWISE:
      remoteTwo.counterClockwise = commandToSave;
      break;
    default:
      Serial.println("Unknown currentInput when trying to store remote two code! Not storing the remote code!");
      break;
  }
  storedRemoteTwo.write(remoteTwo);
}
void saveRemoteThreeCode(IRCommandData commandToSave) {
  switch (scannerInput) {
    case SHORT_PRESS:
      remoteThree.shortPress = commandToSave;
      break;
    case UP:
      remoteThree.up = commandToSave;
      break;
    case DOWN:
      remoteThree.down = commandToSave;
      break;
    case LEFT:
      remoteThree.left = commandToSave;
      break;
    case RIGHT:
      remoteThree.right = commandToSave;
      break;
    case FORWARD:
      remoteThree.forward = commandToSave;
      break;
    case BACKWARD:
      remoteThree.backward = commandToSave;
      break;
    case CLOCKWISE:
      remoteThree.clockwise = commandToSave;
      break;
    case COUNTER_CLOCKWISE:
      remoteThree.counterClockwise = commandToSave;
      break;
    default:
      Serial.println("Unknown currentInput when trying to store remote three code! Not storing the remote code!");
      break;
  }
  storedRemoteThree.write(remoteThree);
}

// Make fun noises and sounds for playing around with (toggle sound/lights, change sonic color, show temperature)
void doToyActions() {
  switch (currentInput) {
    case SHORT_PRESS:
      sonicLightTime = (sonicLightTime > 0) ? 0 : 60000;
      sonicSoundTime = sonicLightTime;
      break;
    case REG_PRESS:
      currentColor = (currentColor == NUM_SONIC_COLORS-1) ? 0 : currentColor + 1;
      sonicLightTime += 500;
      break;
    case COUNTER_CLOCKWISE: {
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      // Converting from default Celsius to Fahrenheit.
      temp.temperature = (temp.temperature * 9.0/5) + 32;

      // Get the number of tens above zero (for 56, that would be 5) and beep that many times
      // Also select a color representing the temperature from red at the hottest into yellow, green, blue, and finally white for the coldest
      int tensAboveZero = max(0, min(temp.temperature / 10, 90));
      setColorByColor(TEMP_COLORS[tensAboveZero]);

      // Beep X times
      for (int i = 0; i < tensAboveZero; i++) {
        tone(SPEAKER, 500, 50);
        delay(200);
      }
      // Delay for the remaining time so we always have the colored light for the same amount of time, regardless of beep count
      delay((10 - tensAboveZero) * 200);

      // Set the color back to what it was
      setColorByIndex(currentColor);
      break;
    }
    case NONE:
      // We don't want no input to go to the default case
      break;
    default:
      // Any other move adds sound and light so you can get continous if you keep inputting
      sonicLightTime += 500;
      sonicSoundTime = sonicLightTime;
      break;
  }
}


void doEffects() {
  if (sonicLightTime > 0) {
    if (currentState == SCANNER) {
      color dim = SONIC_COLORS[currentColor];
      dim.r = max(dim.r - 150, 0);
      dim.g = max(dim.g - 150, 0);
      dim.b = max(dim.b - 150, 0);
      setColorByColor(dim);
    }
    else
      setColorByIndex(currentColor);
  }
  int currentTime = millis();

  if (stateJustChanged && currentTime - lastLoopSoundPlay > loopSoundLength) {
    lastLoopSoundPlay = currentTime;
    int sound = stateChangeSound[stateChangeIndex++];
    loopSoundLength = 20;
    if (stateChangeIndex == stateChangeSoundNum) {
      loopSoundLength = 1000;
      stateChangeIndex = 0;
      stateJustChanged = false;
    }
    tone(SPEAKER, sound, 20);
  }
  else if (currentState == SCANNER && scannerInput != NONE && currentTime - lastLoopSoundPlay > loopSoundLength) {
    lastLoopSoundPlay = currentTime;
    int sound = scanSound[scanIndex++];
    loopSoundLength = 12;
    if (scanIndex == scanSoundNum) {
      loopSoundLength = 1000;
      scanIndex = 0;
    }
    tone(SPEAKER, sound, 12);
  }
  else if (sonicSoundTime > 0 && currentTime - lastLoopSoundPlay > loopSoundLength) {
    lastLoopSoundPlay = currentTime;
    int loopSound = random(800, 1000);
    loopSoundLength = 10;
    tone(SPEAKER, loopSound, loopSoundLength);
  }
}

// NOT CURRENTLY USED: Would use a sine wave (based on the time) to change the brightness/color of the light
void modifyLights() {
  color currentSonicColor = SONIC_COLORS[currentColor];

  float sinValue = sin(millis()/50.0);

  currentSonicColor.r += (sinValue + 1) / 2 * BRIGHTNESS_FLUCTUATION;
  currentSonicColor.g += (sinValue + 1) / 2 * BRIGHTNESS_FLUCTUATION;
  currentSonicColor.b += (sinValue + 1) / 2 * BRIGHTNESS_FLUCTUATION;

  setColorByColor(currentSonicColor);
}


// Set the color of the RGB LEDs
void setColorByIndex(int i) {
  setColorByColor(SONIC_COLORS[i]);
}
void setColorByColor(color c) {
  pinMode(LED_R, OUTPUT);
  bool redOn = c.r >= 100;
  digitalWrite(LED_R, redOn);
  analogWrite(LED_G, c.g);
  analogWrite(LED_B, c.b);
}

// Take in an input type and format it as a string for printing out
String inputToString(INPUT_TYPE input) {
  switch (input) {
    case SHORT_PRESS:
      return F("Short Press");
    case REG_PRESS:
      return F("Reg Press");
    case LONG_PRESS:
      return F("Long Press");
    case UP:
      return F("Up");
    case DOWN:
      return F("Down");
    case LEFT:
      return F("Left");
    case RIGHT:
      return F("Right");
    case FORWARD:
      return F("Forward");
    case BACKWARD:
      return F("Backward");
    case CLOCKWISE:
      return F("Clockwise");
    case COUNTER_CLOCKWISE:
      return F("Counter-Clockwise");
    default:
      return F("Unknown Input!");
  }
}
