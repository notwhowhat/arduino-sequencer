// arduino 8 bit sequencer based on lookmumnocomputers design
int buttonPins[] = {9, 10, 11, 12, 19, 5, 6, 7}; //input buttons
int stepPins[] = {17, 16, 15, 14, 1, 2, 3, 4}; //output steps
int forwardPin = 8;
int reversePin = 0;
int resetPin = 18;
int zeroPin = 13;

int currentStep = 0;
int direction = 0; //direction sequence or step will go : 1 for forward, -1 for reverse, 0 for current step
int directionNow = 1; //direction now chosen : 1 = forward, -1 = reverse
int keyState = 0; // 0 = steady state (off/LOW), this is used to make sure we do not count 1 press as multiples by checking for release

//define variables (for BPM and millis)
int BPM = 120;
int BPMnow = BPM;
bool stepTriggered = false; //true if a step has been triggered
bool sequenceProgramMode = false; //if true then in sequence program mode where sequence will proceed at BPM


bool inZeroMode = false;
//unsigned long sequenceStepTime = 0;
unsigned long sequenceStepTimeStart = millis();
unsigned long sequenceStepTimeNext = sequenceStepTimeStart + 60/BPM*1000; //given current sequence time, this will provide the next moment when the sequence will move in direction given
unsigned long buttonPressTime = 0; //millis when pressed
unsigned long buttonHoldTime = 0; // difference, eg = millis() - buttonPressTime;


void setup() {
  // put your setup code here, to run once:
  for (int i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT);
    pinMode(stepPins[i], OUTPUT);
  }
  pinMode(forwardPin, INPUT);
  pinMode(reversePin, INPUT);
  pinMode(resetPin, INPUT);
  pinMode(zeroPin, INPUT);
}

void loop() {
  // keyboard presses
  for (int i = 0; i < 8; i++) {
    if (digitalRead(buttonPins[i])== HIGH) {
      currentStep = i;
    }
  }

  // switch presses
  if (keyState == 0) {
    if (digitalRead(forwardPin) == HIGH) {
      keyState = 1;
      if (currentStep == 7) {
        currentStep = 0;
      } else {
        currentStep += 1;
      }
    } else if (digitalRead(reversePin) == HIGH) {
      keyState = 1;
      if (currentStep <= 0) {
        currentStep = 7;
      } else {
        currentStep -= 1;
      }
    }
    if (digitalRead(zeroPin) == HIGH) {
      keyState = 1;
      currentStep = 0;
    } else if (digitalRead(resetPin) == HIGH) {
      keyState = 1;
      currentStep = -1;
    }
  } else if (keyState == 1 && digitalRead(forwardPin) == LOW && digitalRead(reversePin) == LOW && digitalRead(zeroPin) == LOW && digitalRead(resetPin) == LOW ) {
    keyState = 0;
  }

  //ouput
  for (int i = 0; i < 8; i++) {
    if (currentStep == i) {
      digitalWrite(stepPins[i], HIGH);
    } else {
      digitalWrite(stepPins[i], LOW);
    }
  }
}
