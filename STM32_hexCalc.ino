//STM32F103CBT6 (china copy GD103) ST7735 14 pin SPI 1.77" 
//PA0 0-3300mV 100k series, 100nF to GND. PA1 divider 10M / 1M2, 100nF to GND 
//PA2 connected to current mirror common base. when input, samples the ref V on resistors. When high output, disables the current mirror.
//100uA flows always (27.4k). PA11 200uA,PA12 900uA,PA8 2.9mA,PA13 9.9mA via mosfets
//PB14 senses power switch position

#include <Adafruit_GFX.h>  
#include <Adafruit_ST7735.h> 
#include <SPI.h>

#define TFT_CS     PB12
#define TFT_DC     PB13
#define TFT_RST    PC14 

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

byte kdrv[]={PB8,PB7,PB5,PB3,PA14,PA15};
byte kbin[]={PB9,PB6,PB4,PC13};
byte uasw[]={PA11,PA12,PA8,PA13};
byte kbcod[]={12,8,4,0,13,9,5,1,14,10,6,2,15,11,7,3,'/','*','-','+','C','&','#','='};
float sample[3600],prev[8],vref=3.300,m ;   //vref is the 3.3V supply HT7333 measured w Fluke12;
unsigned int tim1,tim2,tim3,tim4=100,timout=6000;
byte scn=0xff, stat ,opr, hexopr=1,dvm =0; String txt[]={"DEC","HEX"};
String mA[]={"0.1m","0.3m","1.0m","3.0m","10mA"};
unsigned int op[2],num,sidx,tidx,nidx; // sample , tft index 

int fr, freq[]={261,293,330,349,392,440,494}; //261,277,293,311,330,349,367,392,415,440,466,494}; //c c# d d# e f f# g g# a a# b c c# d d# e f f# g g# a a# b
byte x;
void setup(void) {
  disableDebugPorts();
  Serial.begin(9600);
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1);
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  for (byte i=0; i<6; i++) pinMode(kdrv[i],OUTPUT); 
  for (byte i=0; i<4; i++) pinMode(kbin[i],INPUT_PULLUP);
  pinMode(PB14,INPUT_PULLUP);  //power switch position
  for (byte i=0; i<4; i++) pinMode(uasw[i],OUTPUT);
  pinMode(PA2,OUTPUT); digitalWrite(PA2,HIGH);tft.fillScreen(ST7735_BLACK); 
  pinMode(PA0, INPUT_ANALOG); pinMode(PA1, INPUT_ANALOG);
  
  //tft.fillScreen(ST7735_BLACK); 
    Timer4.setPeriod(32000); Timer4.setPrescaleFactor(100);
    Timer4.setMode(TIMER_CH1,TIMER_OUTPUTCOMPARE);
    Timer4.setCompare(TIMER_CH1, 1); 
    Timer4.attachInterrupt(TIMER_CH1,handler);
    Timer4.refresh();
    //Timer4.resume();
    Timer4.pause();
    //pinMode(PA9, OUTPUT);
  pinMode(PB0, PWM);  //sound output Timer3
  Timer3.setPrescaleFactor(1000);    //72Mhz divider 
  Timer3.pause(); if (digitalRead(PB14)) dvmFrame(61); else calFrame(); 
}

