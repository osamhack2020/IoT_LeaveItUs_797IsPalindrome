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
String OutSerialStr = "";

MFRC522 rfid(RFID_PIN_SDA,RFID_PIN_RST); //pin 53 & 3

SoftwareSerial LoRaSerial(18, 19); //RX, TX
int PhoneStatus[3][7]; // 행: 0.금일 영내 여부, 1.무게, 2.반납여부
String RegisteredRFtag[7]; //string of registered RFtag
String UID_from_rfid; //Info read from rfid
byte door; //open or close
byte UsingHourStart; //Web을 통해 불출시간 정할 수 있음
byte UsingHourEnd; //Web을 통해 반납시간 정할 수 있음
byte RoomNum = 12; //생활관 번호 지정해야 함

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
    
    //LoRaSerial.print("AT+SEND=1,13,message from ");
    //LoRaSerial.print(RoomNum);
    //String Message = Gateway.read(); 
    //byte comtype =Message.find(",") + 1;
    //if(comtype == 2/5/7) -> 해당 명령
    LoRaSerial.print("AT+SEND=1,28,"); //1 대신 gateway 주소입력
    Serial.print("LoRa status: Sending message to gateway"); //debugging 용이하도록 시리얼 출력
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
  byte ReturnErr[7] = {0,};
  while(1){
    
    for(byte i = 0; i < 7; i++){
      if(PhoneStatus[0][i]){//반납 대상자이면
        float weight = fsr[i].getForce(); //i번째의 무게 입력 받음
        float ratio = weight/PhoneStatus[1][i];
        if(0.9 > ratio || ratio > 1.1){//휴대폰 케이스 변경, 센서 오차 감안한 오차범위
          ReturnedErr[i] = 1;
        }
      }
    }
    //LoRaSerial.print("AT+SEND=1,13,message from ");
    //LoRaSerial.print(RoomNum);
    LoRaSerial.print("AT+SEND=1,7,");
    Serial.print("Weight check result : ");
    for(byte i=0; i < 7, i++){
      LoRaSerial.print(ReturnedErr[i]);
      Serial.print(ReturnedErr[i]);
    }
    LoRaSerial.println();
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
      Serial.print("door : ");
      Serial.println(door);
      
      //LoRaSerial.print("AT+SEND=1,13,message from ");
      //LoRaSerial.print(RoomNum);
      if(unlockedHolder){
        String messStr += "AT+SEND=1,53,"; //이곳에 gateway address 1 대신 입력
        messStr += "휴대폰 보관함 문이 임의로 열렸습니다." //53byte
        LoRaSerial.println(messStr);
      }  
    }
  }
}

//Thread4 : 로라
static void vLoRa_Receive(void *arg){
  while(1){
    if(LoRaSerial.available()){
      InSerialStr = LoRaSerial.readStringUntil(':'); //Message from the web
      
      if(InSerialStr.startsWith("Update PhoneStatus:")){
        for(byte i = 0; i < 3; i++){
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
      if(InSerialStr.startsWith("Update UsingHourStart:")){ //불출시간
        byte num = LoRaSerial.parseInt();
        if(0 <= num & num <= 24)//correct input
          UsingHourStart = num;
      }
      if(InSerialStr.startsWith("Update UsingHourEnd:")){ //반납시간
        byte num = LoRaSerial.parseInt();
        if(0 <= num & num <= 24)//correct input
          UsingHourEnd = num;
      }
      
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

    LoRaSerial.print("AT+ADDRESS=1\r\n"); //보관함 고유 주소 코드 부여: 0~65535 ex)A부대 B생활관 C보관함 = 12345 
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    LoRaSerial.print("AT+NETWORKID=5\r\n"); //게이트웨이와 보관함이 같아야 한다. 생활관 내에서만 통일하면 됨
    vTaskDelay((500L * configTICK_RATE_HZ) / 1000L);

    //initialize RFID module
    SPI.begin();
    rfid.PCD_Init();
    
    // int PhoneStatus[2][7] //WEB에서 값읽어와서 업데이트해줘야함
    // String RegisteredRFtag[7] //WEB에서 읽어와야 함
    
    xTaskCreate(vNFC,"RFID",128, NULL, 4, &Task_NFC_Handler);
    xTaskCreate(vCheck, "Check", 64, NULL, 1, &Task_Check_Handler);
    xTaskCreate(vDoor, "DoorStatus", 64, NULL, 2, &Task_Door_Handler);
    xTaskCreate(vLoRa_Receive, "LoRaReceive",128, NULL, 3, &Task_LoRa_Rcv_Handler);
    
}

void loop() {} //NO LOOP
