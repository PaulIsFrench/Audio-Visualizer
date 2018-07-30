#include <Adafruit_NeoPixel.h> //https://github.com/adafruit/Adafruit_NeoPixel
#include "visualizer.h"

////Declare Spectrum Shield pin connections
#define STROBE           4
#define RESET            6
#define DC_ONE          A1
#define DC_TWO          A2
#define SENSOR_PIN      A3
#define STRIP_PIN        3 // STRIP
#define N_STRIP_LEDS    30
#define LEVELS          16



//SETTINGS
#define BRIGHTNESS  255       //(0 to 255)
#define RISE_RATE     0.13    //(0 to 1) higher values mean livelier display
#define FALL_RATE     0.04    //(0 to 1) higher values mean livelier display
#define CONTRAST      1.3     //(undefined range)
#define MAX_VOL       600       //(0 to 1023) maximum value reading expected from the shield
#define LIVELINESS    2       //(undefined range)
#define LIVELINESS_T  1.7     //(undefined range)
#define LIVELINESS_T2 2.0
#define MULTIPLIER    90       //(undefined range) higher values mean fewer LEDs lit @ a given volume

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(N_STRIP_LEDS , STRIP_PIN, NEO_GRB + NEO_KHZ800);
//Define LED connections on the Arduino/Shield
int LED[] = {7, 8, 9, 10, 11, 12, 13};

//Define spectrum variables
int freq_amp;
int StereoLeft[7];
int StereoRight[7]; 
int Mono[7];
int i;

// READ
void recordSoundSensor(int (&frequencies) [7]);
void recordFrequencies(int (&frequencies) [7]);
// records frequencies into the frequencies array
double getAvg(const int (&frequencies) [7]);
// returns avg of frequencies

//FILTER
double smoothVol(double newVol);
// brings vol values closer together, resulting a smoother output (avoid strobing)
double autoMap(const double vol);
// scales volume values in order to get consisitent output at different overall volumes

//DISPLAY
uint32_t GetColor(byte pos, double vol);
// finds the new color value based on color rotation position and volume
void displayStrip(const double vol, Adafruit_NeoPixel &strip);
// detemines what LEDs are on/off; sends RGB values to the strip

//HELPER FUNCTIONS
int findLED(int level, int place);
//returns the index of any LED on the tower given its level and position on the level
void bluetoothInput();

//GLOBALS (globals are normally a bad idea, but the nature of Arduino's loop()
//         function makes them necessary)
circularArray<double> vols = circularArray<double>(100);          // last 100 volumes recorded
circularArray<uint32_t> color = circularArray<uint32_t>(100);       // last 100 color values generated
int numLoops = 0;           // number of loops executed so far
double redMult = 1.0;       // changed by BT
double grnMult = 1.0;       // changed by BT
double bluMult = 1.0;       // changed by BT
double brtMult = 1.0;
bool grnOn = true;
bool bluOn = true;
bool redOn = true;
bool MSGEQ7On = true;
double eqMults [7];

///********************Setup Loop*************************/
//Set default values for arrays
void setup() {
  for (int i = 0; i < 100; i++)
  {
    vols.addToFront(100);   //if set to 0, display would flash on. this creates a slow opening
    color.addToFront(strip1.Color(0, 0, 0));
  }

  for (int i = 0; i<7; i++)
  {
    eqMults[i] = 1.0;
  }

  Serial.begin(9600);

  
  //Set spectrum Shield pin configurations
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_ONE, INPUT);
  pinMode(DC_TWO, INPUT);
  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);
  
//  Initialize Strip
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, HIGH);
  delay(1);
  digitalWrite(STROBE, HIGH);
  delay(1);
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, LOW);

//  //Initialize Spectrum Analyzers
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, HIGH);
  delay(1);
  digitalWrite(STROBE, HIGH);
  delay(1);
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, LOW);
}
//
//
///**************************Main Function Loop*****************************/
void loop() {
  int frequencies[7];
  double volume;
  static uint16_t pos = 0;


    
  //
  // READ
  //

  if (MSGEQ7On)
    recordFrequencies(frequencies);
  else
    recordSoundSensor(frequencies);
    
  volume = getAvg(frequencies);

  //
  // FILTER
  //
  volume = smoothVol(volume);
  volume = autoMap(volume);

  //
  // DISPLAY
  //
  color.addToFront(GetColor(pos & 255, volume));

  //rotate the color assignment wheel
  int rotation = map (volume, 0, strip1.numPixels() / 2.0, 0, 25);
  numLoops++;
  if (numLoops % 5 == 0) //default 5
    pos += rotation;


  displayStrip(volume, strip1);

  
 
}
//
//


///*******************Serial Plotter*********************************************************************************************************************************************************************
void recordSoundSensor(int (&frequencies) [7])
{
  int total = 0;
  const int numAvgs = 30;
  int i;

  for(i=0; i < numAvgs; i++) //take 30 readings to smooth noise
  {
      total+= analogRead(SENSOR_PIN);
  }
  double avg = ((double)total/numAvgs);
  Serial.println(avg);
  
  int band;
  for (band = 0; band < 7; band++)
  {
    frequencies[band] = avg*100;

    //if (frequencies[band] < 70) frequencies[band] = 0;

  }
}

