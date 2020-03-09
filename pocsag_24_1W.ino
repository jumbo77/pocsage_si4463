// sendpocsag

// receive POCSAG-message from serial port
// transmit message on 433.900 Mhz using si4432-based "rf24" ISM modules

// This is the "non-ham" version of the application. All references
// to ham call-signs and the CW beacon has been removed

// this program uses the arduino RadioHead library
// http://www.airspayce.com/mikem/arduino/RadioHead/


// Version 0.1.0 (20140330) Initial release
// Version 0.1.1 (20140418) pocsag message creation now done on arduino
// Version 0.1.2 (20140516) using "Pocsag" class, moved to ReadHead library, removed callsign and CW beacon

#include <EEPROM.h>
#include <SPI.h>
#include <RH_RF24.h>
//2016/9/17
//chg RH_24.h for sel pin = 4 not ss for openlrsng
// pocsag message generation library
#include <Pocsag.h>

#define TX_ 5
#define SDN_pin 9
// Singleton instance of the radio
RH_RF24 rf24(8,2);
int led=5;//d5=green d6=red
int ptt=6;
int RX_=3;
//int TX_=4;
Pocsag pocsag;

// frequency
float freq;
int eeAddr=0;
void setup() 
{
    pinMode(led, OUTPUT);   
    pinMode(ptt, OUTPUT);   
    pinMode(RX_, OUTPUT);  
    pinMode(SDN_pin, OUTPUT);
    pinMode(TX_, OUTPUT);
    digitalWrite(SDN_pin, LOW);
    //digitalWrite(TX_, HIGH);
    EEPROM.get(eeAddr,freq);
    Serial.begin(9600);
  if (rf24.init()) {
    Serial.println("RF Module init OK");
      digitalWrite(led, HIGH);
  } else {
    Serial.println("RF Module init FAILED");
  }; // end if
      digitalWrite(ptt, LOW);
  if(freq>0){
    Serial.println(freq,4);
  rf24.setFrequency(freq, 0.000); // subtract 30 Khz for crystal correction
  }
  else{
    Serial.println("FREQ=159.8500mhz");
  rf24.setFrequency(159.8500, 0.005); // subtract 30 Khz for crystal correction
        digitalWrite(RX_, LOW);
        digitalWrite(TX_, HIGH);
  }

//P 538976 111 3 hi
//P 1000000  P 1000207 P 1000407
 // rf24.setModemConfig(RH_RF24::POCSAG_1b_2d5); 
  rf24.setModemConfig(RH_RF24::POCSAG_1b_4d5); 
  //POCSAG_1b_2d5  1200bps,2.5hkz
  //OCSAG_1b_4d5  1200bps,4.5khz
 // POCSAG_b5_4d5 512bps,5khz
  rf24.setTxPower(0x7f);

}

