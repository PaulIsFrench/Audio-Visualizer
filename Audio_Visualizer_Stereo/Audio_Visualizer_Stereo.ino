#include <Adafruit_NeoPixel.h>

//********************Spectrum Shield pin connections************************************************************************************************************************************************************************
#define STROBE 4
#define RESET 5
#define DC_One A0
#define DC_Two A1 
#define PIN 3
#define LEDNUM 30
#define BAND_NUM 7
#define OFFTHRESH  0.000
#define LOWTHRESH  0.125
#define MIDTHRESH  0.500
#define HIGHTHRESH 1.000
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDNUM , PIN, NEO_GRB + NEO_KHZ800);

//********************Define spectrum variables******************************************************************************************************************************************************************************
int freq_band;
int StereoLeft[BAND_NUM];
int StereoRight[BAND_NUM]; 
int CENTER = LEDNUM/2;
//              R     O    Y    G    B    P    W 
int Red[]     ={255, 255, 255,   0,   0, 127, 255};
int Green[]   ={  0,  64, 255, 255,   0,   0, 255};
int Blue[]    ={  0,   0,   0,   0, 255, 255, 255};
//********************Threshold_values***************************************************************************************************************************************************************************************
int lowcut[]  ={150, 200, 150, 175, 200, 200, 150};
int midcut[]  ={200, 250, 200, 200, 300, 225, 175};
int highcut[] ={250, 500, 250, 300, 500, 275, 200};
int Mono[BAND_NUM];
int i;

//********************Setup Loop********************************************************************************************************************************************************************************************

void setup() {
  
//********************Set spectrum Shield pin configurations*****************************************************************************************************************************************************************
  
  Serial.begin (9600);
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_One, INPUT);
  pinMode(DC_Two, INPUT);  
  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);
  
//********************Initialize Strip***************************************************************************************************************************************************************************************+
  
  strip.begin();
  strip.setPixelColor(0, strip.Color(0,0,0));
  strip.show();
  
//********************Initialize Spectrum Analyzers**************************************************************************************************************************************************************************
  
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

//********************Main Function Loop************************************************************************************************************************************************************************************

void loop() {  
  Read_Frequencies();
  Graph_Frequencies();
  Display_lights();
}

//********************Pull frquencies from Spectrum Shield*******************************************************************************************************************************************************************

void Read_Frequencies(){
  
//********************Read frequencies for each band*************************************************************************************************************************************************************************
  
  for (freq_band = 0; freq_band<BAND_NUM; freq_band++)
  {
    StereoLeft[freq_band] = analogRead(DC_One);
    StereoRight[freq_band] = analogRead(DC_Two); 
    Mono[freq_band] = (StereoLeft[freq_band]+ StereoRight[freq_band])/2; 
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);
  }
}

//********************Serial Plotter*****************************************************************************************************************************************************************************************

void Graph_Frequencies(){
 Serial.print(500);
  Serial.print(" ");
 Serial.print(0);
  Serial.print(" ");
  
//********************Choose Which Frequency to Graph************************************************************************************************************************************************************************

// Serial.println(Mono[0]);
// Serial.println(Mono[1]);
// Serial.println(Mono[2]);
// Serial.println(Mono[3]);
// Serial.println(Mono[4]);
// Serial.println(Mono[5]);
// Serial.println(Mono[6]);
}

//********************Sets left channel color********************************************************************************************************************************************************************************
uint32_t Stereo_Left_color(int i){
  double threshold = HIGHTHRESH;
  if (StereoLeft[i]<lowcut[i]){
   threshold = OFFTHRESH ;
  }
  if (StereoLeft[i]>=lowcut[i] && StereoLeft[i]<midcut[i]){
    threshold = LOWTHRESH ;
  }
  if (StereoLeft[i]>=midcut[i] && StereoLeft[i]<highcut[i]){
    threshold = MIDTHRESH;
  }
  if (StereoLeft[i]>=highcut[i]){
    threshold = HIGHTHRESH;
  }
 return strip.Color( Red[i]*threshold, Green[i]*threshold, Blue[i]*threshold);
}

//********************Sets right channel color******************************************************************************************************************************************************************************
uint32_t Stereo_Right_color(int i){ 
  double threshold = HIGHTHRESH;
  if (StereoRight[i]<lowcut[i]){
   threshold = OFFTHRESH ;
  }
  if (StereoRight[i]>=lowcut[i] && StereoRight[i]<midcut[i]){
    threshold = LOWTHRESH ;
  }
  if (StereoRight[i]>=midcut[i] && StereoRight[i]<highcut[i]){
    threshold = MIDTHRESH;
  }
  if (StereoRight[i]>=highcut[i]){
    threshold = HIGHTHRESH;
  }
 return strip.Color( Red[i]*threshold, Green[i]*threshold, Blue[i]*threshold);
}


//********************Creating Threshhold for Each color********************************************************************************************************************************************************************
void Display_lights(){ 
  for(int i=0;i<BAND_NUM;i++){
    uint32_t l_color = Stereo_Left_color(i);
    uint32_t r_color = Stereo_Right_color(i);
 
//********************Left***************************************************************************************************************************************************************************************************

    if (i == 0){
      strip.setPixelColor(CENTER-i*2 , l_color );
    }
    strip.setPixelColor(CENTER+i*2+1 , l_color );
    strip.setPixelColor(CENTER+i*2+2 , l_color );

//********************Right**************************************************************************************************************************************************************************************************

    if (i == 0){
      strip.setPixelColor(CENTER-i*2-1 , r_color );
    }
    strip.setPixelColor(CENTER-i*2-2 , r_color );
    strip.setPixelColor(CENTER-i*2-3 , r_color );
    strip.show ();
  }
  delay(10);
}
