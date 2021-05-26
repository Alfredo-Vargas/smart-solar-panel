#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include "symbols.h"
#include <SimpleDHT.h>
#include "SR04.h"

#define T1 5000             // miliseconds between measurements
#define T2 10               // milisecond delay for every step degree rotation
#define T3 3000             // display time, time given to the user to read the measurements
#define dt 2000             // time interval where the LCD output will be refreshed

#define analogPin0 A0       // pin for LDR1
#define analogPin1 A1       // pin for LDR2
#define lux_sensitivity 50  // trigger of the servo motor

#define servoPin 3          // pin for the servo motor

#define dht11Pin 2          // pin for temperature/humidity sensor

#define echoPin 11          // pin for the echo of the ultrasound sensor
#define trigPin 12          // pin for the trigger of the ultrasound sensor
#define min_distance 30     // trigger distance to take measurements and display it


/* Initialization of modules*/
Servo servo;
LiquidCrystal_I2C lcd(0x27,16,2);
SimpleDHT11 dht11;
SR04 sr04 = SR04(echoPin, trigPin);


/*Data type elements relevant for LDR's*/
const double m = -1.42;   // m = -1.4059;
const double b = pow(10.0, 6.8); // 1.25 * pow(10.0, 7)  
const double R = 10000;
double R0, R1, V0, V1, lux0, lux1;         // R1 resitance at the LDR, V2 voltage at R2, lux value measured
double lux_diff, lux_avg, w;               // rotation direction: w = 1 clockwise, w = 0 counterclockwise
byte current_angle = 90;
byte rotation_step = 1;

/*Data type elements relevant for LCD */
String lcd_lux_avg;
String blank_line = "                ";     // string of 16 spaces
unsigned long startTime; 
unsigned long currentTime;
unsigned long elapsedTime;

/*Data type elements relevant for the UART */
byte temp_units = 'c';                // 0 for celsius and 1 for farenheit
String input;

/*Data type elements relevant for DHT11*/
byte temperature = 0;
byte humidity = 0;
byte data[40] = {0};

/*Data type elements relevant for the HC-SR04*/
long distance;


void setup() {
  /* LDR setup */
  pinMode(analogPin0, INPUT);     // LDR input
  pinMode(analogPin1, INPUT);     // LDR input

  /* Servo Initialization */
  servo.attach(servoPin);         // initialize servo
  servo.write(current_angle);     // restart servo position

  /*LCD setup*/
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.createChar(0, waterDrop);
  lcd.createChar(1, termometer);
  lcd.createChar(2, degrees);
  lcd.createChar(3, lightbulb1);
  lcd.createChar(4, lightbulb2);

  /*LCD opening screen*/
  lcd.setCursor(0,0);
  lcd.print("Smart Technology");
  lcd.setCursor(0,1);
  lcd.print("LightScanner");
  delay(T1);
  startTime = millis();

  /*UART setup*/                  //  UART: Universal Asynchronous Receiver/Transmitter
  Serial.begin(9600);
  
  /*Welcome message*/
  Serial.println(F("----------------------------------------------------------------------"));
  Serial.println(F("Type 'f' or c' to show the temperature in Fahrenheits or Celsius"));
  Serial.println(F("----------------------------------------------------------------------"));
}

/*FUNCTION DECLARATIONS*/
double luxMeter(double R1){                 // calculates the lux given the resistance
  double lux;
  lux = b * pow(R1, m);
  return lux;
}

void deleteCarriageReturn(byte line){       // deletes a line and puts back the cursor at the beginning
  lcd.setCursor(0, line);
  lcd.print(blank_line);
  lcd.setCursor(0, line);
}

void printLux(double lux_avg){              // prints the lux avg in the first line of the LCD
  deleteCarriageReturn(0);
  lcd.setCursor(0,0);
  lcd.write(byte(3));
  lcd.write(byte(4));
  lcd_lux_avg = "  " + String(lux_avg) + " lux";
  lcd.print(lcd_lux_avg);
}

void printTempHum(byte temp, byte hum, byte temp_units){          // prints the temperature and humidity in the second line of the LCD
  String temper;
  deleteCarriageReturn(1);
  if (temp_units == 'c')
    temper = String(temp) + " C  ";
  else if (temp_units == 'f') 
    temper = String(temp) + " F  ";
  if (temp_units != 'c' && temp_units != 'f'){
    temper = String(temp) + " C  ";
    Serial.println(blank_line);
    Serial.println("Please type only 'c' or 'f'.");
    Serial.println("Temperature values on LCD in Celsius (default)");
    Serial.println(blank_line);
  }

  lcd.setCursor(0, 1);
  lcd.write(byte(1));
  lcd.print(temper);
  lcd.write(byte(0));
  lcd.print(hum);
}


void loop() {
/*UART user input code*/
  while(Serial.available() > 0 && (temperature != 0)){
    input = Serial.readStringUntil('\n');
    temp_units = input[0];
  }

/* LDR sensors code*/
  V0 = 5.0 * analogRead(analogPin0) / 1023;
  V1 = 5.0 * analogRead(analogPin1) / 1023;

  R0 = V0 * R / (5 - V0);
  R1 = V1 * R / (5 - V1);

  lux0 = luxMeter(R0);
  lux1 = luxMeter(R1);

  lux_diff = abs(lux0 - lux1);

  lux_avg = (lux0 + lux1) / 2; 
  if (lux_diff >= lux_sensitivity){
      w = lux0 > lux1 ? 0 : 1;
      if (w == 1 && current_angle > rotation_step + 1)
          current_angle -= rotation_step;
      else if (w == 0 && current_angle < 179 - rotation_step)
          current_angle += rotation_step;
      
      if ((w == 1 && current_angle > rotation_step + 1) || (w == 0 && current_angle < 179 - rotation_step)){
          servo.write(current_angle);
          delay(T2);
      }
  }

  /*LCD code */
  distance = sr04.Distance();
  currentTime = millis();
  elapsedTime = currentTime - startTime;

  if (distance > min_distance)
    lcd.noBacklight();
  if (elapsedTime > dt && distance < min_distance) {
    /*DHT11 sensor code*/
    dht11.read(dht11Pin, &temperature, &humidity, data);
    if (temp_units == 'f')
      temperature = (byte)(9 * temperature / 8) + 32;     // temp conversion from Celsius to Farenheit
    lcd.backlight();
    printLux(lux_avg);
    printTempHum(temperature, humidity, temp_units);
    startTime = millis();
    delay(T3);
  }
}
