#include <Arduino.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include <iostream>
#include <map>

using namespace std;

int count_tel = 0;
char tel_num[10];
int x = 5; //cursor for OLED
int mode = 0;
int debt = 0;

std::map<char*, int> tel_debt_DB;
std::map<char*, int>::iterator it ;

char key;
#define ROW_NUM     4 // four rows
#define COLUMN_NUM  3 // three columns
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte pin_rows[ROW_NUM] = {18, 5, 17, 16}; // GIOP18, GIOP5, GIOP17, GIOP16 connect to the row pins
byte pin_column[COLUMN_NUM] = {4, 0, 2};  // GIOP4, GIOP0, GIOP2 connect to the column pins
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Commu Part
uint8_t NinaAddress[] = {0x3C, 0x61, 0x05, 0x03, 0x42, 0x70};
uint8_t ViewAddress[] = {0xA4, 0xCF, 0x12, 0x8F, 0xCA, 0x28};
uint8_t PatAddress[] = {0x3C, 0x61, 0x05, 0x03, 0x68, 0x74};
uint8_t FaiiAddress[] = {0x24, 0x6F, 0x28, 0x25, 0x86, 0xDC};

typedef struct have_bebt {
  int state;
  int debt;
}have_bebt;

typedef struct state {
  int state;
}state;

have_bebt recv_debt;
have_bebt send_debt;
state send_end;
state recv_7;

esp_now_peer_info_t peerInfo1;
esp_now_peer_info_t peerInfo2;

bool compareMac(const uint8_t * a, uint8_t * b){
  for(int i=0;i<6;i++){
    if(a[i]!=b[i])
      return false;    
  }
  return true;
}

void get_tel_num(){
  Serial.println("ssss");
  count_tel = 0;
  x = 5; //cursor for OLED

  while (1){
    key = keypad.getKey();
    if (key != NO_KEY) {
      if (key != '#' && key != '*' && count_tel != 10){
        display.setCursor(x, 25);
        x = x + 12;
        display.println(key);
        display.display();
        tel_num[count_tel]=key;
        count_tel++;
        Serial.println(count_tel);
      }

      else if (key == '#' && count_tel != 0){
        x = x - 12;
        display.setTextColor(BLACK);
        display.setCursor(x, 25);
        count_tel--;
        display.println(tel_num[count_tel]);
        display.display();
        display.setTextColor(WHITE);
        tel_num[count_tel]=' ';
        Serial.println(count_tel);
      }

      else if(key == '*' && count_tel == 10){
        display.clearDisplay();
        display.display();
        Serial.print(tel_num);
        break;
      }
    }
    
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);

  if(compareMac(mac_addr,FaiiAddress)){
    memcpy(&recv_debt, incomingData, sizeof(recv_debt));
    if(recv_debt.state == 6){
      printf("state6\n");
      mode = 6;
    }

    else if(recv_debt.state == 10){
      printf("state10\n");
      mode = 10;
    }

  }

  else if(compareMac(mac_addr,NinaAddress)){
    printf("state7\n");
    memcpy(&recv_7, incomingData, sizeof(recv_7));
    mode = 7;
  }
}

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    for(;;);
  }
  delay(500);
  display.clearDisplay();
  display.display();

  display.setTextSize(2);
  display.setTextColor(WHITE);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(peerInfo1.peer_addr, NinaAddress, 6);
  peerInfo1.channel = 0;  
  peerInfo1.encrypt = false;

  memcpy(peerInfo2.peer_addr, FaiiAddress, 6);
  peerInfo2.channel = 0;  
  peerInfo2.encrypt = false;
        
  if (esp_now_add_peer(&peerInfo1) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  if (esp_now_add_peer(&peerInfo2) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  if (mode == 6){
    get_tel_num ();
    debt = recv_debt.debt;
    it = tel_debt_DB.find(tel_num);

    if(it == tel_debt_DB.end()){
        tel_debt_DB[tel_num]=debt;
    }
    else{ 
        it->second = it->second+debt;
        debt = it->second;
    }

    send_debt.state = 5;
    send_debt.debt = recv_debt.debt;
    esp_err_t result = esp_now_send(NinaAddress, (uint8_t *) &send_debt, sizeof(send_debt));
    if (result == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }

    mode = 0;
  }

  else if (mode == 7){
    get_tel_num ();
    it = tel_debt_DB.find(tel_num);

    if(it == tel_debt_DB.end()){
        tel_debt_DB[tel_num]=0;
        debt = 0;
    }
    else
        debt = it->second;

    send_debt.state = 8;
    send_debt.debt = debt;
    esp_err_t result1 = esp_now_send(FaiiAddress, (uint8_t *) &send_debt, sizeof(send_debt));
    if (result1 == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }

    esp_err_t result2 = esp_now_send(NinaAddress, (uint8_t *) &send_debt, sizeof(send_debt));
    if (result2 == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }

    mode = 0;

  }

  else if (mode == 10){
    it->second = recv_debt.debt;

    send_debt.state = 10;
    send_debt.debt = recv_debt.debt;

    esp_err_t result = esp_now_send(NinaAddress, (uint8_t *) &send_debt, sizeof(send_debt));
    if (result == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }
    mode = 0;
  }
}