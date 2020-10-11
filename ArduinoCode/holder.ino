#include <Arduino_FreeRTOS.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "FSR.h"
#include "RFID.h"

// Pin Definitions
#define FSR_1_PIN_1	A10
#define FSR_2_PIN_1	A0
#define FSR_3_PIN_1	A3
#define FSR_4_PIN_1	A13
#define FSR_5_PIN_1	A11
#define FSR_6_PIN_1	A7
#define FSR_7_PIN_1	A8
#define DOORSWITCH	2
#define RFID_PIN_RST	3
#define RFID_PIN_SDA	53

// Global variables and defines
String InSerialStr = "";
// object initialization
//HardwareSerial& InSerialStr(Serial);
FSR fsr_1(FSR_1_PIN_1);
FSR fsr_2(FSR_2_PIN_1);
FSR fsr_3(FSR_3_PIN_1);
FSR fsr_4(FSR_4_PIN_1);
FSR fsr_5(FSR_5_PIN_1);
FSR fsr_6(FSR_6_PIN_1);
FSR fsr_7(FSR_7_PIN_1);
RFID rfid(RFID_PIN_SDA,RFID_PIN_RST); //pin 53 & 3


const int timeout = 10000;       //define timeout of 10 sec
long time0;

// 세마포어 핸들을 선언
SemaphoreHandle_t sem;
//------------------------------------------------------------------------------
//rfid, fsr, door, lora 수행함

//Thread1 : RFID를 통해 NFC tag 판단
static void vNFC(void * arg){
  while(1){
    // RFID Card Reader - RC522 - Test Code
    //Read RFID tag if present
    String rfidtag = rfid.readTag();
    //print the tag to serial monitor if one was discovered
    rfid.printTag(rfidtag);
  }
}

//Thread2 : FSR를 통해 정식 핸드폰을 제출했는지 판단
static void vFSR(void * argl){
  while(1){
    
  }
}

//Thread3 : 문이 열렸는지 안열렸는지 판단
static void vdoor(void *arg){
  while(1){
    if (Serial.available()){
      InSerialStr = Serial.readStringUntil('\n');
      if(InSerialStr == )
    }
  }
}

//Thread4 : 로라
static void vLoRa(void *arg){
  while(1){
    
  }
}

void setup() 
{
    Serial.begin(115200);
    while (!Serial); // wait for serial port to connect. Needed for native USB
    Serial.print("AT\r\n");
    //delay function is needed

    Serial.print("AT+PARAMETER=10,7,1,7\r\n"); //For less than 3Kms

    Serial.print("AT+BAND=915000000\r\n"); //Bandwidth set to 915MHz
    //delay()

    Serial.print("AT+ADDRESS=1\r\n"); //고유 주소를 가져야 한다. 부대에 따른 코드 부여: 0~65535
    //delay()

    Serial.print("AT+NETWORKID=5\r\n"); //허브와 보관함이 같아야 한다. 부대 내의 생활관 번호 부여: 0~16
    //delay()

    //initialize RFID module
    rfid.init();
}

void loop() 
{
    if (Serial.available())
    {
    InSerialStr = Serial.readStringUntil('\n');
    }

    
    else if(menuOption == '2') {
    float fsr_1Force = fsr_1.getForce();
    Serial.print(F("Force: ")); Serial.print(fsr_1Force); Serial.println(F(" [g]"));

    }
   

    
}

char menu()
{
    while (!Serial.available());

    // Read data from serial monitor if received
    while (Serial.available()) 
    {
        char c = Serial.read();
        if (isAlphaNumeric(c)) 
        {   

            else
            {
                Serial.println(F("illegal input!"));
                return 0;
            }
            time0 = millis();
            return c;
        }
    }
}
