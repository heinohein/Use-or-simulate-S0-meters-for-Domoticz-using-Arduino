//---------------------------------------------------------------------------------------------
// 3-1-2017 copied from https://forum.mysensors.org/topic/1978/power-usage-sensor-multi-channel-local-display/2
// Functions original source:
// Arduino Pulse Counting Sketch for counting pulses from up to 12 pulse output meters.
// uses direct port manipulation to read from each register of 6 digital inputs simultaneously
//
// Licence: GNU GPL
// part of the openenergymonitor.org project
//
// Original Author and credits to: Trystan Lea
// 
// This scetch simulates the output of a S0PCM5 device. This device sends a message with the counters of 5 S0 meters
// to the Domoticz software. In this scetch the puls counts of zero or more S0 meters can be send to Domoticz but the
// output of up to 12 S0 meters can easily be manipulated or simulated. See DEBUG in this scetch 
// 
// 1-10-2019  V3.1 source renamed tot Simulate_S0PCM5_Domoticz.ino, now only the S0PCM5 code is retained except for the power 
//            computations(which are not needed for Domoticz but make info in Domoticz comparable)
// 6-10-2019  V3.2 debug version autogenerate random pulses in meter 1 2 3 4 5
// 7-10-2019  V3.3 add meters 6 7 8 9 to see what is happening in Domoticz. The output of 6+ is not displayed in Domoticz
//
/* Output specification:
Search Google S0PCM Module testing en protocoll.pdf:
/32432:S0 Pulse Counter V0.5
ID:32432:I:10:M1:0:0:M2:0:0:M3:0:0:M4:0:0:M5:0:0
Search google DIY S0-pulsecounter ready2use with Domoticz (Arduino based)
/a:S0 Pulse Counter V0.x
Data record (repeated every interval):
ID:a:I:b:M1:c:d:M2:e:f:M3:g:h:M4:i:j:M5:k:l
a -> Unique ID of the S0PCM
b -> interval between two telegrams in seconds
c -> number of pulses in the last interval of register 1
d -> number of pulses since the last start-up of register 1 etc
*/
//---------------------------------------------------------------------------------------------
//PLEASE CONFIGURE lastMeter
// PLEASE/ ALTER and CHECK LINES WITH // DEBUG .... before compiling
//---------------------------------------------------------------------------------------------
char VERSION[]="V3.3";   //************************************

class PulseOutput
{
public:                                      //AWI: access to all
  boolean pulse(int,int,unsigned long);                  //Detects pulses, in pulseLib.ino
  unsigned long rate( unsigned long );                   //Calculates rate 

  unsigned long count;                                   //pulse count accumulator
  unsigned long countAccum;                              //pulse count total accumulator for extended error checking (only resets at startup)
  unsigned long prate;                                   //pulse width in time 
  unsigned long prateAccum;                              //pulse rate accumulator for calculating mean.
  float dayUsagekWh;                                     //total usage today in Watthour
private:
  boolean ld,d;                                          //used to determine pulse edge
  unsigned long lastTime,time;                           //used to calculate rate
};

//---------------------------------------------------------------------------------------------
// Variable declaration
//---------------------------------------------------------------------------------------------

//CHANGE THIS TO VARY RATE AT WHICH PULSE COUNTING ARDUINO SPITS OUT PULSE COUNT+RATE DATA
//time in seconds;
const unsigned long printTime = 10000000;  // delay between serial outputs in micro seconds (one meter at a time)  
const byte lastMeter = 13;                 // is number of meters + 1 (2->13)

byte curMeter = 2 ;                        // current meter for serial output, wraps from 2 to lastMeter
char data11[11];                           // workfields for sprintf statement

//---------------------------------------------------------------------------------------------
PulseOutput p[14];                         //Pulse output objects

int a,b,la,lb;                             //Input register variables

unsigned long ltime, time, deltaTime;      //time variables

#define CTS A1                             // use port A1 for CTS clear to send to master (not needed if used standalone
unsigned long ctsTime=0ul;                 // time when a CTS low stopped transmitting to master
unsigned long deltaCtsTime=10000000ul;     // 10 secondes

