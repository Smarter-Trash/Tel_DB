#include <Arduino.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  3 // three columns

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte pin_rows[ROW_NUM] = {18, 5, 17, 16}; // GIOP18, GIOP5, GIOP17, GIOP16 connect to the row pins
byte pin_column[COLUMN_NUM] = {4, 0, 2};  // GIOP4, GIOP0, GIOP2 connect to the column pins

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    for(;;);
  }

  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  //display.setCursor(0, 10);
  //display.println("Hello, world!");
  //display.display(); 
}

int count_tel = -1;
int x = 5;
char key;

void loop() {
  display.setCursor(25, 25);
  display.println("Welcome");
  display.display(); 
  delay(5000);
  display.clearDisplay();
  display.display();
  while (1){
    key = keypad.getKey();
    Serial.print("count: ");
    Serial.println(count_tel);
    Serial.print("key: ");
    Serial.println(key);
    if (key != NO_KEY) {
      if (key == '*' && count_tel == -1) {
        count_tel = 0;
      }else if (key == '#' && count_tel == 10) {
        x = 5;
        count_tel = -1;
        delay(2000);
        display.clearDisplay();
        display.display();
        break;
      }else if (count_tel >= 0 && key != '#' && key != '*'){
        Serial.println(count_tel);
        display.setCursor(x, 25);
        x = x + 12;
        display.println(key);
        count_tel++;
        display.display();
      }
    }
  }
}