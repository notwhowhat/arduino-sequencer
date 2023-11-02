// arduino 8 bit sequencer based on lookmumnocomputers design
// now with millis-ification to allow for some spify autoMode sequencing!
int forwardSwiPin = 8; //sets direction forward
int reverseSwiPin = 0; //sets direction reverse
int resetSwiPin = 18; //resets unit sequence state at -1 (not visible)
int zeroSwiPin = 13; //move sequence to zero (first) position 
int keyboardBtnPins[] = {9, 10, 11, 12, 19, 5, 6, 7}; //input buttons
int stepPins[] = {17, 16, 15, 14, 1, 2, 3, 4}; //output steps

bool forwardActive = false;
bool reverseActive = false;
bool resetActive = false;
bool zeroActive = false;

int currentStep = 0; //which step is current 0 is not visible while 1-8 are shown with LEDs via stepPins
bool currentStepOn = false; //tells if current step should be on = true or off = false
int direction = 0; //direction sequence or step will go : 1 for forward, -1 for reverse, 0 for current step
int directionNow = 1; //direction now chosen : 1 = forward, -1 = reverse
int swiState = 0; // 0 = steady state (off/LOW), this is used to make sure we do not count 1 press as multiples by checking for release
int btnState[] = {0,0,0,0,0,0,0,0};
//int autoArpeggiator[8];
int autoBtnMode = 0; //0 = excluding buttons/steps @BPM, 1 = output record mode order of presses @BPM, 2 = ouput record mode order of presses, holds and spaces
int autoRecordingStep = 0; // in autoBtnMode 1 or 2 used to track which step we are at for output.

//define variables (for BPM and millis)
int BPM = 60;
//int BPMnow = BPM;
bool stepTriggered = false; //true if a step has been triggered but not yet solved
bool autoMode = false; //if true then in sequence program mode where sequence will proceed at BPM or according to autoBtnMode
int loopTriggerBPM = 0; //if 1 or above then loop in BPM setting has been triggered, 0 if not triggered
bool tmpDigitalRead = false; // low = false high = true for tmp button

//bool inZeroMode = false;
//unsigned long sequenceStepTime = 0;
unsigned long millisNow = millis();
unsigned long sequenceStepTimeStart = millisNow;
unsigned long sequenceStepTimeNext = sequenceStepTimeStart + 60/BPM*1000; //given current sequence time, this will provide the next moment when the sequence will move in direction given
unsigned long swiPressTime = 0; //millis when pressed
unsigned long swiHoldDuration = 0; // difference for comparision between times, eg = millis() - swiPressTime;
unsigned long btnPressTime[] = {0,0,0,0,0,0,0,0}; //millis when pressed
unsigned long btnHoldDuration[] = {0,0,0,0,0,0,0,0}; // difference for comparision between times, eg = millis() - swiPressTime;

void setup() {
  // basic setup.
  for (int i = 0; i < 8; i++) {
    pinMode(keyboardBtnPins[i], INPUT);
    pinMode(stepPins[i], OUTPUT);
  }
  pinMode(forwardSwiPin, INPUT);
  pinMode(reverseSwiPin, INPUT);
  pinMode(resetSwiPin, INPUT);
  pinMode(zeroSwiPin, INPUT);
  startTest();
}


void outputPins(int currentStep, bool currentStepOn, bool btnState[] ) {
  //ouput given sequence steps
  for (int i = 0; i < 8; i++) {
    if ((currentStep == i && currentStepOn == true) || btnState[i] == true ) {
      digitalWrite(stepPins[i], HIGH);
    } else {
      digitalWrite(stepPins[i], LOW);
    }
  }
}

void startTest( ) {
  //ouput given sequence steps
 for (int j = 7; j >= -2; j--) { 
  currentStep = j;
  for (int i = 0; i < 8; i++) {
      if (currentStep == i || currentStep == i-1 ) {
        digitalWrite(stepPins[i], HIGH);
      } else {
        digitalWrite(stepPins[i], LOW);
      }
      delay(25);
    }
  }
}


bool countdown(void) {
  /*
  if (countdownTime + (60000 / BPM * 6) > millis()) {
    // start flashing
    for (int j = 0; j < 8; j++) {
      digitalWrite(stepPins[j], HIGH);                  
    }
    delay((60000 / BPM)); // one beat
    for (int j = 0; j < 8; j++) {
      digitalWrite(stepPins[j], LOW);                  
    }                
    delay((60000 / BPM)); // one beat         
    // show that it has been sucsessfully done
    return true;
  }
  return false;
  */

  for (int i = 0; i < 4; i++) {
    // start flashing
    for (int j = 0; j < 8; j++) {
      digitalWrite(stepPins[j], HIGH);                  
    }
    delay((60L*1000 / BPM)); // one beat
    for (int j = 0; j < 8; j++) {
      digitalWrite(stepPins[j], LOW);                  
    }                
    delay((60L*1000 / BPM)); // one beat         
  }
  // show that it has been sucsessfully done
  return true;
}

