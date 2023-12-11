/* --------------------------------------------
# arduino-sequencer
## our modular cv step sequencer

![alt text](https://github.com/notwhowhat/arduino-sequencer/blob/main/collaterals/sequencer%208bit%20arduino%20atmega3281.png)
arduino 8 bit sequencer based originally on lookmumnocomputers design and build instructions here  https://www.lookmumnocomputer.com/sequencer-keyboard/
improvements by notwhowhat and ptryk using instead an alternative layout and an atmega328p chip as base and includes the following main changes:
complete new code with millis-ification to allow for some spify autoMode sequencing!! in 3 autoModes!

see README.md for loads of info instructions and todo
https://github.com/notwhowhat/arduino-sequencer/blob/4ee584a79ed4d488f3fb350778e013a050389698/README.md
-------------------------------------------- */
/* BUG: bugs to squash 
BUG: arpeggiator does not start without zero
BUG: arp reset needs to be considered better
*/
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
bool btnState[] = {false,false,false,false,false,false,false,false};

//define variables (for BPM and millis)
int BPM = 120;
float timeFactor = 1.00f;
//int BPMnow = BPM;
bool stepTriggered = false; //true if a step has been triggered but not yet solved
bool autoMode = false; //if true then in sequence program mode where sequence will proceed at BPM or according to autoBtnMode
int loopTriggerBPM = 0; //if 1 or above then loop in BPM setting has been triggered, 0 if not triggered
bool tmpDigitalRead = false; // low = false high = true for tmp button

//autoBtnMode related
int autoBtnMode = 0; //0 = including buttons/steps that the user presses @BPM, 1 = output record mode order of presses @BPM, 2 = ouput record mode order of presses, holds and spaces
int outputListStep = 0; // in autoBtnMode 0, 1 or 2 used to track which step we are at for output.
int outputListSize = 0; // size of the outputList
int outputList[64]; // for recording the notes. 64 steps max len * 8 for arpeggiator
unsigned long autoRecBtnTimeStart[64];
unsigned long autoRecDuration[64];
int autoOutputList[64]; // output list auto - used to save it in conjunction with arpeggiator mode.
int autoOutputListSize; // ouput list size to save it independently if needed for arpeggiator mode.

//millis related
unsigned long millisNow = millis();
unsigned long millisStart = millisNow; // used to normalize the millis in recording :)
unsigned long sequenceStepTimeStart = millisNow;
unsigned long sequenceStepTimeNext = sequenceStepTimeStart + 60/BPM*1000; //given current sequence time, this will provide the next moment when the sequence will move in direction given
unsigned long swiPressTime = 0; //millis when pressed
unsigned long swiHoldDuration = 0; // difference for comparision between times, eg = millis() - swiPressTime;
unsigned long btnPressTime[] = {0,0,0,0,0,0,0,0}; //millis when pressed
unsigned long btnHoldDuration[] = {0,0,0,0,0,0,0,0}; // difference for comparision between times, eg = millis() - swiPressTime;

//bool arpegRecord = true; // to setup arpeggiation
int arpegListSize = 0; // size of list of arpeggiator
int arpegList[10];// = {-1, 0, 1, 0, 0}; // list of changes to current step
int arpegRecordingRoot = 4-1; //when recording this is the root of the arpeggiation that is then recorded relative to this point. 0 is first step so, 4 -1 is the 4th step
unsigned long arpegAutoRecBtnTimeStart[64];
unsigned long arpegAutoRecDuration[64];
bool arpeggiator = false;//true; //true if arpeggiation is turned on
int arpeggiatorStep = 0;//step currently on arpeggiation, this steps through and is reset on each primary sequence step.

// --------------------------------------------
// setup pins for arduino -run 1x
void setup() {
  // basic setup.
  for (int i = 0; i < 8; i++) {
    pinMode(keyboardBtnPins[i], INPUT);
    pinMode(stepPins[i], OUTPUT);
    outputList[i]=i;
  }
  pinMode(forwardSwiPin, INPUT);
  pinMode(reverseSwiPin, INPUT);
  pinMode(resetSwiPin, INPUT);
  pinMode(zeroSwiPin, INPUT);
  startTest();
}