void loop() {
  if (digitalRead(PB14) !=dvm) {if ( digitalRead(PB14))dvmFrame(61); else calFrame(); };
dvm=digitalRead(PB14);
if (dvm) {                 // stat 0x00-0x0f dvm ; 0x10-0x1f beeper ; 0x20-0x2f scanner
if (scn !=0xff) { byte r=kbcod[scan()]; 
                     
                    if (r==61) {dvmFrame(r);stat = 0x00; if (tim4 == 100) tim4=1000; else tim4=100; } //voltmeter
                    if (r==35) {dvmFrame(r);stat = 0x10;} //beeper
                    if (r==38) {dvmFrame(r);stat = 0x20;} //scanner
                    if (r==42) {stat |= 0x02; stat &=~0x04; Timer3.resume(); tim1 = millis()+timout;} //sound on logic
                    if (r==47) {stat |= 0x04; stat &=~0x02; Timer3.resume(); tim1 = millis()+timout;} //sound on offset
                    if (r==67) {stat &= ~0x06; Timer3.pause();} //sound off
                    if (r=='+') {if (stat&0x08) {stat &= ~0x08;tft.setCursor(8,90); tft.fillRect(7,90,115,30,ST7735_BLACK);}
                                else stat |= 0x08; nidx=sidx;}
                    if (r=='-') {sidx =0; }
                    
                    while(scan() != 0xff); }

                      
if ((stat & 0xf0) == 0x00) { //============================ Voltmeter ================================
 
   pinMode(PA2,OUTPUT); digitalWrite(PA2,HIGH); tft.setCursor(10,45); m= analogRead(PA0);
  if (m>4090) {stat |=0x01; tim2=millis()+timout;} else if (millis()> tim2) stat &= ~0x01;
  if (tim2>millis()) {m=analogRead(PA1); m *= 11.2; m /=1.2;} 
  m*= vref; m/=4095; tft.setCursor(20,40);tft.setTextSize(4); tft.print(m); tft.print('V'); 
  tft.drawLine(2,85,157,85,ST7735_BLUE);tft.drawLine(2,84,157,84,ST7735_BLUE);
  tft.setCursor(8,94); tft.setTextSize(1);
  if (stat & 0x04) tft.print("Sound on    "); else 
  if (stat & 0x02) tft.print("Sound > 0.9V"); else 
  if (!(stat & 0x06)) tft.print("            ");
  tft.setCursor(7,104); tft.print(sidx); tft.print(" samples collected   ");
  tft.setCursor(7,114); tft.print("Use + - for samples ");
  tft.print((float)tim4/1000,1);tft.print('s');//tft.print(stat);
  if (stat & 0x06) sound(); if (millis()>tim1) { Timer3.pause(); }
  if (m>0.2 && stat&0x06) {tim1  = millis()+ timout; Timer3.resume();}  //cycle = 31ms
  if (millis() > tim3) {tim3= tim4 * ((millis()/tim4)+1); //=======================tim4 loop 100ms,1s
  if (stat & 0x08) { 
  dvmFrame('+'); tft.print((float)tim4/1000,1); tft.print(" sec");
  while  (scan() != 23  ) { //wait for = sign
  if (nidx<50) nidx=50; tft.setCursor(6,22);
  for (byte i=50; i; i--) {tft.print(sample[nidx-i]); if ((i%5)==1) tft.println(); tft.print(' ');}
  tft.drawLine(0,0,0,120,ST7735_BLUE);tft.drawLine(1,0,1,120,ST7735_BLUE);
  tft.setCursor(6,115); tft.print("Samples "); tft.print(nidx-50); tft.print(" to ");tft.print(nidx-1); tft.print("   ");
     //while(scan() != 18 && scan() !=19) {if (scan() == 23) break;}
  while(scan() != 18 && scan() !=19 && scan() !=23 );  //react to + - = only
  if (scan() ==18 && nidx>50) nidx -=50;  if (scan() == 19 ) nidx +=50; if (nidx > 3600) nidx=3600;
   
  } //show table inner loop
  } //Table show exit, 0x08 flag reset when calling voltmeter menu
  
  sample[sidx]=m; sidx++; 
  if (sidx > 3599) sidx=0;
  }  //tim4 loop 100ms,1s  //Serial.println(millis()-tim2); Serial.print(' '); tim2=millis();
  }
  
if ((stat & 0xf0) == 0x10) { //=========================== Beeper ====================================
       float m; pinMode(PA2,INPUT); digitalWrite(PA12,1); 
       if (analogRead(0)> 4090) {m= analogRead(PA1); m *= 11.2; m /=1.2;} else m= analogRead(PA0); 
       m*= vref; m/=4095; 
       tft.setCursor(20,40);tft.setTextSize(4); tft.print(m); tft.print('V');
       if ( m < 0.1 ) {Timer3.resume(); sound();} else Timer3.pause(); 
       
  }
                     
if ((stat & 0xf0) == 0x20) {//============================ Scanner ===================================
 Timer3.pause();
  pinMode(PA0,OUTPUT); digitalWrite(PA0,0); uamp(0); delay(100); pinMode(PA0,INPUT); //discharge input
  for (byte i=0; i<5; i++) {uamp(1+i);  
             for  (byte i=0; i<10; i++) {delay(10); scn= scan(); if (scn !=0xff) break;}
             sample[i]= analogRead(PA0);  //delay and scan keyboard
             sample[i] = analogRead(PA0); sample[i] *= vref; sample[i]/=4095;}//graph
  tft.drawRect(0,0,159,127,ST7735_BLUE);tft.drawRect(1,1,157,125,ST7735_BLUE);
  tft.setTextSize(1); tft.setCursor(80,35); tft.print("mA-V  Graph");
  byte a=10; 
  for (byte i=0; i<5; i++) {
    tft.fillCircle( i*a+15,125-prev[i]*40,3,ST7735_BLACK); prev[i]=sample[i];
    if (sample[i]<3.1) tft.fillCircle( i*a+15,125-sample[i]*40,3,ST7735_WHITE);
    tft.setCursor(80,110-i*14); tft.print(mA[i]);tft.print("  ");tft.print(sample[i],2);tft.print("V");
    
  }
 
  for (byte i=0; i<3; i++) for (byte j=0; j<56; j++) if (j % 4) tft.drawPixel(j, i*40+5 ,ST7735_BLUE); 
  for (byte i=0; i<3; i++) { tft.setCursor(3, 113-i*40); tft.print(i);   }
  for (byte i=0; i<10; i++) {delay(50); scn= scan(); if (scn !=0xff) break;}  
  }
}

else 
{  //===================================== Calculator ===============================
scn=scan();
//if (scn !=0xff) {if (scn<0x10) Serial.println(kbcod[scn],HEX); else Serial.write(kbcod[scn]);}
if (scn !=0xff) {  //stat = 0 enter 1st operand; stat = 1 have operator; stat = 2 calc result
  
  
  if (scn<0x10) {  //=========================== it is a number ===========================
  tft.setCursor(10, 10+30*stat); 
  if (hexopr) { op[stat] = op[stat]*16; op[stat] +=kbcod[scn]; tft.print(op[stat],HEX);}
         else { if (kbcod[scn]<0x0A) {op[stat] = op[stat]*10; op[stat] +=kbcod[scn];} tft.print(op[stat],DEC);} 
  tft.setCursor(100, 15+30*stat);tft.setTextSize(1);
  if (hexopr) tft.print(op[stat],DEC); else tft.print(op[stat],HEX);
  tft.setTextSize(3);}  //It is a (hex,dec) number
  //============================== it is an operator ================================
  else { tft.setCursor(138, 10+ 30*stat); 
    // 
    tft.write(kbcod[scn]); stat++;
    
    if (kbcod[scn]=='#')  {hexopr = 1-hexopr; calFrame();}
    
    //========================== CE ==========================================
    if (kbcod[scn]=='C')  {  stat--; 
    op[stat] /=(10+6*hexopr);
    delay(200); //Serial.print(op[0],HEX);Serial.print(" "); Serial.print(op[1],HEX);Serial.print(" ");Serial.write(opr);Serial.println(" ");
   tft.fillRect(10,10,159-12,90,ST7735_BLACK);
   
   if (stat ==0) { //tft.setTextColor(ST7735_RED,ST7735_BLACK);
   tft.setCursor(10, 10+30*stat); 
   if (hexopr) {  tft.print(op[stat],HEX);}
         else {  tft.print(op[stat],DEC);} 
  tft.setCursor(100, 15+30*stat);tft.setTextSize(1);
  if (hexopr) tft.print(op[stat],DEC); else tft.print(op[stat],HEX);
  tft.setTextSize(3);//tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
   } //stat0
   
     if (stat ==1) { //tft.setTextColor(ST7735_RED,ST7735_BLACK);
   tft.setCursor(10, 10);
   if (hexopr) { tft.print(op[0],HEX);}  else { tft.print(op[0],DEC);}
         tft.setCursor(140, 10); tft.write(opr); 
   tft.setCursor(100, 15);tft.setTextSize(1);
   if (hexopr) tft.print(op[0],DEC); else tft.print(op[0],HEX);
   tft.setTextSize(3);
   tft.setCursor(10, 10+30*stat);
   if (hexopr) {  tft.print(op[stat],HEX);}
         else {  tft.print(op[stat],DEC);} 
  
  tft.setCursor(100, 15+30*stat);tft.setTextSize(1);
  if (hexopr) tft.print(op[stat],DEC); else tft.print(op[stat],HEX);
  tft.setTextSize(3);//tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
   }  //stat1
   
   }  //CE clear last entry

   //================================ CE end ========================
   //delay(50); Serial.print(op[0],HEX);Serial.print(" "); Serial.print(op[1],HEX);Serial.print(" ");Serial.write(opr);Serial.println("... ");
   if (stat == 1 && kbcod[scn] !='C') opr = kbcod[scn];
   if (stat>0  && kbcod[scn]=='=') { // result , clear
    switch (opr) { 
                    case '+' : num = op[0] + op[1]; break;
                    case '-' : num = op[0] - op[1]; break;
                    case '*' : num = op[0] * op[1]; break; 
                    case '/' : num = op[0] / op[1]; break;
                    case '&' : num = op[0] & op[1]; break;
                    case '=' : num = op[0]        ; break;
      }
   //if (opr == '+') num = op[0] + op[1]; if (opr == '-') num = op[0] - op[1]; if (opr == '*') num = op[0] * op[1]; if (opr == '/') num = op[0] / op[1]; 
   
    tft.setCursor(10, 10+ 30*stat);
    if (hexopr) tft.print(num,HEX); else tft.print(num,DEC);
    tft.setCursor(90, 40+30*stat);tft.setTextSize(1);
   if (hexopr) tft.print(num,DEC); else tft.print(num,HEX);
   tft.setTextSize(3);
    while (scan() != 0xff); delay(100);
    while (scan() == 0xff); delay(100);
    calFrame();
   } //stat2
  }  //not a number
  }

while(scan() != 0xff); delay(50);
} //not dvm

 for  (byte i=0; i<10; i++) {
 if (stat & 0xf0) delay(5); //voltmeter = no delay
 scn= scan(); if (scn !=0xff) break;}
} //loop