void loop() {

  // keyboard button press check
  int btnsPressed = 0;
  for (int i = 0; i < 8; i++) {
    tmpDigitalRead = digitalRead(keyboardBtnPins[i]);
    if (tmpDigitalRead == true) {
      currentStep = i;
      currentStepOn = true;
      btnsPressed++;
      if (autoMode) {
        btnState[i] = 1;
        btnPressTime[i] = millis();
      }
    } else if (tmpDigitalRead == false) {
      btnState[i] = 0;
      btnPressTime[i] = 0;
      //if (!autoMode) {
      //  currentStep = -1;
      //}
    }
  }
  if (btnsPressed==0 && !autoMode){currentStepOn = false;}
  // function switch press check
  
  resetActive = digitalRead(resetSwiPin);
  zeroActive = digitalRead(zeroSwiPin);
  forwardActive = digitalRead(forwardSwiPin);
  reverseActive = digitalRead(reverseSwiPin);

  millisNow = millis();
  if (resetActive || zeroActive || forwardActive || reverseActive ) {
    if (swiState == 0 ) {
      swiState = 1;
      if (swiPressTime == 0) {
        if (resetActive) {
          currentStep = -1;
          autoMode = false;
          for (int i = 0; i < 8; i++) {
            digitalWrite(stepPins[i], LOW);
          }
        } else if (zeroActive) {
          for (int i = 0; i < 8; i++) {
            digitalWrite(stepPins[i], LOW);
          }
          currentStep = 0;
        } else if (forwardActive) {
          directionNow = 1;
          if (!autoMode) {stepTriggered=true;}
        } else if (reverseActive) {
          directionNow = -1;
          if (!autoMode) {stepTriggered=true;}
        }
        swiPressTime = millisNow;
      } 
    } 
    if (swiState == 1 ) {
      swiHoldDuration = millisNow - swiPressTime;
      /*
      if (zeroActive) {
        if (swiHoldDuration >= 2000) {
          if (autoBtnMode == 1) { autoBtnMode = 2;}
          if (autoBtnMode == 2) { autoBtnMode = 1;}      
        }
      }
      */
      if (forwardActive || reverseActive) {
        if (!autoMode ) { 
          currentStepOn = true;
          direction = directionNow;
          if (swiHoldDuration >= 1000) {
            autoMode = true;
            sequenceStepTimeStart = millisNow;
            sequenceStepTimeNext = sequenceStepTimeStart + (60L*1000)/BPM;//*1000;
            loopTriggerBPM = -1; //-1 to lock the BPM value so it does not influence BPM unil next switchpress
          }
        } else if (autoMode ) { // automode
          if (swiHoldDuration < 1000 && loopTriggerBPM == 0 ) {
            //change BPM by 1 in direction
            if (direction == directionNow) {
              BPM += 1;
            } else {
              BPM -= 1;
            }
            loopTriggerBPM = 1; //this makes it go forward in auto mode .. removed now as was anoying to test.
          } else if (loopTriggerBPM > 0 && swiHoldDuration >= 1000 + (loopTriggerBPM -1 ) * 100 ) {
            //change BPM based on time held ~10 for every 1 second
            if (direction == directionNow) {
              BPM += 1;
            } else {
              BPM -= 1;
            }
            loopTriggerBPM += 1;
          }
        }
      }
    }
  } else if (swiState == 1 && (!resetActive && !zeroActive && !forwardActive && !reverseActive) ) { //clean up once nothing is active
    // NOTE: could be an issue if two switches are actived at same time?
    swiHoldDuration = 0; //millisNow - swiPressTime; 
    swiState = 0;
    swiPressTime = 0;
    loopTriggerBPM = 0;
    if (!autoMode){ currentStepOn = false;}
  }
  
  // ouput assimilation
  // automode
  if (autoMode && millisNow > sequenceStepTimeNext) { //if true then next step in automode has been surpassed so lets trigger a step
    sequenceStepTimeStart = millis();
    if(autoBtnMode <= 1 ) { //modes that follow BPM
      // tofix: this should be in the normal auto mode check aswell as the buttonmode check, but not only this one.
      sequenceStepTimeNext = sequenceStepTimeStart + (60L*1000)/BPM;//60/BPM*1000;
      stepTriggered = true;
    } else if (autoBtnMode == 2) { //mode that follows recorded input
      //here it will need to follow the lists provided to know when to start 
      sequenceStepTimeStart = millis();
      sequenceStepTimeNext = sequenceStepTimeStart + btnPressTime[autoRecordingStep]; 
      autoRecordingStep +=1;
      //check if autoRecordingStep is above number of steps if so then startover.
      //!!! need to fix output, clear all ouput
      //clear all output except output button
      //then
      //~~~~>?? currentStep = autoRecordingBtn[autoRecordingStep];
    } else {
      // normal basic autoMode
      sequenceStepTimeNext = sequenceStepTimeStart + (60L*1000)/BPM;//60/BPM*1000;
      stepTriggered = true;
    }
  }

  //find next step if triggered via forward or reverse or via auotmode
  if (stepTriggered == true) {
    digitalWrite(stepPins[currentStep], LOW);
    
    stepTriggered = false;
    currentStep += direction;
    if (currentStep > 7 ) {
      currentStep = 0;
    } else if (currentStep < 0 ) {
      currentStep = 7;
    }
    digitalWrite(stepPins[currentStep], HIGH);
  }
  
  
  /*
  } else if (!autoMode) {
    if (direction == 1) {
      if (currentStep == 7) {
        currentStep = 0;
      } else {
        currentStep += 1;
      }
    } else if (direction == -1){
      if (currentStep <= 0) {
        currentStep = 7;
      } else {
        currentStep -= 1;
      }
    }  
  }
  */
  
  //ouput given sequence steps
  for (int i = 0; i < 8; i++) {
    if ((currentStep == i && currentStepOn == true) || btnState[i] == 1 ) {
      digitalWrite(stepPins[i], HIGH);
    } else {
      digitalWrite(stepPins[i], LOW);
    }
  }
  
}
