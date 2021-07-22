#include "funshield.h"


constexpr size_t displayCount = 4;
constexpr size_t maximumThrows = 9;

constexpr size_t supportedDices[] = {4, 6, 8, 10, 12, 20, 100};
constexpr int dicesCount = sizeof(supportedDices) / sizeof(supportedDices[0]);
size_t positionOfCurrentDice = 1;
size_t currentDice = supportedDices[positionOfCurrentDice];

constexpr int ledPins[] { led1_pin, led2_pin, led3_pin, led4_pin };
constexpr int ledPinsCount = sizeof(ledPins) / sizeof(ledPins[0]);

constexpr int ledsPattern[] = {led1_pin, led2_pin, led3_pin, led4_pin, led3_pin, led2_pin};
constexpr int ledsInPattern = sizeof(ledsPattern) / sizeof(ledsPattern[0]);

unsigned long startMillis;
unsigned long startMillisForAnimation;
unsigned long startMillisProgram;
unsigned long currentMillis;
constexpr size_t animationDelay = 300;

constexpr byte segmentMap[] = {
  0xC0, // 0  0b11000000
  0xF9, // 1  0b11111001
  0xA4, // 2  0b10100100
  0xB0, // 3  0b10110000
  0x99, // 4  0b10011001
  0x92, // 5  0b10010010
  0x82, // 6  0b10000010
  0xF8, // 7  0b11111000
  0x80, // 8  0b10000000
  0x90,  // 9  0b10010000
  0b10100001,   // d
  0b11111111, //empty glyph
};

constexpr byte EMPTY_GLYPH = 0b11111111;
const size_t d = 10;

constexpr byte digitMap[] = {
  0x08,
  0x04,
  0x02,
  0x01,
};


struct Button {
  int pin;
  size_t timeWhenPressed;
  size_t howLongPressed;
  bool isPressed;
  bool wasPressed;

  Button(int button_pin){
    pin = button_pin;
    timeWhenPressed = 0;
    howLongPressed = 0;
    isPressed = false;
    wasPressed = false;
  }

  void setup(){
    pinMode(this->pin, INPUT);
  }
};

Button buttons[] = {
  Button(button1_pin),
  Button(button2_pin),
  Button(button3_pin),
};

size_t buttonsCount() {
  return sizeof(buttons) / sizeof(buttons[0]);
}

struct Display{

  size_t numberOfDigitsToDisplay; 
  size_t normalModeDisplay[displayCount];

  Display(){
    for (size_t i = 0; i < displayCount; i++){
      normalModeDisplay[i] = 0;
    }
  }

  void emptyDisplay(){
    for (size_t i = 0; i < displayCount; i++){
      write_glyph(EMPTY_GLYPH, digitMap[i]);
      normalModeDisplay[i] = 11; //empty glyph
    }
  }

  void write_glyph(byte glyph, byte position) {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, glyph);
    shiftOut(data_pin, clock_pin, MSBFIRST, position);
    digitalWrite(latch_pin, HIGH);
  }

  void write (size_t display[4], size_t currentNumberOfDigitsToDisplay){
    for (size_t i = 0; i < currentNumberOfDigitsToDisplay; i++){
      write_glyph(segmentMap[display[i]], digitMap[i]);
    }
  }


  // @tom: udelat tohle nejak lepe, co kdyby bylo digits milion?
  void throwToDisplay(size_t myThrow){
    numberOfDigitsToDisplay = log10(myThrow)+1;
    for(size_t i = 0; i < displayCount ; i++){
      normalModeDisplay[i] = myThrow % 10;
      myThrow /= 10;
    }      
  }  
};


struct ThrowHandler{

  size_t numberOfThrows;
  size_t sumOfThrows;
  size_t throwCounter;
  size_t currentThrow;

  ThrowHandler(){
    numberOfThrows = 1;
    sumOfThrows = 0;
    throwCounter = 0;
  }
  // @tom: misto komentare pojmenovat jako konstantu
  size_t configModeDisplay[displayCount] = {6, 0, d, 1}; 

  void setSumOfThrows(size_t a, size_t b){
    this->sumOfThrows = a + b;
  }

  void increaseThrowCounter(){
    this->throwCounter++;
  }

  void increaseNumberOfThrows(){
    numberOfThrows = ((numberOfThrows + 1));
    if (numberOfThrows > maximumThrows) numberOfThrows = 1;
    configModeDisplay[displayCount - 1] = numberOfThrows;
  }

  void changeCurrentDice(){
    positionOfCurrentDice = (positionOfCurrentDice + 1) % dicesCount;
    currentDice = supportedDices[positionOfCurrentDice];

    if (currentDice == 100){
      configModeDisplay[0] = configModeDisplay[1] = 0;
    }
    else{
      configModeDisplay[0] = currentDice % 10;
      configModeDisplay[1] = currentDice / 10;
    }
  }

