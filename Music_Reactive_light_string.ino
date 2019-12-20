                        
#define AUDIO_ENVELOPE_PIN 0

// control options
bool autoThreshold = true;

int ledPin[] = {3,5,6};    // LED string connected to PWM pins

// array of recent readings for rolling average, max and min
const PROGMEM byte historyFreqency = 10; // reading frequency in hz
const PROGMEM byte historyLength = 50; //  number of readings to store in history
byte historyIndex = 0; // index of last reading
uint16_t history[historyLength]; // array to store reading history

// global color states
uint32_t stripColor[] = {255, 255, 255};
uint32_t startTime;

void setup()  { 
  // set output mode for led pins
  for (int i=0; i<3; i++)
    pinMode(ledPin[i], OUTPUT);

  randomSeed(analogRead(A0)); // initialize sudo random sequence with a random seed
  
  // initalize history window to 0
  for (int i = 0; i < historyLength; i++)
    history[i] = 0;
  
  // setup timer1 interupt for updating history array
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 62500 / historyFreqency;            // compare match register 16MHz/256/ Interupt Freq. in Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
} 

void loop()  { 
//  int sound = analogRead(AUDIO_ENVELOPE_PIN);
//  int brightness = map(sound, 0, 1024, 0, 255);
//  for (int i=0; i<3; i++)
//    analogWrite(ledPin[i], brightness);   

  fade();
    
}

// rolling average
int rollingAverage() {
  int sum = 0;
  for (int i = 0; i < historyLength; i++)
    sum += history[i];
  return (sum / historyLength);
}

// rolling max
int rollingMax() {
  int maxValue = 0;
  for (int i = 0; i < historyLength; i++)
    if (history[i] > maxValue)
      maxValue = history[i];
  return maxValue;
}

// rolling min
int rollingMin() {
  int minValue = 0;
  for (int i = 0; i < historyLength; i++)
    if (history[i] < minValue)
      minValue = history[i];
  return minValue;
}

// Interrupt called 10 times a second to update the history array
SIGNAL(TIMER1_COMPA_vect) 
{
  history[historyIndex] = analogRead(AUDIO_ENVELOPE_PIN);
  historyIndex = (historyIndex + 1) % historyLength;
}


//Flashing lights.
void flash() {
  int reading, threshold, time, cycles;
  byte colors[] = { random(256), random(256), random(256) };
  cycles = map(controlValue(5), 0, 255, 2, 124); // set initial cycles value
  
  for (int i=0; i < 3; i++) { //turn all colors off
    analogWrite(ledPin[i], 0);
  }
  
  for (int j=0; j<cycles; j++) {  //do x cycles of flashing
    // update threshold and decay time values
    if (autoThreshold) {
      byte sens = map(controlValue(2), 0, 255, -96, 96);
      int value = constrain((((rollingMax() + rollingAverage()) / 2) + sens), 0, 1023);
      threshold = map(value, 0, 1023, 5, 250);
    }
    else threshold = map(controlValue(2), 0, 255, 5, 250);
    time = map(controlValue(4), 0, 255, 5, 500);
    
    while (controlValue(AUDIO_ENVELOPE_PIN) < threshold && j < cycles) // wait till it reaches threshold
      cycles = map(controlValue(1), 0, 255, 3, 90); // update cycles value from controls
    
    for (int i=0; i < 3; i++) { //set color on
      analogWrite(ledPin[i], colors[i]);
    }
    
    while (controlValue(AUDIO_ENVELOPE_PIN) < threshold && j < cycles) // wait till it drops bellow threshold
      cycles = map(controlValue(1), 0, 255, 3, 90); // update cycles value from controls
    
    delay(time); // decay time
    
    for (int i=0; i < 3; i++) { //turn all colors off
      analogWrite(ledPin[i], 0);
    }
    
  }
}


// fade lights to sound
void fade() {
  byte brightness;
  uint8_t delayTime, brightMod, level;
  uint16_t threshold, colorTime;
  int cMod[3];
  
  // update color time
  colorTime = map(controlValue(1), 0, 255, 500, 10000);
  
  // random color as ratio
  // if color time has passed get new random color
  if ((millis() - startTime) >= colorTime) {
    int ratio[3];
    byte brightest;
    brightest = 0;
    // generate new random color ratio
    for (int i = 0; i < 3; i++) {
     ratio[i] = random(65);
     if (ratio[i] > brightest)
       brightest = ratio[i];
    }
    // Scale ratios to max brightness and Convert ratio to color
    for (int j = 0; j < 3; j++) {
      ratio[j] = ratio[j] * (64 / brightest);
      stripColor[j] = min((ratio[j] * 4) - 1, 255);
    }
    // update startTime
    startTime = millis();
  }

  // update threshold, delay time and color time values
  if (autoThreshold) {
    byte sens = map(controlValue(3), 0, 255, -96, 96);
    threshold = map(constrain((rollingMax() + sens), 32, 1023), 32, 1023, 64, 255);
  }
  else threshold = map(controlValue(3), 0, 255, 32, 255);
  delayTime = map(controlValue(4), 0, 255, 0, 70);
  colorTime = map(controlValue(5), 0, 255, 500, 10000);
  
  level = constrain(controlValue(AUDIO_ENVELOPE_PIN), 0, threshold);
  brightMod = map(level, 0, threshold, 0, 1000); // map amplitude to brightness
  
  for (int j=0; j<3; j++) // modulate color with brightMod
    cMod[j] = stripColor[j] * brightMod * 0.001;

  // write to rgb led channels
  for (int i=0; i < 3; i++) // write modulated color to leds
    analogWrite(ledPin[i], cMod[i]); 

  // write to single channels
  analogWrite(9, level);
  //analogWrite(10, level);
  
  delay(delayTime);
    
}

int controlValue(uint8_t channel) {
  return analogRead(channel) / 4;
  // controlValues[channel] = analogRead(channel) / 4;
  // return controlValues[channel];
}

