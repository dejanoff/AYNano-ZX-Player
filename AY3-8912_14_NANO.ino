#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#define SS 10
#define I2C_ADDRESS 0x3C

/* BUTTONS VALUES */
#define OK 300 //from this
#define OK2 490 //to this

#define NEXT 100 //from this
#define NEXT2 250 //to this

#define PREV 500 //from this
#define PREV2 750 //to this
#define NOBUT 1000 //less then is button pressed default is 1000 or 10

bool keycodes=false; //show button codes
String psgname="LOADING.";
SSD1306AsciiWire oled;
File root;
byte volA=0;
byte volB=0;
byte volC=0;
short int fileCnt = 0;
short int dirCnt = 0;
short int sel = 0;
short int curdir = 0;
bool randomize=true;
bool ft=true;
void resetAY(){
  pinMode(A0, OUTPUT); // D0
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT); // D3
  pinMode(A6, INPUT); // A6                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
  
  pinMode(4, OUTPUT); // D4
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT); // D7
  
  pinMode(8, OUTPUT);  // BC1
  pinMode(9, OUTPUT);  // BDIR
  
  digitalWrite(8,LOW);
  digitalWrite(9,LOW);
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  
  for (byte i=0;i<16;i++) { ay_out(i,0); }
}


void setupAYclock(){
  pinMode(3, OUTPUT);
  TCCR2A = 0x23;
  TCCR2B = 0x09;
  OCR2A = 8;
  OCR2B = 3; 
}
void setup() {
  //Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();
  oled.set1X();
  oled.setCursor(0, 0);
  byte j=0;
  for(byte i=0x2c; i<=0x7f; i++) {
  j++;
oled.print(char(i)); if (j>=21) {j=0; oled.print("\n");}

  }
  while(analogRead(A6)<NOBUT) {}
  delay(50);
  oled.clear();
  oled.set1X();
  oled.println("AY Play 1.5");
  delay(50);
  oled.println("by Pavel Bandaletov");
  delay(50);
  if(analogRead(A6)<NOBUT) {keycodes=true;}
oled.print("SD INIT: ");
int cnt=0;
while (!initFile()) {
cnt++;
if(cnt==3000) { oled.print("ERROR"); cnt=0;}
}
oled.println("OK"); delay(50);
oled.print("DIRS: ");delay(50);
randomSeed(analogRead(4)+analogRead(5));
File dir=SD.open("/");
  dir.rewindDirectory();
  while (true) {

    File entr =  dir.openNextFile();
    if (!entr)  { 
      break; 
      }
    if (entr.isDirectory()) {
      dirCnt++;
    }
    entr.close();
  }
  dir.close(); 
  oled.println(dirCnt);
  delay (1000);  
  setupAYclock();
  resetAY();
  setupTimer();
  oled.clear();
}