// --------------------------------------------
// ouput
void outputLeds(){//int currentStep, bool currentStepOn, bool btnState[] ) {
  //ouput given sequence steps
  for (int i = 0; i < 8; i++) {
    if ((currentStep == i && currentStepOn == true) || btnState[i] == true ) {
      digitalWrite(stepPins[i], HIGH);
    } else {
      digitalWrite(stepPins[i], LOW);
    }
  }
}


// --------------------------------------------
// startup test of system 
void startTest( ) {
  //ouput given sequence steps
 for (int j = 0; j <= 10; j++) { 
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


// --------------------------------------------
// count down pre recording to record button presses
bool countDown(int loops, float BPMfactor) {
  for (int i = 0; i < loops; i++) {
    // start flashing
    for (int j = 0; j < 8; j++) {
      digitalWrite(stepPins[j], HIGH);                  
    }
    delay((60L*1000 / BPM * BPMfactor)); // one beat
    for (int j = 0; j < 8; j++) {
      digitalWrite(stepPins[j], LOW);                  
    }                
    delay((60L*1000 / BPM * BPMfactor)); // one beat         
  }
  // show that it has been sucsessfully done
  return true;
}


/* --------------------------------------------
main loop()!!!
-------------------------------------------- */
void loop() {
  millisNow = millis();
  // keyboard button press check
  int btnsPressed = 0;
  for (int i = 0; i < 8; i++) {
    tmpDigitalRead = digitalRead(keyboardBtnPins[i]);
    if (tmpDigitalRead == true && btnState[i] == false) {
      currentStep = i;
      currentStepOn = true;
      btnsPressed++;
      if (autoMode) {
        btnState[i] = true;
        btnPressTime[i] = millisNow;
      }
    } else if (tmpDigitalRead == false) {
      btnState[i] = false;
      btnPressTime[i] = 0;
      //if (!autoMode) {
      //  currentStep = -1;
      //}
    }
  }
  if (btnsPressed==0 && !autoMode){currentStepOn = false;}


  // --------------------------------------------
  // switch press check
  resetActive = digitalRead(resetSwiPin);
  zeroActive = digitalRead(zeroSwiPin);
  forwardActive = digitalRead(forwardSwiPin);
  reverseActive = digitalRead(reverseSwiPin);
  
  
  // --------------------------------------------
  // button processing
  int maxBtnHoldDuration = 0; //initilization to follow the maximum time the buttons are held as it loops through teh folloiwng loop

  if(autoMode) {
    btnsPressed = 0;
    /*for (int i = 0; i < 64; i++) {
      outputList[i] = 0;
    }
    */
    for (int i = 0; i < 8; i++) {
      if (btnState[i] == true ) {
        outputList[btnsPressed] = i;
        btnsPressed += 1;
        btnHoldDuration[i] = millisNow - btnPressTime[i];
        if (maxBtnHoldDuration < btnHoldDuration[i] ) {
          maxBtnHoldDuration = btnHoldDuration[i];
        }
      }
    }
  }
  
  // --------------------------------------------
  // autoBtnModes 
  if (maxBtnHoldDuration >= 5000 || (zeroActive && btnsPressed > 0 ) ) {
    maxBtnHoldDuration = 0; //NOTE NEED TO LOCK THIS LOOP AFTER FIRST RUN UNTIL IT IS DONE!!!! , setting maxBtnHoldDuration=0 should do it!
    
    // --------------------------------------------
    // autoBtnMode 0 (multipress)
    //record or not to record that is the queetsion
    if (btnsPressed > 1 ) { // >1 then just cycle through the buttons chosen
      autoBtnMode = 0;
      outputListStep = -1;
      outputListSize = btnsPressed-1;
      btnsPressed = 0;
      countDown(4, 0.25);  //info flash
    }

    // --------------------------------------------
    // autoBtnMode 1 & 2 (single press)
    // one button is pressed, so autoBtnMode 1 or 2 time! , autoBtnMode 1 or 2 time! , autoBtnMode 1 or 2 time! !!
    // first do countdown do da doooo dooo.. do da dooodod dooo
    // the countdown has sucessfully been finnished
    // next: time to record
    // this needs to have all elements of the buttons but also only gets to be run once
    else if (btnsPressed == 1 ) { //if in automode then it will record the primary sequence, otherwise !automode it will be record for the arpeggiator!
      if (autoBtnMode != 2 ) {
        autoBtnMode = 1;
      } else {autoBtnMode = 2; }
      
      countDown(4, 1);
      outputListStep = 0;
      autoRecBtnTimeStart[outputListStep] = 0; //resets state to make sure it is zero
      autoRecDuration[outputListStep] = 0; //resets state to make sure it is zero

      // --------------------------------------------
      // initilization and then recording autoBtnMode 1 & 2
      for (int i = 0; i < 8; i++) {btnState[i] = false; } // clear btnState for recording
      unsigned long millisStart = millis();
      currentStep = -1;
      bool whileCntrl = true; //check for while

      // temp values of recording before deciding if it is for arpeggiatior or steps
      //int outputListSize = 0; // size of the outputList
      int ToutputList[64]; // for recording the notes. 64 steps max len
      unsigned long TautoRecBtnTimeStart[64];
      unsigned long TautoRecDuration[64];
      do {
        millisNow = millis(); 
        zeroActive = digitalRead(zeroSwiPin);
        for (int k = 0; k < 8; k++) { // loops through all of the buttons for to check if they have been pressed
          tmpDigitalRead = digitalRead(keyboardBtnPins[k]);
          if (tmpDigitalRead == true && btnState[k] == false ) {
            // was false last cycle and true now, so new note and new values
            if (outputListStep == 0 ) {
              millisStart = millisNow;
            }
            btnState[k] = true;
            btnPressTime[k] = millisNow;
            
          } else if (tmpDigitalRead == false && btnState[k] == true ) {
            // opposite of last check, so the note has just finnished
            // outputListStep gets changed every time a note has finnished, and is saved for the order
            // of the sequence
            if (millisNow - (TautoRecBtnTimeStart[outputListStep] + TautoRecDuration[outputListStep] ) > 25 ){ //debounce level
              
              ToutputList[outputListStep] = k;

              // subtracted by millisStart, for to get relative time values
              TautoRecBtnTimeStart[outputListStep] = btnPressTime[k] - millisStart; // normalized to the start of the recording
              TautoRecDuration[outputListStep] = millisNow - TautoRecBtnTimeStart[outputListStep]; // duration of press

              outputListStep++;
              TautoRecBtnTimeStart[outputListStep] = 0; //resets state to make sure it is zero
              TautoRecDuration[outputListStep] = 0; //resets state to make sure it is zero

              btnState[k] = false;
            }
          } else if (tmpDigitalRead == false && btnState[k] == false ) {
            // if it has been false for a while, it will remain false
            btnState[k] = false;
          } 
        }
        // show the pressed button (should be just one :) .. what if it isn't :S )
        outputLeds();
        
        if (outputListStep > 0 ) {  
          //not first time through then check for too many steps, dead space or long hold of 5000ms (5s) 
          
          if (outputListStep >= 64 || millisNow - (TautoRecBtnTimeStart[outputListStep-1] + TautoRecDuration[outputListStep-1] ) > 5000 || arpegAutoRecDuration[outputListStep] > 5000 || zeroActive ) {
            whileCntrl = false;
          }
           
        } else {whileCntrl = true; }

      } while (whileCntrl); // set to check if last note was 5 seconds long, if so done!
      
      countDown(4, 0.25);  //info flash
      
      if (autoMode) {
        for (int i=0; i < 64; i++) {
          outputList[i] = autoOutputList[i]  = ToutputList[i];
          autoRecBtnTimeStart[i] = TautoRecBtnTimeStart[i];
          autoRecDuration[i] = TautoRecDuration[i];
        }
        outputListSize = autoOutputListSize = outputListStep-1;
      } else { //arpeggiator recording in !autoMode!!
        //BUG: there is an issue here .. something is breaking in this else loop causing an all out failure of the arpeg. eg countDown usnig areptListSize crashes :(
        //delay(1000);
        //countDown(outputListStep-1, 1); 
        for (int i=0; i < 64; i++) {
          arpegList[i] = ToutputList[i] - arpegRecordingRoot;
          arpegAutoRecBtnTimeStart[i] = TautoRecBtnTimeStart[i];
          arpegAutoRecDuration[i] = TautoRecDuration[i];
        }
        arpegListSize = outputListStep-1;
        arpeggiator = true;
        //arpegRecord = true;
        int tmpCurrentStep = currentStep;
        //currentStep = 4;
        delay(1000);
        //countDown(outputListStep-1, 1); 
        //delay(1000);
        countDown(arpegListSize,1); //BUG: this is causing it to flash on the first 4 steps!
        delay(1000);
        for (int i=0; i < arpegListSize; i++) {
          delay(500);
          currentStep = arpegRecordingRoot + arpegList[i];
          outputLeds();
        }
        
        //currentStep = tmpCurrentStep;
      }
      outputListStep = 0;
      zeroActive = false;

      /*
      // --------------------------------------------
      // arepeggiator assimilation
      // this should fix the ouputlist to give the steps between including all the arepeggiator steps
      if (arpeggiator && arpegRecord ) { // && autoBtnMode == 1
        int tList[64];
        //int outputListSize = 4;
        //int outputList[] = {1,3,5,4};
        int tListSize = outputListSize;
        int k=0;
        
        outputListSize = 0;
        for(int i=0; i < tListSize; i++ ){ tList[i] = outputList[i];}
        for(int i=0; i < tListSize; i++) {
          for (int j=0; j < arpegListSize; j++) {
            outputList[k] = tList[i] + arpegList[j];
            outputListSize++;
            k++;
          }
        }
        arpegRecord = false;
        //autoBtnMode = 1;
      }
      */
      
      //-----> autoBtnMode startup --> should be a function .. to use in places this is found
      //vvvv function basics?? including different options found in output management
      millisNow = millis(); 
      sequenceStepTimeStart = millisNow;
      sequenceStepTimeNext = sequenceStepTimeStart + (60L*1000)/BPM;//*1000;  //THIS NEED TO BE MODIFIED FOR TYPE OF autoBrnMode!!!
    }
  } 


  // --------------------------------------------
  // switch processing 
  // determining what all the different combinations of time and switches do within different modes.
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
        } else if (zeroActive && (!forwardActive || !reverseActive)) {
          if (!autoMode) {
            for (int i = 0; i < 8; i++) {
              digitalWrite(stepPins[i], LOW);
            }
            currentStep = 0;
          } else {
            if (autoBtnMode == 0) { autoBtnMode = 0; }
            else {
              if (autoBtnMode == 1) { autoBtnMode = 2; } 
              else if (autoBtnMode == 2) { autoBtnMode = 1; }
              countDown(1, 0.25);  //info flash
              //for (int i = 0; i < autoBtnMode; i++) {delay(100); countDown(1, 2); }
            }
          }
          
        } else if (forwardActive && !zeroActive) {
          directionNow = 1;
          if (!autoMode) {stepTriggered=true;}
        } else if (reverseActive && !zeroActive) {
          directionNow = -1;
          if (!autoMode) {stepTriggered=true;}
        }
        swiPressTime = millisNow;
      } 
    } 
    if (swiState == 1 ) {
      swiHoldDuration = millisNow - swiPressTime;
      if (zeroActive && (forwardActive || reverseActive)){
        direction = direction * -1; 
        zeroActive = false;
      }
      if (swiHoldDuration >= 2000) {
        if (resetActive) {
          if (autoMode) { // autoMode is reset and apreggiator is removed if it is active
            autoBtnMode = -1;
            for (int i = 0; i < 64; i++) {
              outputList[i] = 0;
            }
            // TODO: - then it has to be setup so automode is removed from output while arpeggiator is saved
            
          }
          else { // !autoMode = arpeggiator is reset
            arpeggiator = false;
            arpeggiatorStep = 0;
            //arpegRecord = false;
            /*for (int i = 0; i < 64; i++) {
              outputList[i] = autoOutputList[i] = 0;
            }
            */
            // TODO: - then it has to be done so arpeggiator is removed from the normal sequence whatever that is.
          }   
          
          
          /*for (int i = 0; i < 64; i++) {
            outputList[i] = 0;
          }
          */
          countDown(4, 0.25);  //info flash
        } 
      }
      

      // --------------------------------------------
      // switch processing 2
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
              //BPM += 1;
              if (autoBtnMode == 2) {timeFactor -= 1.0F/100; } else {BPM += 1; }
            } else {
              //BPM -= 1;
              if (autoBtnMode == 2) {timeFactor += 1.0F/100; } else {BPM -= 1; }
            }
            loopTriggerBPM = 1; //this makes it go forward in auto mode .. removed now as was anoying to test.
          } else if (loopTriggerBPM > 0 && swiHoldDuration >= 1000 + (loopTriggerBPM -1 ) * 25 ) {
            //change BPM based on time held ~20 for every 1 second
            if (direction == directionNow) {
              //BPM += 1;
              if (autoBtnMode == 2) {timeFactor -= 1.0F/100; } else {BPM += 1; }
            } else {
              //BPM -= 1;
              if (autoBtnMode == 2) {timeFactor += 1.0F/100; if (timeFactor < 1.0F/100){timeFactor =1.0F/100;}} else {BPM -= 1; }
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


  


  // --------------------------------------------
  // automodes
  //NOTE: below ot integrate?? //-----> autoBtnMode startup --> should be a function .. to use in places this is found
  // ouput assimilation
  // automode
  if (btnsPressed == 0 && autoMode && millisNow > sequenceStepTimeNext) { //if true then next step in automode has been surpassed so lets trigger a step
    sequenceStepTimeStart = millis();
    if(autoBtnMode <= 1 ) { //modes that follow BPM
      // tofix: this should be in the normal auto mode check aswell as the buttonmode check, but not only this one.
      if (arpeggiator) {
        sequenceStepTimeNext = sequenceStepTimeStart + (60L*1000)/(BPM*(arpegListSize));
        if (arpeggiatorStep = arpegListSize) {
          arpeggiatorStep=0;
          stepTriggered = true;
        }
        else {
          arpeggiatorStep++;
        }
      }//60/BPM*1000;
      else {
        sequenceStepTimeNext = sequenceStepTimeStart + (60L*1000)/BPM; 
        stepTriggered = true;
      }
      
    } else if (autoBtnMode == 2) { //mode that follows recorded input
      //here it will need to follow the lists provided to know when to start 
      unsigned long TautoRecDuration = 0;
      
      if (outputListStep + direction +1 > outputListSize ) { //forward direction - next step is after duration of current step
        TautoRecDuration = autoRecBtnTimeStart[1];//autoRecDuration[outputListStep];
      } else if ( outputListStep + direction < 0 ) { //reverse direction  
        TautoRecDuration = autoRecBtnTimeStart[outputListSize] - autoRecBtnTimeStart[outputListSize + direction];
      }
      else {
        if (direction == +1 ){
          TautoRecDuration = autoRecBtnTimeStart[outputListStep +1 + direction] - autoRecBtnTimeStart[outputListStep +1]; }//abs(autoRecBtnTimeStart[outputListStep + direction] - autoRecBtnTimeStart[outputListStep] ); 
        else {
          TautoRecDuration =  autoRecBtnTimeStart[outputListStep] - autoRecBtnTimeStart[outputListStep + direction]; 
        }
        
      }
      sequenceStepTimeNext = sequenceStepTimeStart + (timeFactor * (TautoRecDuration ) );
      stepTriggered = true;
    } 
  }


  // --------------------------------------------
  // triggered step processing
  //find next step if triggered via forward or reverse or via auotmode
  if (arpeggiator && !stepTriggered ) {
    //to do so it fixes arpeggiator output > 7 then 7, < 0 then 0
    currentStep = outputList[outputListStep+arpegList[arpeggiatorStep]];
    if (currentStep > 7) { currentStep = 7;}
    else if (currentStep < 0 ) {currentStep = 0; }
  } else if (stepTriggered == true) {
    //digitalWrite(stepPins[currentStep], LOW);
    //to fix so it uses ouputList[] && 7 = outputListStep
    stepTriggered = false;
    
    if (autoBtnMode >= 0 && outputListSize != 0) { //autoMode && 
      outputListStep += direction;
      if (outputListStep > outputListSize ) {
        outputListStep = 0;
      } else if (outputListStep < 0 ) {
        outputListStep = outputListSize;
      }
      currentStep = outputList[outputListStep];
    } else { // == autoBtnMode = 0
      currentStep += direction;
      if (currentStep > 7 ) {
        currentStep = 0;
      } else if (currentStep < 0 ) {
        currentStep = 7;
      }
    }
  }
  

  // --------------------------------------------
  // ouput
  outputLeds();
}
