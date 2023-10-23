//int buttonPins[] = {0, 1, 2, 3, 4, 5, A0, A1};
//int stepPins[] = {8, 9, 10, 11, 12, 13, 6, 7};
int buttonPins[] = {9, 10, 11, 12, 19, 5, 6, 7}; //input buttons
int stepPins[] = {17, 16, 15, 14, 1, 2, 3, 4}; //output steps
int forwardPin = 8;
int reversePin = 0;
int resetPin = 18;
int zeroPin = 13;

int currentStep = 0;
int direction = 0;
int keyRelease = 1; // true

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
  // put your main code here, to run repeatedly:

  // keyboard presses
  for (int i = 0; i < 8; i++) {
    if (digitalRead(buttonPins[i])== HIGH) {
      currentStep = i;
    }
  }

  if (digitalRead(forwardPin) == HIGH && keyRelease == 1) {
    if (currentStep == 7) {
      currentStep = 0;
    } else {
      currentStep += 1;
    }
    keyRelease = 0;
  } else if (digitalRead(forwardPin) == LOW && keyRelease == 0){
    keyRelease = 1;
  }

  if (digitalRead(reversePin) == HIGH && keyRelease == 1) {
    if (currentStep == 0) {
      currentStep = 7;
    } else {
      currentStep -= 1;
    }
    keyRelease = 0;
  } else if (digitalRead(reversePin) == LOW && keyRelease == 0){
    keyRelease = 1;
  }

  for (int i = 0; i < 8; i++) {
    
    if (currentStep == i) {
      digitalWrite(stepPins[i], HIGH);
    } else {
      digitalWrite(stepPins[i], LOW);
    }
  }
}
