// arduino 8 bit sequencer based on lookmumnocomputers design
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

int currentStep = 0;
int direction = 0; //direction sequence or step will go : 1 for forward, -1 for reverse, 0 for current step
int directionNow = 1; //direction now chosen : 1 = forward, -1 = reverse
int swiState = 0; // 0 = steady state (off/LOW), this is used to make sure we do not count 1 press as multiples by checking for release
int btnState[] = {0,0,0,0,0,0,0,0};

//define variables (for BPM and millis)
int BPM = 120;
int BPMnow = BPM;
bool stepTriggered = false; //true if a step has been triggered but not yet solved
bool autoMode = false; //if true then in sequence program mode where sequence will proceed at BPM
int loopTriggerBPM = 0; //if 1 or above then loop in BPM setting has been triggered, 0 if not triggered

//bool inZeroMode = false;
//unsigned long sequenceStepTime = 0;
unsigned long sequenceStepTimeStart = millis();
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
}

void loop() {
  // keyboard button presses
  for (int i = 0; i < 8; i++) {
    tmp = digitalRead(keyboardBtnPins[i]);
    if (tmp == HIGH) {
      currentStep = i;
      if (autoMode) {
        btnState[i] = 1;
        btnPressTime[i] = millis();
      } 
    }
  }

  // function switch presses
  if (digitalRead(resetSwiPin) == HIGH) {
    resetActive = true;
  }
  if (digitalRead(zeroSwiPin) == HIGH) {
    zeroActive = true;
  }
  if (digitalRead(forwardSwiPin) == HIGH) {
    forwardActive = true;
  }
  if (digitalRead(reverseSwiPin) == HIGH) {
    reverseActive = true;
  }

  if (resetActive || zeroActive || forwardActive || reverseActive ) {
    if (swiState == 0 ) {
      swiState = 1;
      swiPressTime = millis();
      
      if (swiPressTime == 0) {
        if (resetActive) {
          currentStep = -1;
          autoMode = false;
        } else if (zeroActive) {
          currentStep = 0;
        } else if (forwardActive) {
          directionNow = 1;
        } else if (reverseActive) {
          directionNow = -1;
        }
      } 
    } else if (swiState == 1 ) {
      swiHoldDuration = millis() - swiPressTime;
      if (forwardActive || reverseActive) {
        if (!autoMode ) { 
          if (swiHoldDuration >= 1000) {
            autoMode = true;
            direction = directionNow;
            sequenceStepTimeStart = millis();
            sequenceStepTimeNext = sequenceStepTimeStart + 60/BPM*1000;
            //need to setup a lock so it does not influence BPM unil next keypress
          } else if (!stepTriggered) { //triggers the step 1x each time the switch is pressed
            stepTriggered=true; 
          }
        } else if (automode ) { // automode
          if (swiHoldDuration < 1000 && loopTriggerBPM == 0 ) {
            //change BPM by 1 in direction
            BPM = BPM += directionNow;
            loopTriggerBPM = 1;
          } else if (swiHoldDuration >= 1000 + loopTriggerBPM * 200 - 200 ) {
            //change BPM based on time held ~5 for every 1 second
            // BPM = BPMnow + dir * direction * ((buttonHoldTime-1000)/500);
            BPM += directionNow;
            loopTriggerBPM += 1;
          }
        }
      }
  } else if (swiState == 1 && (resetActive && zeroActive && forwardActive && reverseActive) ) {
    swiHoldDuration = millis() - swiPressTime; 
    swiState = 0;
    loopTriggerBPM = 0;
  }

      



  // ouput assimilation
  // if momentary switch then
  if (autoMode && millis() > sequenceStepTimeNext) { //if true then next step in automode has been surpassed
    sequenceStepTimeStart = millis();
    sequenceStepTimeNext = sequenceStepTimeStart + 60/BPM*1000;
  }
  //find next step
  currentStep += direction;
  if (currentStep > 7 ) {
    currentStep = 0;
  } else if (currentStep < 0 ) {
    currentStep = 7;
  }

  /*} else if (!autoMode) {
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
    if (currentStep == i || btnState[i]==1) {
      digitalWrite(stepPins[i], HIGH);
    } else {
      digitalWrite(stepPins[i], LOW);
    }
  }
}
