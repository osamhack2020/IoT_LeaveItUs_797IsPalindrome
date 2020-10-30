#include <MFRC522.h>
#include <Arduino_FreeRTOS.h>
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
boolean runEvery(unsigned long interval);

MFRC522 rfid(RFID_PIN_SDA,RFID_PIN_RST); //pin 53 & 3

SoftwareSerial LoRaSerial(18, 19); //RX, TX
int PhoneStatus[2][7]; // 행: 0.무게, 1.반납여부
String RegisteredRFtag[7]; //string of registered RFtag
String UID_from_rfid; //Info read from rfid
byte door; //휴대폰 보관함이 열려있음을 알린다.
bool checkActiv;
String LockerUID = "abcdef";//제조시 보관함 UID를 ASCII 문자열로 EEPROM에 저장
byte UID_length= LockerUID.length(); //UID의 길이/바이트 수


//define task handlers
TaskHandle_t Task_NFC_Handler;
TaskHandle_t Task_Check_Handler;
TaskHandle_t Task_Door_Handler;
TaskHandle_t Task_LoRa_Handler;

//rfid, fsr, door, lora 입출력 수행함
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
  }
}

//Thread2 : FSR를 통해 정식 핸드폰을 제출했는지 판단
static void vCheck(void * arg){
  
  FSR fsr1(FSR_1_PIN_1);
  FSR fsr2(FSR_2_PIN_1);
  FSR fsr3(FSR_3_PIN_1);
  FSR fsr4(FSR_4_PIN_1);
  FSR fsr5(FSR_5_PIN_1);
  FSR fsr6(FSR_6_PIN_1);
  FSR fsr7(FSR_7_PIN_1);
  FSR fsr[7] = {fsr1, fsr2,fsr3,fsr4,fsr5,fsr6,fsr7};
  
  while(1){
    if(checkActiv == 1){ //보관되어 있는 휴대폰의 무게를 검사하기를 활성화시킴
      for(byte i = 0; i < 7; i++){
        float weight = fsr[i].getForce(); //i번째의 무게 입력 받음
        PhoneStatus[0][i] = (int)weight;
      }
      checkActiv = 0;
    }
    
  }
}

//Thread3 : 문이 열렸는지 안열렸는지 판단
static void vDoor(void *arg){
  pinMode(DOORSWITCH, INPUT); //2번에서 값 읽어옴
  
  while(runEvery(5000)){ //5초에 한번 읽어옴
    door = digitalRead(DOORSWITCH); //High : closed low: open
    if(door == 0){ //open
      door = 1;
    }
    else{
      door = 0;
    }
  }
}

//Thread4 : 
static void vLoRa(void *arg){
  String InSerialStr = "";
  while(1){
    if(LoRaSerial.available()){
      String determinator = LoRaSerial.readString();
      while(determinator != LockerUID); //해당 보관함의 UID와 같아야 데이터 수신
      
      InSerialStr = LoRaSerial.readStringUntil(':'); //Message from the web
      if(InSerialStr.startsWith("Update RFtag:")){
        for(byte i = 0; i < 7; i++){
          RegisteredRFtag[i] = LoRaSerial.read();
        }
      }
      
      InSerialStr = LoRaSerial.readStringUntil('\n');
      if(InSerialStr.startsWith("Check")){
        checkActiv = 1; //check 활성화
      }
      
    }
    
    else{ //일괄적으로 모아서 전송
      if (runEvery(random(10000,20000))){ //10sec ~ 20sec 사이에서 임의로 전송
      LoRaSerial.print("AT+SEND=1,11"+(String)UID_length+",LockerUID :");
      LoRaSerial.print(LockerUID);
      
      //RFID를 통한 반납여부 전송
      LoRaSerial.print("AT+SEND=1,32,RFID");
      Serial.print("LoRa status: Sending message to gateway");
      for(byte i = 0; i < 7; i++){ //불출 반납 상황 송신: 불출여부
        LoRaSerial.print(PhoneStatus[1][i]);
        Serial.print(PhoneStatus[1][i]);
      }
      //무게 전송
      LoRaSerial.print("AT+SEND=1,33,Check");
      Serial.print("Weight check result :");
      for(byte i=0; i < 7; i++){
        LoRaSerial.print(PhoneStatus[0][i]);
        Serial.print(PhoneStatus[0][i]);
      }
      //DOOR 전송
      String messStr = {};
      messStr += "AT+SEND=1,5,Door";
      messStr += door;
      LoRaSerial.println(messStr);
      }
    }
  }
}

void setup(){
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
    xTaskCreate(vLoRa, "LoRaReceive",128, NULL, 3, &Task_LoRa_Handler);
    
}

void loop(){} //NO LOOP

boolean runEvery(unsigned long interval){
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  else
    return false;
}
