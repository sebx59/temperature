#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define VERSION "v0.4"

//Définition des délais de répétition
#define LOOP_DELAY_RUN 5000
#define LOOP_DELAY_MENU 50

//Paramétrage de l'écran OLED
#define OLED_ADDRESS 0x3C
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 64
#define LOGO16_GLCD_WIDTH 128

// Definitions du HTU21
#define HTDU21D_ADDRESS 0x40  //Unshifted 7-bit I2C address for the sensor
#define TRIGGER_TEMP_MEASURE_HOLD 0xE3
#define TRIGGER_TEMP_MEASURE_NOHOLD 0xF3
#define SOFT_RESET 0xFE

//Définition des boutons
#define BTN_MENU 6
#define BTN_UP 3
#define BTN_DOWN 4
#define BTN_LEFT 5
#define BTN_RIGHT 2

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

//Définition du menu

#define MENU_ITEM_LOOP_DELAY 0
#define MENU_ITEM_DEBUG 1
#define MENU_ITEM_PROBE_DELAY 2

volatile bool menu = false;
int currentMenuItem = 0;
unsigned long paramValues[] = {LOOP_DELAY_RUN, 1, 60};
String paramLabels[] = {"Intervalle", "Mode Debug", "Délai sonde"};
float lastTemperature = 0.0;
int loop_count = 0;

void setup()   {
//  Serial.begin(9600);
  display.begin(0, OLED_ADDRESS);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.setTextColor(WHITE);
  display.clearDisplay();   // clears the screen and buffer   // Efface l'écran

  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print(F("Initialisation..."));
  display.display();

  Wire.begin();

  softReset();

  pinMode(BTN_MENU, INPUT);
  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_LEFT, INPUT);
  pinMode(BTN_RIGHT, INPUT);
  attachInterrupt(digitalPinToInterrupt(BTN_RIGHT), enterMenu, LOW);
}

void enterMenu() {

  menu = true;
//  Serial.println("Bouton droit appuyé");
}


void loop() {


  if (menu) {

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(F("Pincab cooler "));
    display.println(VERSION);
    display.println(F("Menu"));
    display.display();

    int btnUp = digitalRead(BTN_UP);
    int btnDown = digitalRead(BTN_DOWN);
    int btnLeft = digitalRead(BTN_LEFT);
    int btnRight = digitalRead(BTN_RIGHT);
    int btnEnter = digitalRead(BTN_MENU);

    delay(LOOP_DELAY_MENU);

  } else {

//    Serial.println(digitalRead(BTN_RIGHT));

    unsigned int rawTemperature = htdu21d_readTemp();
    if (rawTemperature != 998) {
     
      lastTemperature = -46.85 + (175.72 * (rawTemperature / (float)65536)); //From page 14
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(F("Pincab cooler "));
    display.println(VERSION);

    display.println(" ");

    if (paramValues[MENU_ITEM_DEBUG] == 1) {
      display.print(F("Raw Temp. : "));
      display.println(rawTemperature);
    }
    display.print(F("Temp. : "));

    display.print(lastTemperature, 1);
    display.println(F(" C"));
    display.display();

    delay(paramValues[MENU_ITEM_LOOP_DELAY]);
  }
}

unsigned int htdu21d_readTemp()
{
  //Request the temperature
  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(TRIGGER_TEMP_MEASURE_NOHOLD);
  Wire.endTransmission();

  //Wait for sensor to complete measurement
  delay(paramValues[MENU_ITEM_PROBE_DELAY]); //44-50 ms max - we could also poll the sensor

  //Comes back in three bytes, data(MSB) / data(LSB) / CRC
  Wire.requestFrom(HTDU21D_ADDRESS, 3);

  //Wait for data to become available
  int counter = 0;
  while (Wire.available() < 3)
  {
    counter++;
    delay(1);
    if (counter > 100) return 998; //Error out
  }

  unsigned char msb, lsb, crc;
  msb = Wire.read();
  lsb = Wire.read();
  crc = Wire.read(); //We don't do anything with CRC for now

  unsigned int temperature = ((unsigned int)msb << 8) | lsb;
  temperature &= 0xFFFC; //Zero out the status bits but keep them in place

  return temperature;
}

void softReset() {

  Wire.beginTransmission(HTDU21D_ADDRESS);
  Wire.write(SOFT_RESET);
  Wire.endTransmission();
}