void setupTimer(){
  cli();
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 1250;

  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

void ay_out(unsigned char port, unsigned char data){
if(port==8) {if(data<16) { volA=(volA+data)/2; } else {volA=16;}}
  if(port==9) {if(data<16) { volB=(volB+data)/2; } else {volB=16;} }
  if(port==10) {if(data<16) { volC=(volC+data)/2; } else {volC=16; }}
 
  PORTB = PORTB & B11111100;

  PORTC = port & B00001111;
  PORTD = PORTD & B00001111;

  PORTB = PORTB | B00000011;
  delayMicroseconds(1);
  PORTB = PORTB & B11111100;

  PORTC = data & B00001111;
  PORTD = (PORTD & B00001111) | (data & B11110000);

  PORTB = PORTB | B00000010;
  delayMicroseconds(1);
  PORTB = PORTB & B11111100;
}

unsigned int playPos = 0;
unsigned int fillPos = 0;
const byte bufSize = 255;
byte proc=0;
byte playBuf[bufSize]; // 31 bytes per frame max, 50*31 = 1550 per sec, 155 per 0.1 sec

File fp;
boolean playFinished = false;
boolean showname = false;
byte cy=0;
unsigned long fsize=100;
unsigned long total=100;
void loop() {
  fillBuffer(); 
  oled.set2X(); 
  oled.setCursor(0, 0);
  
  if(cy==15) {oled.print("\\");  if(showname==false) {proc=(100-ceil(100*fsize/total)); if(proc>100) {proc=100;} oled.set1X(); 
  //oled.setCursor(0, 3); for(byte i=0;i<16;i++) {oled.print("_");}  
  for(float i=0;i<=(0.82*proc);i++) {oled.setCursor(i, 3); oled.print(char(0x7F));}
  oled.print(" ");oled.print(proc);oled.print("%");
  }
  }
  if(cy==10 ) {oled.print("-");  }
  if(cy==5 ) {oled.print("/"); }
  
  if(cy==1 ) { oled.print("|"); if(randomize==true) {  oled.set1X();oled.setCursor(122, 3); oled.print("R");} else { oled.setCursor(122, 3); oled.print(" ");} } 
    
  oled.set1X();
  if(cy>0) {
  
  oled.setCursor(96, 0);
  for(byte t=0; t<=1.6*(17-volA); t+=4) {oled.print("|"); }
  for(byte t=1.6*(volA); t>0; t--) { oled.setCursor(126-t, 0); oled.print(char(0x7F)); }
  oled.setCursor(126, 0); oled.print("[");  
  
  
  oled.setCursor(96, 1);
  for(byte t=0; t<=1.6*(17-volB); t+=4) {oled.print("|"); }
  for(byte t=1.6*(volB); t>0; t--) { oled.setCursor(126-t, 1); oled.print(char(0x7F)); }
  oled.setCursor(126, 1); oled.print("[");
  
  
  oled.setCursor(96, 2);
  for(byte t=0; t<=1.6*(17-volC); t+=4) {oled.print("|"); }
  for(byte t=1.6*(volC); t>0; t--) { oled.setCursor(126-t, 2); oled.print(char(0x7F)); }
  oled.setCursor(126, 2); oled.print("[");
  
  }   
  cy++;
  if(cy>20) {cy=1;}
  short int bc=analogRead(A6);
  if(keycodes==true) { oled.setCursor(72, 0); oled.print(bc); oled.print("   "); }
  short int bc2=NOBUT;
  if ((bc<bc2)||(ft==true)||(playFinished==true)){
  bc=analogRead(A6);
  if(playFinished==true) {bc=(NEXT+10);}
  if((bc>=OK)&&(bc<=OK2)&&(playFinished==false)) { delay(200);} else { 
  if(fp) { fp.close(); }
  if(root) { root.close(); }
  ft=false;
  
  
  playFinished = false;
  //oled.print(bc,DEC);   
     
  fillPos = 0;
  playPos = 0;
  cli();  
  fillBuffer();
  sei();
  delay(200);
  oled.set2X();
  }
  bc2=analogRead(A6);
  //oled.print(bc);
  oled.print(" ");
  if((bc>=OK)&&(bc<=OK2)) {
  if(randomize==false) { randomize=true; oled.set1X();oled.setCursor(122, 3); oled.print("R");  } else {randomize=false; oled.set1X();oled.setCursor(122, 3); oled.print(" ");  }      }
  if((bc>=NEXT)&&(bc<=NEXT2)) { oled.set2X();  for(byte t=0; t<72; t++) { oled.setCursor(t, 0); oled.print(">>"); } oled.setCursor(72, 0); oled.print("  ");  }
  if((bc>=PREV)&&(bc<=PREV2)) {oled.set2X();  for(byte t=0; t<72; t++) { oled.setCursor(72-t, 0); oled.print("<<"); } oled.setCursor(0, 0); oled.print("  "); }
  if((bc>=NEXT)&&(bc<=NEXT2)) {
  //NEXT BUTTON
  if((bc2>=NEXT)&&(bc2<=NEXT2)) {
  //Serial.println(bc2,DEC); 
  oled.set2X();
  oled.setCursor(0, 0);
  oled.println("DIR");
  byte t=0;
  while(analogRead(A6)<NOBUT) {oled.setCursor(36+t, 0); oled.print(">>"); t++; if(t>36) {t=0;oled.setCursor(72, 0); oled.print("  ");} } 
  //NEXT DIR
  curdir++;  sel=0; 
  if(curdir>=dirCnt) { curdir=0; sel=0; } 
  //delay(1000); 
  } else { 
  sel++; 
  }  
  if(sel>(fileCnt-1)) {
  curdir++;
  if(curdir>=dirCnt) {
  curdir=0;
  }
  sel=0;
  }
  } 
if((bc>=PREV)&&(bc<=PREV2)) {
    //PREVIOUS BUTTON
    //499
  if((bc2>=PREV)&&(bc2<=PREV2)) {
    //PREV DIR
    //Serial.println(bc2,DEC); 
  oled.set2X();
  oled.setCursor(0, 0);
  oled.println("DIR");
  byte t=0;
  while(analogRead(A6)<NOBUT) {  oled.setCursor(72-t, 0); oled.print("<<"); t++; if(t>36) {t=0;oled.setCursor(36, 0);oled.print("  ");} } 
    curdir--;  sel=0; 
    //Serial.println(String(curdir)); 
  if(curdir<0) {   
  curdir = (dirCnt-1); 
  sel=0;  
    }  
  oled.set2X();
  oled.setCursor(0,0);
  for(byte i=0; i<8; i++) { oled.print(" "); }
  oled.setCursor(0,0);
  oled.print("\\");
  oled.print((curdir+1), DEC);
  oled.print("-");
  oled.print((sel+1), DEC);  
    } else {  
      sel--;
      if((curdir==0)&&(sel<0)) {  
      //randomize=true;
      sel=0;
      }
      }
} 
if((bc>=OK)&&(bc<=OK2)) {} else {
  if(randomize==false ) { 
  if(sel<0) { curdir--;  if(curdir<0) { curdir=(dirCnt-1); } sel=0;
  }
  oled.set2X();
  oled.setCursor(0,0);
  for(byte i=0; i<8; i++) { oled.print(" "); }
  oled.setCursor(0,0);
  oled.print("\\");
  oled.print((curdir+1), DEC);
  oled.print("-");
  oled.print((sel+1), DEC);
  }  else {
  oled.setCursor(0,0);
  //Serial.println("RND");
  for(byte i=0; i<5; i++) { oled.print(" "); } 
  curdir = random(0,(dirCnt-1)); 
  root = SD.open("/" + String(curdir));
  fileCnt=countDirectory(root,String(curdir));
  root.close();
  sel = random(0,fileCnt-1);
  //Serial.println("RND");
  oled.set2X();
  oled.setCursor(0,0);
  for(byte i=0; i<8; i++) { oled.print(" "); }
  oled.setCursor(0,0);
  oled.print("\\");
  oled.print((curdir+1), DEC);
  oled.print("-");
  oled.print((sel+1), DEC); 
  }
  //delay(500);
  oled.set1X();
  openNxFile();
  oled.set1X();
  oled.setCursor(0,3);  
  for(byte i=0; i<24; i++) { oled.print(" "); }
  oled.setCursor(0,2);
  for(byte i=0; i<16; i++) { oled.print(" "); }
  oled.setCursor(0,2);
  oled.print(psgname); 
  
  playFinished = false;
  fillBuffer(); 
  }
  }
}

void fillBuffer(){
  short int fillSz = 0;
  short int freeSz = bufSize;
  if (fillPos>playPos) {
    fillSz = fillPos-playPos;
    freeSz = bufSize - fillSz;
  }
  if (playPos>fillPos) {
    freeSz = playPos - fillPos;
    fillSz = bufSize - freeSz;
  }
  
  freeSz--; // do not reach playPos
  while (freeSz>0){
    byte b = 0xFD;
    if (fp.available()){
      b = fp.read();
    }
    playBuf[fillPos] = b;
    fillPos++;
    if (fillPos==bufSize) fillPos=0;
    freeSz--;
  }
}

void prepareFile(char *fname){
 /* //Serial.println();
  //Serial.print("P [");
  //Serial.print(fname);
  //Serial.println("]");
  */
  fp = SD.open(fname);
  delay(100);
  if (!fp){
    /*//Serial.println("ERROR");*/
  oled.clear();
  oled.set2X();
  oled.println("ERROR");
  oled.set1X();
  oled.println(fname);
  playFinished = true;
  digitalWrite(SS,LOW);
  delay(2000);
  digitalWrite(SS,HIGH);
  if(fp) { fp.close(); }
  if(root) { root.close(); }
  return;
  } else {
    
    total=fsize=fp.size();
  }
    
  while (fp.available()) {
    
    byte b = fp.read();
    if (b==0xFF) {  break; }
  }
 
  fillPos = 0;
  playPos = 0;
  cli();  
  fillBuffer();
  resetAY();
  sei();
}



String openNxFile(){
/*//Serial.print("D=");
  //Serial.print(curdir, DEC);
  //Serial.println();
  //Serial.print("F=");
  //Serial.print(sel, DEC);
  //Serial.println();
 */
short int res=0;
String entryname;
  File dir=SD.open("/" + String(curdir));
  ////Serial.println("/" + String(curdir));
  dir.rewindDirectory();
  while (true) {

    File entr =  dir.openNextFile();
    if (!entr)  { 
      ////Serial.println("END"); 
      break; 
      }
    ////Serial.print(String(res) + "\t\t" + "/" + String(curdir) +"/");
    ////Serial.print(entr.name());
    if (!entr.isDirectory()) {
      ////Serial.print("\t\t");
      ////Serial.println(entr.size(), DEC);
      ////Serial.println();
      if(res==sel) {entr.close(); entryname=String(curdir)+"/"+entr.name(); break;  }
      res++;
    }
    entr.close();
  }
  dir.close();
delay(100);
    ////Serial.print(entryname);      
     prepareFile(string2char("/" + entryname));  
     //entryname.replace("/", "/");
     //entryname.replace(".PSG", ""); 
     psgname=entryname.substring(0,14);
     return entryname;  
}


bool initFile(){
  //Serial.print("INIT SD");
    delay(500);
  if (!SD.begin(SS)) {
    delay(500);
    if (!SD.begin(SS)) {
    delay(500);
    return false;
    }
  }
  //Serial.println("OK");
fileCnt=0;
  root = SD.open("/" + String(0));
  fileCnt=countDirectory(root,String(0));
  root.close();
  //oled.set2X();
  //oled.print(openNxFile());  

  //Serial.print("BUF=");
  //Serial.print(bufSize, DEC);
  //Serial.println();
  
  //Serial.print("POZ=");
  //Serial.print(fillPos, DEC);
  //Serial.println();

  //Serial.print("PLAY=");
  //Serial.print(playPos, DEC);
  //Serial.println();

  //for (byte i=0; i<bufSize;i++){
    //Serial.print(playBuf[i],HEX);
    //Serial.print("-");
    //if (i%16==15) {  
      //Serial.println(); 
      //}
  //}
  return true;
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

int countDirectory(File dir, String dirname) {
  short int res = 0;
  while (true) {

    File entry =  dir.openNextFile();
    if (!entry)  break;
    //Serial.print(dirname+"/");
    //Serial.print(entry.name());
    if (!entry.isDirectory()) {
      //Serial.print("\t\t");
      //Serial.println(entry.size(), DEC);
      //Serial.println();
      res++;
    }
    entry.close();
  }
  
  return res;
}

short int skipCnt = 0;

ISR(TIMER1_COMPA_vect){
  if (skipCnt>0){
    skipCnt--;
  } else {
    short int fillSz = 0;
    short int freeSz = bufSize;
    if (fillPos>playPos) {
      fillSz = fillPos-playPos;
      freeSz = bufSize - fillSz;
    }
    if (playPos>fillPos) {
      freeSz = playPos - fillPos;
      fillSz = bufSize - freeSz;
    }
    boolean ok = false;
    short int p = playPos;
    while (fillSz>0){
      byte b = playBuf[p];
      
      p++; if (p==bufSize) p=0;
      fillSz--;
      fsize--;
      if (b==0xFF){ ok = true; break; }
      if (b==0xFD){ 
        ok = true; 
        playFinished = true;
        for (byte i=0;i<16;i++) ay_out(i,0);
        volA=0;
        volB=0;
        volC=0;
        break; 
      }
      if (b==0xFE){ 
        if (fillSz>0){
          skipCnt = playBuf[p];
          p++; if (p==bufSize) p=0;
          fillSz--;
          fsize--;
          skipCnt = 4*skipCnt;
          ok = true; 
          break; 
        } 
      }
      if (b<=252){
        if (fillSz>0){
          byte v = playBuf[p];
          p++; if (p==bufSize) p=0;
          fillSz--;
          fsize--;
          if (b<16) ay_out(b,v);
        } 
      }
    } // while (fillSz>0)
  
    if (ok){
      playPos = p;
    }
  } // else skipCnt 
}