void loop() {
//vars
int rc;
int state; // state machine for CLI input

// bell = ascii 0x07
char bell=0x07;

// data
long int address;
int addresssource;
int repeat;
char textmsg[42]; // to store text message;
int msgsize;

int freqsize;
int freq1; // freq MHZ part (3 digits)
int freq2; // freq. 100 Hz part (4 digits)

// read input:
// format: "P <address> <source> <repeat> <message>"
// format: "F <freqmhz> <freq100Hz>"

Serial.println("BBJi QQ:1332768862");
/*
Serial.println("QQ:1332768862");
Serial.println("");
Serial.println("Format:");
Serial.println("P <address> <source> <repeat> <message> '('start=zhongwen,')'start=yingwen,@=END");
Serial.println("F <freqmhz> <freq100Hz>");
*/
// init var
state=0;
address=0;
addresssource=0;
msgsize=0;

freqsize=0;
freq1=0; freq2=0;

while (state >= 0) {
char c;
  // loop until we get a character from serial input
  while (!Serial.available()) {
  }; // end while

  c=Serial.read();

/*
  // break out on ESC
  if (c == 0x1b) {
    state=-999;
    break;
  }; // end if
*/

  // state machine
  if (state == 0) {
    // state 0: wait for command
    // P = PAGE
    // F = FREQUENCY
        
    if ((c == 'p') || (c == 'P')) {
      // ok, got our "p" -> go to state 1
      state=1;
      
      // echo back char
      Serial.write(c);
    } else if ((c == 'f') || (c == 'F')) {
      // ok, got our "f" -> go to state 1
      state=10;
      
      // echo back char
      Serial.write(c);
     // Serial.println("st=10");
    } else {
      // error: echo "bell"
      Serial.write(bell);
    }; // end else - if

    // get next char
    continue;
  }; // end state 0

  // state 1: space (" ") or first digit of address ("0" to "9")
  if (state == 1) {
    if (c == ' ') {
      // space -> go to state 2 and get next char
      state=2;

      // echo back char
      Serial.write(c);

      // get next char
      continue;
    } else if ((c >= '0') && (c <= '9')) {
      // digit -> first digit of address. Go to state 2 and process
      state=2;

      // continue to state 2 without fetching next char
    } else {
      // error: echo "bell"
      Serial.write(bell);

      // get next char
      continue;
    }; // end else - if
  };  // end state 1

  // state 2: address ("0" to "9")
  if (state == 2) {
    if ((c >= '0') && (c <= '9')) {
      long int newaddress;

      newaddress = address * 10 + (c - '0');

      if (newaddress <= 0x1FFFFF) {
        // valid address
        address=newaddress;

        Serial.write(c);
      } else {
        // address to high. Send "beep"
        Serial.write(bell);
      }; // end else - if

    } else if (c == ' ') {
      // received space, go to next field (address source)
      Serial.write(c);
      state=3;
    } else {
      // error: echo "bell"
      Serial.write(bell);
    }; // end else - elsif - if

    // get next char
    continue;
  }; // end state 2

  // state 3: address source: one single digit from 0 to 3
  if (state == 3) {
    if ((c >= '0') && (c <= '3')) {
      addresssource= c - '0';
      Serial.write(c);

      state=4;
    } else {
      // invalid: sound bell
      Serial.write(bell);
    }; // end if

    // get next char
    continue;
  }; // end state 3


  // state 4: space between source and repeat
  if (state == 4) {
    if (c == ' ') {
      Serial.write(c);

      state=6; // go from state 4 to state 6
               // (No state 5, callsign removed)
    } else {
      // invalid: sound bell
      Serial.write(bell);
    }; // end if

    // get next char
    continue;
  }; // end state 4


  // state 5: callsign: REMOVED in non-ham version
        
  // state 6: repeat: 1-digit value between 0 and 9
  if (state == 6) {
    if ((c >= '0') && (c <= '9')) {
      Serial.write(c);
      repeat=c - '0';

      // move to next state
      state=7;
    } else {
      Serial.write(bell);
    }; // end if

    // get next char
    continue;
  }; // end state 6


  // state 7: space between repeat and message
  if (state == 7) {
    if (c == ' ') {
      Serial.write(c);

      // move to next state
      state=8;
    } else {
      // invalid char
      Serial.write(bell);
    }; // end else - if

    // get next char
    continue;
  }; // end state 7


  // state 8: message, up to 40 chars, terminate with cr (0x0d) or lf (0x0a)
  if (state == 8) {
    // accepted is everything between space (ascii 0x20) and ~ (ascii 0x7e)
    if (c!='@') {
      if(c=='(')
      c=14;
      if(c==')')
      c=15;
      // accept up to 40 chars
      if (msgsize < 40) {
        Serial.write(c);

        textmsg[msgsize]=c;
        msgsize++;
      } else {
        // to long
        Serial.write(bell);
      }; // end else - if
                        
    } else if (c == '@') {
      // done
   
     // Serial.println("SMS Done");
                        
      // add terminating NULL
      textmsg[msgsize]=0x00;

      // break out of loop
      state=-1;
      break;

    } else {
      // invalid char
      Serial.write(bell);
    }; // end else - elsif - if

    // get next char
    continue;
  }; // end state 8;




  // PART 2: frequency

  // state 10: space (" ") or first digit of address ("0" to "9")
  if (state == 10) {
    if (c == ' ') {
      // space -> go to state 11 and get next char
      state=11;

      // echo back char
      Serial.write(c);

      // get next char
      continue;
    } else if ((c >= '0') && (c <= '9')) {
      // digit -> first digit of address. Go to state 2 and process
      state=11;

      // init freqsize;
      freqsize=0;

      // continue to state 2 without fetching next char
    } else {
      // error: echo "bell"
      Serial.write(bell);

      // get next char
      continue;
    }; // end else - if
  };  // end state 10


  // state 11: freq. Mhz part: 3 digits needed
  if (state == 11) {
    if ((c >= '0') && (c <= '9')) {
      if (freqsize < 3) {
        freq1 *= 10;
        freq1 += (c - '0');
               
        freqsize++;
        Serial.write(c);
               
        // go to state 12 (wait for space) if 3 digits received
        if (freqsize == 3) {
          state=12;
        }; // end if
      } else {
        // too large: error
        Serial.write(bell);
      }; // end else - if
    } else {
      // unknown char: error
      Serial.write(bell);
    }; // end else - if
          
    // get next char
    continue;
          
  }; // end state 11
         

  // state 12: space between freq part 1 and freq part 2            
  if (state == 12) {
    if (c == ' ') {
      // space received, go to state 13
      state = 13;
      Serial.write(c);
                
      // reinit freqsize;
      freqsize=0;                
    } else {
      // unknown char
      Serial.write(bell);
    }; // end else - if

    // get next char
    continue;

  }; // end state 12;
               
  // state 13: part 2 of freq. (100 Hz part). 4 digits needed
  if (state == 13) {
    if ((c >= '0') && (c <= '9')) {
      if (freqsize < 4) {
        freq2 *= 10;
        freq2 += (c - '0');
               
        freqsize++;
        Serial.write(c);
      } else {
        // too large: error
        Serial.write(bell);
      }; // end else - if
            
      // get next char
      continue;
            
    } else if (c == '@') {
      if (freqsize == 4) {
        // 4 digits received, done
        state = -2;
        //Serial.println("0a 0d is bad");

        // break out
        break;                
      } else {
        // not yet 3 digits
        Serial.write(bell);
                
        // get next char;
        continue;
      }; // end else - if
              
    } else {
      // unknown char
      Serial.write(bell);
             
      // get next char
      continue;
    }; // end else - elsif - if

  }; // end state 12;

}; // end while


// Function "P": Send PAGE
if (state == -1) {
  /*
  Serial.print("address: ");
  Serial.println(address);

  Serial.print("addresssource: ");
  Serial.println(addresssource);

  Serial.print("repeat: ");
  Serial.println(repeat);

  Serial.print("message: ");
  Serial.println(textmsg);
*/
  // create pocsag message
  // batch2 option = 0 (truncate message)
  // invert option1 (invert)

  rc=pocsag.CreatePocsag(address,addresssource,textmsg,0,1);


  if (!rc) {
    Serial.print("Error in createpocsag! Error: ");
    Serial.println(pocsag.GetError());

    // sleep 10 seconds
    delay(10000);
  } else {

    // send at least once + repeat
    for (int l=-1; l < repeat; l++) {
      Serial.println("SEND");
      digitalWrite(ptt, HIGH);
      rf24.send((uint8_t *)pocsag.GetMsgPointer(), pocsag.GetSize());
      //Serial.println(pocsag.GetMsgPointer());
      rf24.waitPacketSent();
      
      delay(3000);
      digitalWrite(ptt, LOW);
    }; // end for

  }; // end else - if; 
}; // end function P (send PAGE)

// function "F": change frequency

if (state == -2) {
  float newfreq;
  
  newfreq=((float)freq1)+((float)freq2)/10000.0F; // f1 = MHz, F2 = 100 Hz
  
  if  ((newfreq >= 119.000F) && (newfreq <= 999.9F)){
    Serial.println("new freq saved");
    Serial.println(newfreq,4);
    freq=newfreq;
    rf24.setFrequency(freq, 0.00); // set frequency, AfcPullinRange not used (receive-only)
    EEPROM.put(eeAddr,newfreq);
    
  } else {
    Serial.print("Error: invalid frequency (should be 119-999.9 Mhz) ");
    Serial.println(newfreq);
  }; // end if

}; // end function F (frequency)

}; // end main application





