/*Ultrazvukove mereni vysky hladiny technologickych vod. v1.8
 * Blind Pew 2016 <blind.pew96@gmail.com>
 * GNU GPL v3
 */

#include <NewPing.h>
#include <Adafruit_NeoPixel.h>


//#define DEBUG
#define BRATE 9600 //rychlost seriove linky pro ladici informace
#define TRIG_PIN  9 //triger pin
#define ECHO_PIN  8 //echo pin
#define MAX_DIS 300 //maximalni vzdalenost od snimace
#define LED_PIN 10 //pin led pasku
#define LUX_PIN A2  //fotoodpor
#define LUX_PRAH  20
#define STATPIXEL 0
#define INTERVAL  5*60000 //interval zobrazeni trendu hladiny 5 minut
#define NUMPIXELS 111 //pocet pripojenych diod
#define TRI NUMPIXELS/3 //:-)
#define VYSKA 200 //vyska nadrze
#define OFFSET_LED 13 //offset led pasku
#define MAX_JAS 70 //maximalni svitivost
#define MAX_JAS_STAT 90 //max svitivost status led
#define GREENSEN  40
#define BLUESEN   50
#define POCET_MERENI 50
#define DELAY_MERENI  100
#define OFFSET_MERENI 2
#define MAXCOUNT_LD 8

unsigned long millis_last = 0;
const long interval = INTERVAL;
long vzd_cm = 0;
long raw_hladina = 0;
int hladina = 0; //vyska hladiny v cm
int hladina_m = 0;
int red = 0;
int green = 0;
int blue = 0;
int pix = 0; //pixel urcujici vysku hladiny
bool dot = true; //bodovy rezim
byte count_ld = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DIS);

//---------------------startup test----------------------------
//-trikolora
void start_test() {
  for(int i=0;i<TRI;i++) {
    pixels.setPixelColor(i, pixels.Color(MAX_JAS,MAX_JAS,MAX_JAS));
  }
  for(int i=TRI;i<2*TRI;i++) {
    pixels.setPixelColor(i, pixels.Color(0,0,MAX_JAS));
  }
  for(int i=2*TRI;i<NUMPIXELS;i++) {
    pixels.setPixelColor(i, pixels.Color(MAX_JAS,0,0));
  }
  pixels.show();
}

//---------------zhasne vsechny ledky------------
void vse_vypni() {
  for(int i=STATPIXEL+1;i<NUMPIXELS;i++) {
    pixels.setPixelColor(i, pixels.Color(0,0,0));
  }
}

//-------------trend hladiny-------------------
void trend(int akt, int minula) {
  if(akt > minula) {
    if(akt-minula > OFFSET_MERENI) {
      //hladina stoupa
      pixels.setPixelColor(STATPIXEL, pixels.Color(0,MAX_JAS_STAT,0));
      hladina_m = hladina;
      count_ld = 0;
    }
  }
  if(akt < minula) {
    if(minula-akt > OFFSET_MERENI) {
      //hladina klesa
      pixels.setPixelColor(STATPIXEL, pixels.Color(MAX_JAS_STAT,MAX_JAS_STAT-GREENSEN,0));
      hladina_m = hladina;
      count_ld++;
    }
  }
  if(akt == minula) {
    //nic se nedeje
    pixels.setPixelColor(STATPIXEL, pixels.Color(MAX_JAS,MAX_JAS,MAX_JAS));
  }
  pixels.show();
}


void setup() {
	Serial.begin(BRATE);
	pixels.begin();
	start_test();
	delay(3000);
}
void loop()
{//hladina klesa moc dlouho spustim varovani
  if(count_ld > MAXCOUNT_LD) {
    pixels.setPixelColor(STATPIXEL, pixels.Color(0, 0, 0));
    pixels.show();
  }
  //hlavni mereni
	for(int x=0; x<POCET_MERENI;x++) {
	  vzd_cm = sonar.ping_cm();
    #ifdef DEBUG
      Serial.println(VYSKA - vzd_cm);
    #endif
    raw_hladina = raw_hladina + (VYSKA - vzd_cm);
	  delay(DELAY_MERENI);
	}
  //vypocet
  hladina = raw_hladina / POCET_MERENI;
  raw_hladina = 0;
  #ifdef DEBUG
    Serial.println("-------------");
    Serial.print(hladina);
    Serial.println(" cm");
    Serial.println("");
  #endif

  if(millis() < millis_last) {//preteceni
    millis_last = millis();
  }
  if(millis() - millis_last >= interval) { //urceni trendu hladiny
    trend(hladina, hladina_m);
    millis_last = millis();
  }

  //------------vyber barvy podle vysky hladiny--------------
  if (hladina < 100) {
    red = map(hladina, 0, 100, MAX_JAS, 0);
    green = map(hladina,100, 0, MAX_JAS-GREENSEN, 0);
    blue = 0;
  }
  else {
    red = 0;
    green = map(hladina,100, 200, MAX_JAS-GREENSEN, 0);
    blue = map(hladina, 200, 100, MAX_JAS-BLUESEN, 0);
  }
  pix = map(hladina, 0, VYSKA-OFFSET_LED, NUMPIXELS, 0);

  //zhasne po hladinu
  for(int i=STATPIXEL+1;i<pix;i++) {
    pixels.setPixelColor(i, pixels.Color(0,0,0));
  }

  if(dot) { //pri bodovem rezimu vse zhasni, rozni jednu led
    vse_vypni();
    pixels.setPixelColor(pix, pixels.Color(red,green,blue));
    pixels.show();
  }
  else { //rozne vse hladina -> dno
    for(int i=pix;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(red,green,blue));
    }
  }
  pixels.show();
  #ifdef DEBUG
    Serial.print("Svetelne cislo: ");
    Serial.println(analogRead(LUX_PIN));
    Serial.println("===========================Konec=============================");
    Serial.println("");
  #endif
  if(analogRead(LUX_PIN) < LUX_PRAH) { //je tma setri led
    dot = true;
  }
  else {
    dot = false;
  }

  if(count_ld > MAXCOUNT_LD) {
    pixels.setPixelColor(STATPIXEL, pixels.Color(MAX_JAS_STAT, 0, 0));
    pixels.show();
  }
  delay(1000);
}
