#include <Time.h>
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

MFRC522 rfid(RFID_PIN_SDA,RFID_PIN_RST); //pin 53 & 3

SoftwareSerial LoRaSerial(18, 19); //RX, TX
int PhoneStatus[3][7]; // 행: 금일 영내 여부, 무게, 반납여부
String RegisteredRFtag[7]; //string of registered RFtag
String UID_from_rfid; //Info read from rfid
byte door; //open or close
byte UsingHourStart; //Web을 통해 불출시간 정할 수 있음
byte UsingHourEnd; //Web을 통해 반납시간 정할 수 있음
byte RoomNum; //생활관 번호 지정됨

//define task handlers
TaskHandle_t Task_NFC_Handler;
TaskHandle_t Task_Check_Handler;
TaskHandle_t Task_Door_Handler;
TaskHandle_t Task_LoRa_Rcv_Handler;
// 세마포어 핸들을 선언합니다.
SemaphoreHandle_t sem;
//------------------------------------------------------------------------------
//rfid, fsr, door, lora 수행함

//Thread1 : RFID를 통해 NFC tag 판단
static void vNFC(void * arg){
  while(1){
    if ( ! rfid.PICC_IsNewCardPresent()) //새로운 것 들어올떄만 다음으로 넘어감
      return;
        
    for (byte i = 0; i < rfid.uid.size; i++){ //uid 저장
     UID_from_rfid.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
     UID_from_rfid.concat(String(rfid.uid.uidByte[i], HEX));
    }
    UID_from_rfid.toUpperCase();
    for(byte i = 0; i < 7; i++){
      if(RegisteredRFtag[i] == UID_from_rfid.substring(1)){//일치하는 폰 찾음
        if(PhoneStatus[2][i] == 0){ //불출된 상태이면
          PhoneStatus[2][i] = 1; //반납처리됨
        }
        else{
          PhoneStatus[2][i] = 0; //불출처리함
        }
      }
    }
    for(byte i = 0; i < 7; i++){ //불출 반납 상황 송신
      LoRaSerial.print(PhoneStatus[2][i]);
      LoRaSerial.print(" ");
    }
    LoRaSerial.println();
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
  byte ReturnErr[7] = {0, };
  while(1){
    float weight;
    //Serial.print(F("Force: ")); Serial.print(fsr_1Force); Serial.println(F(" [g]"));
    
    for(byte i = 0; i < 7; i++){
      if(PhoneStatus[0][i]){//반납 대상자이면
        weight = fsr[i].getForce();
        float ratio = weight/PhoneStatus[1][i];
        if(0.9 > ratio || ratio > 1.1){//휴대폰 케이스 변경, 센서 오차 감안한 오차범위
          ReturnedErr[i] = 1;
        }
      }
    }
  }
}

//Thread3 : 문이 열렸는지 안열렸는지 판단
static void vDoor(void *arg){
  pinMode(DOORSWITCH, INPUT); //2번에서 값 읽어옴
  byte unlockedHolder = 0; //휴대폰 보관함이 열려있음을 알린다.
  
  while(1){ //AT+SEND=<Address>,<Payload Length>,<Data>
    door = digitalRead(DOORSWITCH); //High : closed low: open
    if(door == 0){ //open
      if(hour() < UsingHourStart && hour() > UsingHourEnd)
        unlockedHolder = 1; //alert
    }
    if (LoRaSerial.available()){
      LoRaSerial.print("door : ");
      LoRaSerial.println(door);
      if(unlockedHolder)  
        LoRaSerial.println("휴대폰 보관함 문이 임의로 열렸습니다.")
    }
  }
}

//Thread4 : 로라
static void vLoRa_Receive(void *arg){
  RoomNum = Serial.read();
  while(1){
    if(LoRaSerial.available()){
      PhoneStatus
    }
    InSerialStr = LoRaSerial.readStringUntil('\n');
    
  }

void setup() 
{
    Serial.begin(115200);
    LoRaSerial.begin(115200);
    while (!Serial); // wait for serial port to connect. Needed for native USB
    while (!Serial);
    while(Serial.print("AT\r\n") != "OK");
    
    Serial.print("AT+PARAMETER=10,7,1,7\r\n"); //For less than 3Kms
    //LoRaSerial.print("AT+PARAMETER = 12,3,1,7\r\n"); //For more than 3kms
    Serial.print("AT+BAND=915000000\r\n"); //Bandwidth set to 915MHz
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    Serial.print("AT+ADDRESS=1\r\n"); //고유 주소를 가져야 한다. 부대에 따른 코드 부여: 0~65535
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    Serial.print("AT+NETWORKID=5\r\n"); //허브와 보관함이 같아야 한다. 부대 내의 생활관 번호 부여: 0~16
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    //initialize RFID module
    SPI.begin();
    rfid.PCD_Init();
    
    int PhoneStatus[2][7] //WEB에서 값읽어와서 업데이트해줘야함
    String RegisteredRFtag[7] //WEB에서 읽어와야 함
    
    xTaskCreate(vNFC,"RFID",128, NULL, Priority, &Task_NFC_Handler);
    xTaskCreate(vCheck, "Check", 128, NULL, Priority, &Task_Check_Handler);
    xTaskCreate(vDoor, "DoorStatus", 128, NULL, Priority, &Task_Door_Handler);
    xTaskCreate(vLoRa_Receive, "LoRaReceive",128, NULL, Priority, &Task_LoRa_Rcv_Handler);
    
}

void loop() {} //NO LOOP
