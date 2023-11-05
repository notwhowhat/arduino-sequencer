![alt text](https://github.com/notwhowhat/arduino-sequencer/blob/main/collaterals/sequecer%208bit%20arduino%20atmega3281.png)
arduino 8 bit sequencer based originally on lookmumnocomputers design and build instructions here  https://www.lookmumnocomputer.com/sequencer-keyboard/
improvements by JakobG and PG using atmega328 base chip and includes the following main changes:
complete new code with millis-ification to allow for some spify autoMode sequencing!! in 3 autoModes!

functions:
while very base functionality is accessible directly via signals in /out there are more advanced faetures available via pressing buttons, switches or turning the pots.. but there are a number of special features as well 
  base mode (when unit is powered ) - will run lights across in forward direction.
  push buttons - step 1 (0) on left and step 8 (7) on the right. Pressing these allows 12v signal adjusted by pots in two signals to carry on and out in to individual steps or to keyboard out. 
  pots - turned to get right notes into the different CVs
  forward / reverse - moves step to next step forward or reverse (in direction given, unless in autoMode recorded autoBtnModes 1 or 2, then it carries on in the direction recorded or opposite (reverse))
  default - if held in either direction for > 1s it will go into automode. holding during automode will speed up or slow down depending on direction. one momentary hold will give +/- 1BPM, holding it >1s will then increase BPM considerably more quickly (10BPM/s)
  automode - will proceed through the steps one at a time, proceeding from eventual button presses. >1 button pressed > 5s = autoBtnMode 0. 1 button pressed > 5s = autoBtnMode 1, gives a BPM related countdown which allows up to 64 steps of recording! this is exited by either pressing a button or not pressing anything for >5s
  reset - momentary = exits automode and puts unit hidden step = 0 (-1). held > 2s = as well as momentary functions it clears all modes to base mode (saving BPM and which autoBtnMode) 
  zero - momenttary = sequence goes back to zero. held > 2s = switches autoBtnMode btween 1 and 2 which is singnaled by the same number of long flashes after some quick flashes.
  autoBtnMode 0 - proceed through all steps according to BPM, pressing a button will result in the sequence proceedign from the butotn pressed
  autoBtnMode 1 - proceeds through sequence recorded according to BPM (default when single button pressing), pressing a button impacts the sequence
  autoBtnMode 2 - proceeds through sequence recorded according to when buttons were pressed in time, pressing a button impacts the sequence

electonics notes
  serial interface - used to program the unit and is connected with button facing you and the mini usb down the dupont connectors with metal up are inserted to the main connector from left to right ground(black)-space-power(red)-yellow-orange-white. 
  serial interface switch - connected according to the colors yellow to yellow, white to white and orange to orange.

TODO
? - adjust speed of autoBtnMode 2 (recording with spaces) with forward and reverse
? - fix autoBtnMode 2 to include dead space - autoBtnMode 3??
? - remove dead code
? - organize variables a bit better
? - improve comments for build for fixing later (eg how to connect serial interface (with botton on top dupont pins show .. power / gnd to left 3 pins and others to the right) and quick button (needed to swtich beteween program and funcitonal mode quickly = use colors))
? - +/-12v input test .. today running directly off of 5v source voltage via USB red(power) and black(ground) (NOT coming from ardunio with a space between) should allow for 6-12v function.

