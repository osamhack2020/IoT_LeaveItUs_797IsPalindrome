#include <SoftwareSerial.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

#include <SPI.h>              // include libraries


byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; //부대별 고유 mac 주소 설정

char hostname = "gcp.kimdictor.kr";
String pathname = "/api/locker/"; //뒤에{라커 UID}/tag or /door형식으로 요청
int SERVER_PORT = 80;
  //connect to HTTP server
EthernetClient client;

SoftwareSerial LoRaSerial(10,3); //RX,TX

String from_lora_uid; //요청하는 보관함의 UID
String from_server_uid; //데이터 업데이트할 보관함
byte UID_length = from_server_uid.length();
bool door; //문열림 여부
String from_lora_return[7]; //반납 여부
String from_server_rfid[7]; //인가받은 휴대폰의 tag
bool check; //제대로 반납됐는지 확인 func enable bit
int from_lora_weight[7]; //반납된 휴대폰의 무게


void setup() {
  Serial.begin(115200);                   // initialize serial
  LoRaSerial.begin(115200);
  while (!Serial);
  
  Serial.print("AT\r\n");
  Serial.print("AT+PARAMETER=10,7,1,7\r\n"); //For less than 3Kms
  //LoRaSerial.print("AT+PARAMETER = 12,3,1,7\r\n"); //For more than 3kms
  Serial.print("AT+BAND=915000000\r\n"); //Bandwidth set to 915MHz
  Serial.print("AT+ADDRESS=1\r\n"); //고정된 주소값: gateway, holder
  Serial.print("AT+NETWORKID=5\r\n"); //게이트웨이와 보관함이 같아야 한다
  
  Serial.println("Initialize Ethernet");
  while(!Ethernet.begin(mac));
}

void loop() {
  httpRequest_serverdata();
  Serial.println();
  delay(5000);
  
  if(Serial.available()){//node에서 데이터 받으면 : 반납여부,무게,개폐
    String Inputbuffer;
    from_lora_uid = Serial.readStringUntil('\n'); //read first
    from_lora_uid = from_lora_uid.substring(12);
    
    Inputbuffer = Serial.readStringUntil('\n');
    if(int index = Inputbuffer.indexOf("RFID"))
      for(byte i = 0; i < 7; i++){
        from_lora_return[i] = Inputbuffer[index+i+4];
      }
    
    Inputbuffer = Serial.readStringUntil('\n');
    if(int index = Inputbuffer.indexOf("Check"))
      for(byte i = 0; i < 7; i++){
        from_lora_weight[i] = Inputbuffer[index+i+5];
      }
    
    Inputbuffer = Serial.readStringUntil('\n');
    if(int index = Inputbuffer.indexOf("Door"))
      door = Inputbuffer[index+4];
  }

  else{//수신중이 아닐때: node로 데이터 분배
    LoRaSerial.println("AT+SEND=1,"+ (String)UID_length + ',' + from_server_uid);
    LoRaSerial.println("AT+SEND=1,69,");
    for(byte i = 0; i < 7; i++){
      LoRaSerial.print(from_server_rfid[i]);//8byte UID * 7 + "Update RFtag:"
    }
    LoRaSerial.println();
    LoRaSerial.println("AT+SEND=1,5,Check");  
    
  }
  
  httpRequest_nodedata();
  Serial.println();
  delay(5000);
  
}

void httpRequest_nodedata(){//보관함 업데이트 사항 서버에 올림
  
  String jsondata = "";

  StaticJsonDocument<200> jsonDoc;
  JsonObject outdoc = jsonDoc.to<JsonObject>();
  outdoc["uid"] = from_lora_uid;
  outdoc["door"] = door;
  for(byte i = 0; i < 7; i++){
    outdoc["RFID"].add(from_lora_return[i]);
    outdoc["Phone Status"].add((String)from_lora_weight[i]);
  }
  
  serializeJsonPretty(outdoc, jsondata);
  //Serial.println(jsondata);
  

  if(!client.connect(hostname, SERVER_PORT)){
    Serial.println(F("Connection failed"));
    return;
  }
  
  Serial.print(F("Connected!"));
  
  //Send HTTP request
  client.print("POST "); client.print(pathname); 
  client.print(from_lora_uid);client.println(" HTTP/1.0");
  
  client.print("HOST:"); client.println(String(hostname));
  client.println("Connection: close");
  client.println(jsondata);
  client.println(F(""));
  
  client.flush();
  client.stop();//client connection stop
  
}


void httpRequest_serverdata(){//서버로부터 업데이트 사항 받아옴

  Serial.println("get server data");
  while(!client.connect(hostname,SERVER_PORT));
  Serial.println(F("Connected"));
  
  client.println("GET " + pathname + " HTTP/1.0");
  client.println("Host: "+ String(hostname));
  client.println("Connection: close");
  if(client.read() == 0){
    Serial.println(F("Failed to send request"));
    return;
  }
  client.println();
  
  while(client.available()){
    String jsondata = client.readString();
  
    StaticJsonDocument<200> jsonDoc;
    deserializeJson(jsonDoc, jsondata);

    copyArray(jsonDoc["RFID"], from_server_rfid);
    from_server_uid = jsondata["uid"];//업데이트 대상 보관함의 UID
  }
    
}