void handler (void)
  {
  digitalWrite(PA9,!digitalRead(PA9));

  byte sc =kbcod[scan()];
 }

byte dlyScan(unsigned int d){
     byte dsc; 
     while(d--) {delay(1); dsc=scan(); }
     return(dsc);
}

byte scan(void){
  byte i,j,r=0xff;
  for (i=0; i<6; i++) {digitalWrite(kdrv[i],LOW); for (j=0; j<4; j++) if (digitalRead(kbin[j])==0) r=i*4+j; digitalWrite(kdrv[i],HIGH);}
  return(r);
  }
  
void dvmFrame(byte x){tft.fillScreen(ST7735_BLACK); tft.drawRect(0,0,159,127,ST7735_BLUE);tft.drawRect(1,1,157,125,ST7735_BLUE);
                tft.setTextSize(2); tft.setCursor(25,10); 
                switch (x) {
                 case 61: { tft.print("Voltmeter"); break;}
                 case 35: { tft.print("Beeper"); tft.setTextSize(1);tft.setCursor(8,114); tft.print("Output 1mA"); break; }
                 case 38: { tft.print("Scanner"); break; } 
                 case '+': {tft.setTextSize(1);tft.print("Samples "); break;}
      }               
  }
  
void calFrame(void) {
  tft.fillScreen(ST7735_BLACK); tft.setCursor(10,10); tft.setTextSize(3);tft.print(0);
  tft.drawRect(0,0,159,127,ST7735_BLUE);tft.drawRect(1,1,157,125,ST7735_BLUE);
  stat=0; op[0]=op[1]=0; num=0; opr=0;
  tft.setCursor(10,100); tft.setTextSize(2);tft.print(txt[hexopr]);tft.setTextSize(1);
  tft.setCursor(90,112); tft.print(txt[1-hexopr]);tft.setTextSize(3);
  }

  void uamp(byte u){
    //0=0, 1=100uA, 2=300uA, 3=1mA, 4=3mA, 5=10mA values
    if (u>5) u=5; if (u) pinMode(PA2,INPUT);
    for (byte i=0; i<4; i++) digitalWrite(uasw[i],0);
    switch (u) {
              case 0: { pinMode(PA2,OUTPUT); digitalWrite(PA2,HIGH); break;}
              case 1: { break; }
              default:{ digitalWrite(uasw[u-2],1); break; } //2 to 5 will be 0 to 3
      }
    }

void sound(void){
   
    int dcval=analogRead(PA0); dcval/=186; 
    if ((stat & 0x04)) {
    if (dcval>21) dcval=21; 
    if (dcval < 7) fr=freq[dcval]/2;
    if (dcval > 6 && dcval < 14) fr=freq[dcval % 7];
    if (dcval > 13 && dcval < 21) fr=freq[dcval % 7]*2;
    if (dcval > 20) fr=freq[dcval % 7]*4;
    }
    if (stat & 0x02){ if (dcval>5) fr=freq[4]*2; else fr=20000;}
    if ((stat & 0xf0) == 0x10) fr=freq[4]*2;
    fr= 9680*4/fr;
    Timer3.setOverflow(fr);  //timer overflow frequency
    pwmWrite(PB0,fr / 2);    //PWM 50%
  }    
    
 
