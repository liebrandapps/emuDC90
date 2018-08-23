#include <RCSwitch.h>



#include <ELECHOUSE_CC1101_RCS_DRV.h>

int logfile[40];
int i = 0;
float lastTime = 0;
boolean capturing = false;
boolean checking = false;
boolean dataIncoming = false;
int PIN_RX=2;
int PIN_TX=3;

int esp; // for ESP8266 & Arduino setting.
RCSwitch mySwitch = RCSwitch();



void setup() {
  Serial.begin(9600);
  Serial.println("Initializing CC1101...");
  esp = 0; 
  ELECHOUSE_cc1101.setESP8266(esp);    // esp8266 & Arduino SPI pin settings. Don´t change this line!
  ELECHOUSE_cc1101.setMHZ(433.92); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
  ELECHOUSE_cc1101.Init(PA10);    // must be set to initialize the cc1101! set TxPower  PA10, PA7, PA5, PA0, PA_10, PA_15, PA_20, PA_30.
  
  
}

void loop() {
  if(Serial.available()) {
      //give time to serial transmission
      delay(100);
      char buffer[40];
      int idx=0;
      while(Serial.available() && idx<40) {
        buffer[idx] = Serial.read();
        if(buffer[idx]=='$') break;
        idx = idx+1;
      }
      buffer[idx]=0;
      switch(buffer[0]) {
        case 'R':
            receiveMode();
            break;
        case 'T':
            sendCommand(buffer+1);
            break;
        case 'D':
            test();
            break;
      }
  }
}

void receiveMode() {
  ELECHOUSE_cc1101.SetRx();    // set Recive on
  Serial.println("Ready to receive....");
  pinMode(PIN_RX, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_RX), handleInterrupt, CHANGE);
}

void handleInterrupt() {
  //Serial.print(".");
  if (!capturing) {  //wenn keine Aufnahme läuft
    if (!checking) {  //wenn nicht gerade auf "Start-Signal" geprüft wird
      if (digitalRead(PIN_RX) == HIGH) {  //wenn Wechsel von LOW nach (jetzt) HIGH
        lastTime = micros();
        checking = true;
      }
    }

    else {    //wenn gerade auf Start-Signal geprüft wird
      if ((micros() - lastTime > 4000) && (digitalRead(PIN_RX) == LOW)) {    //wenn HIGH-Phase länger als 4ms war und wir jetzt LOW sind
        //das war das Start-Signal
        checking = false;
        capturing = true;
        lastTime = micros();
      }

      else {
        //das war nicht das Start-Signal
        checking = false;
      }
    }
  }

  else {  //es läuft eine Aufnahme
    if (!dataIncoming) {  //bisher noch keine Nutzdaten empfangen
      if ((micros() - lastTime > 1000) && digitalRead(PIN_RX) == HIGH) {  //das war die lange LOW-Phase vor Beginn der Übertragung
        dataIncoming = true; //ab jetzt kommen Daten  
        lastTime = micros();
      }
    }

    else {  //jetzt wird es interessant, jetzt kommen die Daten
      //wenn steigene Flanke (also jetzt HIGH)
      if (digitalRead(PIN_RX) == HIGH) {
        //Beginn der HIGH-Phase merken
        lastTime = micros();
      }  

      //wenn fallende Flanke (also jetzt LOW) 
      else if (digitalRead(PIN_RX) == LOW) {
        //=> prüfe wie lange HIGH war
        if (micros() - lastTime > 500) {
          //long
          logfile[i] = 1;
        }

        else {
          //short
          logfile[i] = 0;
        }

        if (i < 39) {
          //solange noch nicht alle Bits empfangen wurden
          i++;
        }

        else {
          //wir sind fertig
          noInterrupts();  //Interrupts aus damit Ausgabe nicht gestört wird
          Serial.println("Empfangene Daten:");
          //Ausgabe als "quad-bit"
          for (i = 0; i <= 38; i = i + 2) {
            if ((logfile[i] == 0) && (logfile[i+1] == 0))
              Serial.print("0");

            else if ((logfile[i] == 0) && (logfile[i+1] == 1))
              Serial.print("F");

            else if ((logfile[i] == 1) && (logfile[i+1] == 0))
              Serial.print("Q");

            else if ((logfile[i] == 1) && (logfile[i+1] == 1))
              Serial.print("1");
          }
          Serial.println();
          i = 0;
          dataIncoming = false;
          capturing = false;
          interrupts();  //Interrupts wieder an
          return;  //und alles auf Anfang
        }
      }

    }
  }
}


void sendCommand(char* cmd) {
    mySwitch.enableReceive(digitalPinToInterrupt(PIN_RX));
    mySwitch.disableReceive();
    ELECHOUSE_cc1101.SetTx(); 
    mySwitch.enableTransmit(PIN_TX);
    mySwitch.setProtocol(4);
    Serial.println(cmd);
    mySwitch.sendQuadState(cmd);
}

void test() {
  //noInterrupts();
  
  mySwitch.enableReceive(digitalPinToInterrupt(PIN_RX));
  mySwitch.disableReceive();
  ELECHOUSE_cc1101.SetTx(); 
  mySwitch.enableTransmit(3);
  mySwitch.setProtocol(4);
  Serial.println("Runter...");
  mySwitch.sendQuadState("FFF110Q000Q00F0F0F0F");
  mySwitch.sendQuadState("FFF110Q000Q00F0F0F1Q");
  
  delay(5000);
  
  //Stopp
  Serial.println("Stopp...");
  mySwitch.sendQuadState("FFF110Q000Q00F0FFFFF");
  
  delay(1000);
  
  //rauf
  Serial.println("Rauf...");
  
  mySwitch.sendQuadState("FFF110Q000Q00F0F0101");
  mySwitch.sendQuadState("FFF110Q000Q00F0F0110");
  
  delay(5000);
  
  //Stopp
  Serial.println("Stopp...");
  mySwitch.sendQuadState("FFF110Q000Q00F0FFFFF");
  
  delay(1000);
  
}