  size_t generateRandomNumber(size_t time){ 
    return ((time + startMillisProgram) % currentDice) + 1;
  }
};

struct Mode{

  bool normalMode = true;
  bool configMode = false;
  bool animationMode = false;
  bool showResultMode = false;


  void setNormalMode(){
    normalMode = true;
    configMode = false;
    showResultMode = false;
    animationMode = false;
  }

  size_t setConfigMode(){
    normalMode = false;
    configMode = true;
    showResultMode = false;
    animationMode = false;
    return 0;
  }
};

struct GraphicHandler{

  int ledToTurnOn = 0;
  int ledToTurnOff = 1;
  
  void turnOnAllLeds(){
    for (size_t j = 0; j < ledPinsCount; j++){
      digitalWrite(ledsPattern[j], LOW);
    }
  }

  void turnOffAllLeds(){
    for (size_t j = 0; j < ledPinsCount; j++){
      digitalWrite(ledsPattern[j], HIGH);
    }
  }

  void showAnimation(){
    startMillisForAnimation = currentMillis;
    digitalWrite(ledsPattern[ledToTurnOn], LOW);
    digitalWrite(ledsPattern[ledToTurnOff], HIGH);
    ledToTurnOff = ledToTurnOn;
    ledToTurnOn = (ledToTurnOn + 1) % ledsInPattern;
  }

  bool startAThrow(Display display){
    startMillis = startMillisForAnimation = millis();
    display.emptyDisplay();
    return true;
  }
};

Display display = Display();
ThrowHandler throwHandler = ThrowHandler();
Mode mode = Mode();
GraphicHandler graphicHandler = GraphicHandler();


void finnishAThrow(){
  currentMillis = millis();
  throwHandler.currentThrow = throwHandler.generateRandomNumber(currentMillis - startMillis);
  throwHandler.setSumOfThrows(throwHandler.sumOfThrows, throwHandler.currentThrow); 
  throwHandler.increaseThrowCounter(); 

  if (throwHandler.throwCounter == throwHandler.configModeDisplay[3]){
    graphicHandler.turnOnAllLeds();
    display.emptyDisplay();   
    startMillisForAnimation = millis();
    mode.showResultMode = true;
  }
  else{     
    display.throwToDisplay(throwHandler.currentThrow);
    graphicHandler.turnOffAllLeds();
  }
  mode.animationMode = false;
}

void showResult(){
  currentMillis = millis();
  if (currentMillis - startMillisForAnimation >= animationDelay){        
    display.throwToDisplay(throwHandler.sumOfThrows);      
    graphicHandler.turnOffAllLeds();
    mode.showResultMode = false;
    throwHandler.sumOfThrows = 0;
    throwHandler.throwCounter = 0; 
  }
}

void setup() {
  for (size_t i = 0; i < ledPinsCount; ++i) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], OFF);
  }

  for(size_t i= 0; i < buttonsCount(); ++i) {
      buttons[i].setup();
  }
  pinMode(latch_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  Serial.begin(9600);
  display.emptyDisplay();

  startMillisProgram = millis();
}

void loop() {

  for (size_t i = 0; i < buttonsCount(); i++){
    if (digitalRead(buttons[i].pin) == LOW) {
      buttons[i].isPressed = true;
    }
    else {
      buttons[i].isPressed = false;
    }
  
    if (! buttons[i].isPressed) { 
      if (i == 0 && mode.normalMode && buttons[0].wasPressed){ 
        finnishAThrow();
      }

      else if (i == 0 && mode.configMode && buttons[0].wasPressed){
        display.emptyDisplay();
        mode.setNormalMode();
      }

      buttons[i].wasPressed = false;
    }
    
    else if (buttons[i].isPressed && ! buttons[i].wasPressed){
      if (i == 0){
        if (mode.normalMode){
          mode.animationMode = graphicHandler.startAThrow(display);      
        }
      }
      else {
        if (mode.normalMode){
         throwHandler.throwCounter = mode.setConfigMode(); 
        }
        else if (i == 1){
          throwHandler.increaseNumberOfThrows();
        }
        else {
          throwHandler.changeCurrentDice();
        }
      }
      buttons[i].wasPressed = true;
    }  
  }
  if (mode.showResultMode){
    showResult();
  }
  else if (mode.animationMode){
    currentMillis = millis();
    if (currentMillis - startMillisForAnimation >= animationDelay){
      graphicHandler.showAnimation();      
    }
  }
  else if (mode.normalMode) {
    display.write(display.normalModeDisplay, display.numberOfDigitsToDisplay);
  }
  else {display.write(throwHandler.configModeDisplay, displayCount); 
  }
}
