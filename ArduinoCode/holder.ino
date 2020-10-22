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

MFRC522 rfid(RFID_PIN_SDA,RFID_PIN_RST); //pin 53 & 3

SoftwareSerial LoRaSerial(18, 19); //RX, TX
int PhoneStatus[2][7]; // 행: 0.무게, 1.반납여부
//String RegisteredRFtag[7]; //string of registered RFtag
String UID_from_rfid; //Info read from rfid
byte door; //open or close
bool usingTime;
String LockerUID = ;//보관함 UID를 ASCII 문자열로 EEPROM에 저장

//define task handlers
TaskHandle_t Task_NFC_Handler;
TaskHandle_t Task_Check_Handler;
TaskHandle_t Task_Door_Handler;
TaskHandle_t Task_LoRa_Rcv_Handler;

//rfid, fsr, door, lora 수행함
//Thread1 : RFID를 통해 NFC tag 판단
static void vNFC(void * arg){
  while(1){
    if (rfid.PICC_IsNewCardPresent()){ //새로운 것 들어올떄만 
    
      for (byte i = 0; i < rfid.uid.size; i++){ //uid 저장
        UID_from_rfid.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
        UID_from_rfid.concat(String(rfid.uid.uidByte[i], HEX));
      }
      UID_from_rfid.toUpperCase();
    
      for(byte i = 0; i < 7; i++){
        if(RegisteredRFtag[i] == UID_from_rfid.substring(1)){//일치하는 폰 찾음
          if(PhoneStatus[1][i] == 0){ //불출된 상태이면
            PhoneStatus[1][i] = 1; //반납처리됨
          }
          else{
            PhoneStatus[1][i] = 0; //불출처리함
          }
        }
      }
      
    }
    if (runEvery(random(10000,20000))){ //10sec ~ 20sec 사이에서 임의로 전송
      LoRaSerial.print("AT+SEND=1,11,LockerUID :");
      LoRaSerial.print(LockerUID);
      LoRaSerial.print("AT+SEND=1,28,");
      Serial.print("LoRa status: Sending message to gateway"); //debugging 용이하도록 시리얼 출력
      for(byte i = 0; i < 7; i++){ //불출 반납 상황 송신
        LoRaSerial.print(PhoneStatus[2][i]);
        LoRaSerial.print(" ");
      }
      LoRaSerial.println();
      LoRaSerial.println("AT+SEND=1,11,END");
    }
  }
}

//Thread2 : FSR를 통해 정식 핸드폰을 제출했는지 판단
static void vCheck(void * argl){
  
  FSR fsr1(FSR_1_PIN_1);
  FSR fsr2(FSR_2_PIN_1);
  FSR fsr3(FSR_3_PIN_1);
  FSR fsr4(FSR_4_PIN_1);
  FSR fsr5(FSR_5_PIN_1);
  FSR fsr6(FSR_6_PIN_1);
  FSR fsr7(FSR_7_PIN_1);
  FSR fsr[7] = {fsr1, fsr2,fsr3,fsr4,fsr5,fsr6,fsr7};
  byte ReturnErr[7] = {0,}; //정상 반납 여부를 저장하는 배열
  while(1){
    for(byte i = 0; i < 7; i++){
      float weight = fsr[i].getForce(); //i번째의 무게 입력 받음
      float ratio = weight/PhoneStatus[0][i];
      if(0.9 > ratio || ratio > 1.1){//휴대폰 케이스 변경, 센서 오차 감안한 오차범위
        ReturnedErr[i] = 1;
      }
    }
    if (runEvery(random(10000,20000))){ //10sec ~ 20sec 사이에서 임의로 전송
      LoRaSerial.print("AT+SEND=1,11,LockerUId :");
      LoRaSerial.print(LockerUID);
      LoRaSerial.print("AT+SEND=1,28,");
      Serial.print("Weight check result :");
      for(byte i=0; i < 7, i++){
        LoRaSerial.print(ReturnedErr[i]);
        Serial.print(ReturnedErr[i]);
      }
      LoRaSerial.println();
      LoRaSerial.println("AT+SEND=1,11,END");
    }
  }
}

//Thread3 : 문이 열렸는지 안열렸는지 판단
static void vDoor(void *arg){
  pinMode(DOORSWITCH, INPUT); //2번에서 값 읽어옴
  byte unlockedHolder = 0; //휴대폰 보관함이 열려있음을 알린다.
  
  while(1){
    door = digitalRead(DOORSWITCH); //High : closed low: open
    if(door == 0){ //open
      if(usingTime ==  0) //반납시간 이후이면
        unlockedHolder = 1; //alert
    }
    
    if (runEvery(random(10000,20000))){ //10sec ~ 20sec 사이에서 임의로 전송
      Serial.print("door : ");
      Serial.println(door);
      
      LoRaSerial.print("AT+SEND=1,11,LockerUID :");
      LoRaSerial.print(LockerUID);
      if(unlockedHolder){
        String messStr += "AT+SEND=1,53,"; //이곳에 gateway address 1 대신 입력
        messStr += "휴대폰 보관함 문이 임의로 열렸습니다." //53byte
        LoRaSerial.println(messStr);
      }
      LoRaSerial.println("AT+SEND=1,11,END");
    }
  }
}

//Thread4 : 웹 -> 로라 데이터 전송
static void vLoRa_Receive(void *arg){
  while(1){
    if(LoRaSerial.available()){
      String determinator = LoRaSerial.read();
      while(determinator != LockerUID); //해당 보관함의 UID와 같아야 데이터 수신
      InSerialStr = LoRaSerial.readStringUntil(':'); //Message from the web
      
      if(InSerialStr.startsWith("Update PhoneStatus:")){
        for(byte i = 0; i < 2; i++){
          for(byte j =0; j < 7; j++){
            PhoneStatus[i][j] = LoRaSerial.read();
          } 
        }
      }
      if(InSerialStr.startsWith("Update RFtag:")){
        for(byte i = 0; i < 7; i++){
          RegisteredRFtag[i] = LoRaSerial.read();
        }
      }
      // if(InSerialStr.startsWith("UsingTime:")){
      //   usingTime = LoRaSerial.read(); //1 : 불출시작, 0: 반납시작
      // }
      
    }
  }

void setup() 
{
    Serial.begin(115200);
    LoRaSerial.begin(115200);
    while (!Serial);
    while (!LoRaSerial);
    LoRaSerial.print("AT\r\n");
    
    LoRaSerial.print("AT+PARAMETER=10,7,1,7\r\n"); //For less than 3Kms
    //LoRaSerial.print("AT+PARAMETER = 12,3,1,7\r\n"); //For more than 3kms
    LoRaSerial.print("AT+BAND=915000000\r\n"); //Bandwidth set to 915MHz
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    LoRaSerial.print("AT+ADDRESS=1\r\n"); //고정된 주소값: gateway, holder
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    LoRaSerial.print("AT+NETWORKID=5\r\n"); //게이트웨이와 보관함이 같아야 한다
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    //initialize RFID module
    SPI.begin();
    rfid.PCD_Init();
    
    randomSeed(analogRead(0));
    
    xTaskCreate(vNFC,"RFID",128, NULL, 4, &Task_NFC_Handler);
    xTaskCreate(vCheck, "Check", 64, NULL, 1, &Task_Check_Handler);
    xTaskCreate(vDoor, "DoorStatus", 64, NULL, 2, &Task_Door_Handler);
    xTaskCreate(vLoRa_Receive, "LoRaReceive",128, NULL, 3, &Task_LoRa_Rcv_Handler);
    
}

void loop() {} //NO LOOP
