#include <MFRC522.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "FSR.h"

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
FSR fsr_1(FSR_1_PIN_1);
FSR fsr_2(FSR_2_PIN_1);
FSR fsr_3(FSR_3_PIN_1);
FSR fsr_4(FSR_4_PIN_1);
FSR fsr_5(FSR_5_PIN_1);
FSR fsr_6(FSR_6_PIN_1);
FSR fsr_7(FSR_7_PIN_1);
MFRC522 rfid(RFID_PIN_SDA,RFID_PIN_RST); //pin 53 & 3

SoftwareSerial LoRaSerial(RX, TX);
int PhoneStatus[3][7]; //O/X , weight, returned or not
String RegisteredRFtag[7]; //string of registered RFtag
String UID_from_rfid;
byte door;
// const int timeout = 10000;       //define timeout of 10 sec
// long time0;

// 세마포어 핸들을 선언합니다.
SemaphoreHandle_t sem;
//------------------------------------------------------------------------------
//rfid, fsr, door, lora 수행함

//Thread1 : RFID를 통해 NFC tag 판단
static void vNFC(void * arg){
  while(1){
    // if(InSerialStr == "Reset RFID"){
      
    // }
    if ( ! rfid.PICC_IsNewCardPresent()) //새로운 것 들어올떄만 다음으로 넘어감
      return;
        
    for (byte i = 0; i < rfid.uid.size; i++){ //uid 저장
     UID_from_rfid.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
     UID_from_rfid.concat(String(rfid.uid.uidByte[i], HEX));
    }
    UID_from_rfid.toUpperCase();
    for(byte i = 0; i < 7; i++){
      if(PhoneStatus[1][i] == UID.substring(1)){//일치하는 폰 찾음
        if(PhoneStatus[2][i] == 0){ //불출된 상태이면
          PhoneStatus[2][i] = 1; //반납처리됨
        }
        else{
          PhoneStatus[2][i] = 0; //불출처리함
        }
      }
    }
  }
}

//Thread2 : FSR를 통해 정식 핸드폰을 제출했는지 판단
static void vCheck(void * argl){
  while(1){
    for(byte i = 0; i < 7; i++){
      if(PhoneStatus[])
    }
  }
}

//Thread3 : 문이 열렸는지 안열렸는지 판단
static void vdoor(void *arg){
  pinMode(DOORSWITCH, INPUT); //2번에서 값 읽어옴
  door = digitalRead(DOORSWITCH); //High : closed low: open
  while(1){ //AT+SEND=<Address>,<Payload Length>,<Data>
      if (LoRaSerial.available()){
        LoRaSerial.print(door);
        if(LoRaSerial.read() == 1)
      if(InSerialStr == )
    }
  }
}

//Thread4 : 로라
static void vLoRa(void *arg){
  while(1){
    if(LoRaSerial.available()){
      PhoneStatus
    }
  }

void setup() 
{
    Serial.begin(115200);
    LoRaSerial.begin(115200);
    while (!Serial); // wait for serial port to connect. Needed for native USB
    while (!Serial);
    Serial.print("AT\r\n");
    //delay function is needed

    Serial.print("AT+PARAMETER=10,7,1,7\r\n"); //For less than 3Kms
    //LoRaSerial.print("AT+PARAMETER = 12,3,1,7\r\n"); //For more than 3kms
    Serial.print("AT+BAND=915000000\r\n"); //Bandwidth set to 915MHz
    //delay()

    Serial.print("AT+ADDRESS=1\r\n"); //고유 주소를 가져야 한다. 부대에 따른 코드 부여: 0~65535
    //delay()

    Serial.print("AT+NETWORKID=5\r\n"); //허브와 보관함이 같아야 한다. 부대 내의 생활관 번호 부여: 0~16
    //delay()

    //initialize RFID module
    SPI.begin();
    rfid.PCD_Init();
    
    int PhoneStatus[2][7] //WEB에서 값읽어와서 업데이트해줘야함
    String RegisteredRFtag[7] //WEB에서 읽어와야 함
}

void loop() 
{
    if (Serial.available())
    {
    InSerialStr = LoRaSerial.readStringUntil('\n');
    }

    
    float fsr_1Force = fsr_1.getForce();
    Serial.print(F("Force: ")); Serial.print(fsr_1Force); Serial.println(F(" [g]"));
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