void recordFrequencies(int (&frequencies) [7])
{
  int band;
  for (band = 0; band < 7; band++)
  {
    frequencies[band] = ((analogRead(DC_ONE) + analogRead(DC_TWO)) / 2);

    if (frequencies[band] < 70) frequencies[band] = 0;

    //account for noise
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);
  }
}

double getAvg(const int (&frequencies)[7])
{
  int total = 0;

  for (int k = 0; k < 7; k++)
  {
    total += eqMults[k] * frequencies[k];
  }

  return ((double)total / 7);
}

double smoothVol(double newVol)
{
  double oldVol = vols[0];

  if (oldVol < newVol)
  {
    newVol = (newVol * RISE_RATE) + (oldVol * (1 - RISE_RATE));
    // limit how quickly volume can rise from the last value

    return newVol;
  }

  else
  {
    newVol = (newVol * FALL_RATE) + (oldVol * (1 - FALL_RATE));
    // limit how quickly volume can fall from the last value

    return newVol;
  }

}

double autoMap(const double vol)
{
  double total = 0;
  static double avg;
  static int prevMillis = 0;

  // this if statement acts like a delay, but without putting the entire sketch on hold
  if (millis() - prevMillis > 100)
  {
    for (int i = 100; i > 0; i--)
    {
      total += vols[i - 1];
    }
    vols.addToFront(vol);

    avg = total / 100.0;

    prevMillis = millis();
  }

  if (avg < 5) return 0;
  return map (vol, 0, 3 * avg + 1, 0, strip1.numPixels() / 1.0);
}

uint32_t GetColor(byte pos, double vol) //returns color & brightness
{
  double myRedMult = redMult * brtMult;
  double myGrnMult = grnMult * brtMult;
  double myBluMult = bluMult * brtMult;
  
  pos = 255 - pos;

  // some last-minute transforms I dont want to stick in the main fcn
  vol = pow(vol, CONTRAST);
  vol = map(vol, 0, MAX_VOL, 0, 255);

  if (vol > 255) vol = 255;

  // All 3 colors on
  if (redOn && grnOn && bluOn)
  {
    if (pos < 85) {
      int red = 255 - pos * 3;
      int grn = 0;
      int blu = pos * 3;
      return strip1.Color(vol * red * myRedMult / 255, vol * grn * myGrnMult / 255, vol * blu * myBluMult / 255);
    }
    if (pos < 170) {
      pos -= 85;
      int red = 0;
      int grn = pos * 3;
      int blu = 255 - pos * 3;
      return strip1.Color(vol * red * myRedMult / 255, vol * grn * myGrnMult / 255, vol * blu * myBluMult / 255);
    }
    pos -= 170;
    int red = pos * 3;
    int grn = 255 - pos * 3;
    int blu = 0;
    return strip1.Color(vol * red * myRedMult / 255, vol * grn * myGrnMult / 255, vol * blu * myBluMult / 255);
  }

  // Two colors on
  else if ((redOn && grnOn) || (grnOn && bluOn) || (redOn && bluOn))
  {
    int color1 = 0;
    int color2 = 0;
    if (pos < 127) {
      color1 = 255 - pos * 2;
      color2 = pos * 2;
    }
    else {
      pos -= 127;
      color1 = pos * 2;
      color2 = 255 - pos * 2;
    }

    if (redOn && grnOn)
    {
      return strip1.Color(vol * color1 * myRedMult / 255, vol * color2 * myGrnMult / 255, 0);
    }
    else if (grnOn && bluOn)
    {
      return strip1.Color(0, vol * color1 * myGrnMult / 255, vol * color2 * myBluMult / 255);
    }
    else if (redOn && bluOn)
    {
      return strip1.Color(vol * color1 * myRedMult / 255, 0, vol * color2 * myBluMult / 255);
    }
  }

  // Only one color on
  else if (redOn)
  {
    return strip1.Color(vol * 255 * myRedMult / 255, 0, 0);
  }
  else if (grnOn)
  {
    return strip1.Color(0, vol * 255 * myGrnMult / 255, 0);
  }
  else if (bluOn)
  {
    return strip1.Color(0, 0, vol * 255 * myBluMult / 255);
  }

  // All colors off
  else
    return strip1.Color(0, 0, 0);
}

void displayStrip(const double vol, Adafruit_NeoPixel &strip)
{
  uint32_t off = strip1.Color(0, 0, 0);

  // some last-minute transforms I dont want to stick in the main fcn
  double newVol = 0.5 * vol;
  newVol = pow(newVol, LIVELINESS);

  int i;
  for (i = 0; i < strip.numPixels() / 2; i++)
  {
    // some magic numbers here that made the output better. should eventually get rid of them
    int threshold = (int)((i + 1) * MULTIPLIER - (3 * newVol) + 5);

    if (newVol > threshold)
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 - i, color[i]);
    }
    else
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 - i, off);
    }

    if (newVol > threshold)
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 + i, color[i]);
    }
    else
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 + i, off);
    }
  }
  strip.show();
}

