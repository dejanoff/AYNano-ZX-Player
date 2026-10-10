#include <SPI.h>
#include <SD.h>

void resetAY(){
  pinMode(A0, OUTPUT); // D0
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT); // D3
  
  pinMode(4, OUTPUT); // D4
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT); // D7
  
  pinMode(8, OUTPUT);  // BC1
  pinMode(9, OUTPUT);  // BDIR
  
  digitalWrite(8,LOW);
  digitalWrite(9,LOW);
  
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  
  for (int i=0;i<16;i++) ay_out(i,0);
}


void setupAYclock(){
  pinMode(3, OUTPUT);
  TCCR2A = 0x23;
  TCCR2B = 0x09;
  OCR2A = 8;
  OCR2B = 3; 
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(4)+analogRead(5));

  initFile();

  setupAYclock();
  resetAY();
  setupTimer();
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
const int bufSize = 200;
byte playBuf[bufSize]; // 31 bytes per frame max, 50*31 = 1550 per sec, 155 per 0.1 sec

File fp;
boolean playFinished = false;

void loop() {
  fillBuffer();
  if (playFinished){
    fp.close();
    openRandomFile();
    playFinished = false;
  }
}

void fillBuffer(){
  int fillSz = 0;
  int freeSz = bufSize;
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
  Serial.print("prepare [");
  Serial.print(fname);
  Serial.println("]...");
  
  fp = SD.open(fname);
  
  if (!fp){
    Serial.println("error opening music file");
    return;
  }  
    
  while (fp.available()) {
    byte b = fp.read();
    if (b==0xFF) break;
  }
 
  fillPos = 0;
  playPos = 0;
  cli();  
  fillBuffer();
  resetAY();
  sei();
}

File root;
int fileCnt = 0;

void openRandomFile(){
  int sel = random(0,fileCnt-1);

  Serial.print("File selection = ");
  Serial.print(sel, DEC);
  Serial.println();
  
  root.rewindDirectory();
  
  int i = 0;
  while (true) {
    File entry =  root.openNextFile();
    if (!entry)  break;

    Serial.print(entry.name());
    if (!entry.isDirectory()) {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
      
      if (i==sel) prepareFile(entry.name());
      i++;
    }
    entry.close();
  }
}

void initFile(){
  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  root = SD.open("/");

  // reset AY

  fileCnt = countDirectory(root);
  Serial.print("Files cnt = ");
  Serial.print(fileCnt, DEC);
  Serial.println();
  openRandomFile();

  Serial.print("Buffer size = ");
  Serial.print(bufSize, DEC);
  Serial.println();
  
  Serial.print("fillPos = ");
  Serial.print(fillPos, DEC);
  Serial.println();

  Serial.print("playPos = ");
  Serial.print(playPos, DEC);
  Serial.println();

  for (int i=0; i<bufSize;i++){
    Serial.print(playBuf[i],HEX);
    Serial.print("-");
    if (i%16==15) Serial.println();
  }

  Serial.println("done!");
}

int countDirectory(File dir) {
  int res = 0;
  root.rewindDirectory();
  while (true) {

    File entry =  dir.openNextFile();
    if (!entry)  break;

    Serial.print(entry.name());
    if (!entry.isDirectory()) {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
      res++;
    }
    entry.close();
  }
  
  return res;
}

int skipCnt = 0;

ISR(TIMER1_COMPA_vect){
  if (skipCnt>0){
    skipCnt--;
  } else {
    int fillSz = 0;
    int freeSz = bufSize;
    if (fillPos>playPos) {
      fillSz = fillPos-playPos;
      freeSz = bufSize - fillSz;
    }
    if (playPos>fillPos) {
      freeSz = playPos - fillPos;
      fillSz = bufSize - freeSz;
    }

    boolean ok = false;
    int p = playPos;
    while (fillSz>0){
      byte b = playBuf[p];
      p++; if (p==bufSize) p=0;
      fillSz--;
      
      if (b==0xFF){ ok = true; break; }
      if (b==0xFD){ 
        ok = true; 
        playFinished = true;
        for (int i=0;i<16;i++) ay_out(i,0);
        break; 
      }
      if (b==0xFE){ 
        if (fillSz>0){
          skipCnt = playBuf[p];
          p++; if (p==bufSize) p=0;
          fillSz--;
          
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
          
          if (b<16) ay_out(b,v);
        } 
      }
    } // while (fillSz>0)
  
    if (ok){
      playPos = p;
    }
  } // else skipCnt 
}