float momentPowerkW;                       // workfields power computation
float totalMomentPowerkW;                  // running total of momentPowerkW
float contributePowerkW;
float dayTotalUsedkWh;
unsigned long pulseRate;

float multiplyer;                         // for test random etc
float momentPower4kW; 
//******************************************************************************************

void setup()
{
  // take care: pull-up inverses state! line 155
  // setup input pins here with pull_up, else (default) float
  pinMode( 2, INPUT_PULLUP);
  pinMode( 3, INPUT_PULLUP);
  pinMode( 4, INPUT_PULLUP);
  pinMode( 5, INPUT_PULLUP);
  pinMode( 6, INPUT_PULLUP);
  pinMode( 7, INPUT_PULLUP);
  pinMode( 8, INPUT_PULLUP);
  pinMode( 9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  pinMode(CTS, INPUT_PULLUP);    // if CTS is HIGH allow transmitting results to AWI Master
 
  Serial.begin(9600);         //Must be 9600 according to S0PCM-5 specs
  Serial.print("{L}Start Simulate S0PCM5 for Domoticz ");
  Serial.println(VERSION);
  
  DDRD = DDRD | B00000000;    // see on Youtube "Arduino basics 103"  start at ca 3 min Port manipulation
  DDRB = DDRD | B00000000;
  ltime = micros();           // at this moment we start reading the ports

}

void loop()
{

  la = a;                    //last register a used to detect input change 
  lb = b;                    //last register b used to detect input change

  //--------------------------------------------------------------------
  // Read from input registers
  //--------------------------------------------------------------------
  a = PIND >> 2;             //read digital inputs 2 to 7 really fast
  b = PINB;                  //read digital inputs 8 to 13 really fast
  time = micros();
  if (la!=a || lb!=b)
  {
    //--------------------------------------------------------------------
    // Detect pulses from register A
    //--------------------------------------------------------------------
    p[2].pulse(0,a,time);                //digital input 2
    p[3].pulse(1,a,time);                //    ''        3
    p[4].pulse(2,a,time);                //    ''        etc
    p[5].pulse(3,a,time);
    p[6].pulse(4,a,time);
    p[7].pulse(5,a,time);

    //--------------------------------------------------------------------
    // Detect pulses from register B
    //--------------------------------------------------------------------
    p[8].pulse(0,b,time);                //digital input 8
    p[9].pulse(1,b,time);                //etc
    p[10].pulse(2,b,time);
    p[11].pulse(3,b,time);
    p[12].pulse(4,b,time);
    p[13].pulse(5,b,time);
  }

  //--------------------------------------------------------------------
  // Spit out output every printTime sec (time here is in microseconds)
  //--------------------------------------------------------------------

  if (digitalRead(CTS)==HIGH)              // HIGH=allow transmission to master
  {
    deltaTime=time-ltime;
    if (deltaTime>printTime) 
    {
      int intDeltaTime=deltaTime/1000000ul;//compute time in sec
      dayTotalUsedkWh=0;
      totalMomentPowerkW=0;

/*
/32432:S0 Pulse Counter V0.5
ID:32432:I:10:M1:0:0:M2:0:0:M3:0:0:M4:0:0:M5:0:0
*/	  
	  Serial.println("/99001:S0 Pulse Counter V0.5"); // print Header

      Serial.print("ID:99001:I:");
      Serial.print(intDeltaTime);          // interim time in seconds
	  
      for (curMeter=2;curMeter<=lastMeter;curMeter++)
      {
//         (p[curMeter].count)=(unsigned long)(curMeter-1);             //NORMAL LOGIC BUT REMOVED FOR DEBUG !!!!
        long number = random(0,6);                              //DEBUG !!!!
        p[curMeter].count=(unsigned long)(number);              //DEBUG !!!!
        pulseRate = (unsigned long)p[curMeter].rate(time);
        if (pulseRate == 0)                                     // calculate power from pulse rate (ms) and truncate to whole Watts
        {
          momentPowerkW = 0;   // avoid overflow assume no Usage
        } 
        else 
        {
          momentPowerkW = ( 1800000. / pulseRate );             // NB 1800000. = 2000 pulse/kWhour, pulseRate is in microseconds
        }
        p[curMeter].countAccum += p[curMeter].count;            //Increment print count accumulator to allow for error checking at client side;

        totalMomentPowerkW+=momentPowerkW;                      // running total momentary used power in W 
        contributePowerkW=momentPowerkW*deltaTime/3600000000.;  //3600000000. = result in kWh 
        p[curMeter].dayUsagekWh+=contributePowerkW;
        dayTotalUsedkWh+=p[curMeter].dayUsagekWh;               // running total power used today in kWh

/*        
// ORIGINAL OUTPUT: build JSON for all counters print Count (W), Count Accum(W), Average ms
// Format {"m":meter,"c":count,"r":rate, "cA":countAccum}
        Serial.print("{\"m\":"); 
        Serial.print(curMeter-1);             //Print meter number
        Serial.print(",\"c\":"); 
        Serial.print(p[curMeter].count);      //Print pulse count
        Serial.print(",\"r\":");             
        Serial.print(pulseRate);              //Print pulse rate
        Serial.print(",\"cA\":"); 
        Serial.print(p[curMeter].countAccum); 
        Serial.println("}");
*/


        Serial.print (":M");
        Serial.print(curMeter-1);             //Print meter number
        Serial.print(":");
        Serial.print(p[curMeter].count);      // print pulse count per meter
        Serial.print (":");
        Serial.print(p[curMeter].countAccum); // print total pulses count accumulator per meter

        p[curMeter].count = 0;                //Reset count (we just send count and the increment)
        p[curMeter].prateAccum = 0;           //Reset accum so that we can calculate a new average

      } // end for (curMeter=2;CurMeter<=lastMeter;curMeter++)
      Serial.println();                      // end of message 
      
      ltime = time;                                           //save last Printed timer  
    } // end if (deltaTime>printTime)
  } // end (digitalRead(CTS)==HIGH) 
  else 
  if (ctsTime==0||time>ctsTime+deltaCtsTime)
  {
    Serial.println("{L}CTS low");
    ctsTime=time;
  }
} // end void loop()

// library for pulse, originally in separate file 

//-----------------------------------------------------------------------------------
//Gets a particular input state from the register binary value
// A typical register binary may look like this:
// B00100100
// in this case if the right most bit is digital pin 0
// digital 2 and 5 are high
// The method below extracts this from the binary value
//-----------------------------------------------------------------------------------
#define BIT_TST(REG, bit, val)( ( (REG & (1UL << (bit) ) ) == ( (val) << (bit) ) ) )

//-----------------------------------------------------------------------------------
// Method detects a pulse, counts it, finds its rate, Class: PulseOutput
//-----------------------------------------------------------------------------------
boolean PulseOutput::pulse(int pin, int a, unsigned long timeIn)
{
  ld = d;                                    //last digital state = digital state
   
  if (BIT_TST(a,pin,1)) d = 1; else d = 0;   //Get current digital state from pin number
   
  // if (ld==0 && d==1)                      // no internal pull_up if state changed from 0 to 1: internal pull-up inverts state
  if (ld==1 && d==0)                         //pull_up f state changed from 0 to 1: internal pull-up inverts state
  {
    count++;                                 //count the pulse
     
    // Rate calculation
    lastTime = time;           
    time = timeIn ;            // correction to allow for processing
    prate = (time-lastTime);// - 400;          //rate based on last 2 pulses
                                                //-190 is an offset that may not be needed...??
    prateAccum += prate - 2000;                     //accumulate rate for average calculation
     
    return 1;
   }
   return 0;
}


//-----------------------------------------------------------------------------------
// Method calculates the average rate based on multiple pulses (if there are 2 or more pulses)
//-----------------------------------------------------------------------------------
unsigned long PulseOutput::rate(unsigned long timeIn)
{
 if (count > 1)
 {
   prate = prateAccum / count;                          //Calculate average
 } else 
 {
 
 if ((timeIn - lastTime)>(prate*2)) prate = 0;}         //Decrease rate if no pulses are received
                                                        //in the expected time based on the last 
                                                        //pulse width.
 return prate; 
}

 
