#include "Arduino.h"
#include "RTClib.h"


// Your GPRS credentials (leave empty, if missing)
const char apn[]      = "TM"; // Your APN                 //CHANGE THIS <-------
const char gprsUser[] = ""; // User                       //CHANGE THIS <-------
const char gprsPass[] = ""; // Password                   //CHANGE THIS <-------
const char thingspeak_WriteAPIKey[] = "XXXXXXXXXXXXXXXX"; //CHANGE THIS <-------
const char thingspeak_Channel[] = "YYYYYYY";              //CHANGE THIS <-------
int anno = 20;                                             
int mese = 7;                                              
int giorno = 10;                                           
int ora = 23;                                              
int minuti = 54;                                           
int secondi = 20;                                          
int fuso = 1;

int annoFirstData = 0;
int meseFirstData = 0;
int giornoFirstData = 0;

char jsonStr[53] = "";
char riga[150] = "";
int passaggi[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int cumulati[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int batt[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int temp[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
String gsmDate = "";
String gsmTime = "";

int totaleGiorno = 0;

int data_length = 0;
byte ore = 24;
boolean firstData = true;

// Configure TinyGSM library
#define SerialAT  Serial1
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   64  // Set RX buffer to 1Kb

// Define the serial console for debug prints, if needed
//#define TINY_GSM_DEBUG Serial
//#define DUMP_AT_COMMANDS

#include <TinyGsmClient.h>
TinyGsm modem(SerialAT);

// Server details
const char server[] = "api.thingspeak.com";

TinyGsmClient client(modem);
const int  port = 80;

// TTGO T-Call pin definitions
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

#define BUZZER               25
#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
#define uS_TO_S_FACTOR 1000000ULL
#define SOUND_PWM_CHANNEL   0
#define SOUND_RESOLUTION    8 // 8 bit resolution
#define SOUND_ON            (1<<(SOUND_RESOLUTION-1)) // 50% duty cycle
#define SOUND_OFF           0                         // 0% duty cycle
esp_sleep_wakeup_cause_t wakeup_reason;

RTC_Millis rtc;
DateTime now;
DateTime dt0;
TimeSpan delta;

const byte numChars = 32;
char receivedChars[numChars] = "";
boolean codeFound = false;

boolean sent = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {



  // Set-up modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  // Set console baud rate
  Serial.begin(57600);
  delay(4000);
  Serial.println(F("Serial OK"));
  gsmDate.reserve(10);
  gsmTime.reserve(12);

  // Keep power when running from battery
  // Wire.begin(I2C_SDA, I2C_SCL);
  // bool   isOk = setPowerBoostKeepOn(1);
  // Serial.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  rtc.adjust(DateTime(anno, mese, giorno, ora, minuti, secondi));

  delay(100);
  now = rtc.now();
  showDate("Start", now);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  lampeggioLed(1);
  Serial.println("Fine setup");
  delay(1000);
  dt0 = DateTime(anno, mese, giorno, ora, minuti + 1, 0);

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {


  now = rtc.now();
  delta = dt0 - now;
  showTimeSpan("delta:", delta);

  Serial.println("Going to sleep now");
  Serial.flush();

  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_enable_timer_wakeup(delta.totalseconds() * uS_TO_S_FACTOR);
  esp_light_sleep_start();

  delay(100);
  print_wakeup_reason();
  while (Serial.available()) {
    Serial.read();
  };
  now = rtc.now();
  showDate("RTC DateTime: ", now);
  int hh = now.hour();
  delay(100);

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) { /// interrupt from PIR
    passaggi[hh] += 1;
    cumulati[hh] += 1;
    Serial.print(F("Passaggi:")); Serial.println(String(passaggi[hh]));
    Serial.print(F("Cumulati:")); Serial.println(String(cumulati[hh]));
    if (hh != 23)
      cumulati[hh + 1] = cumulati[hh];
    tone(BUZZER, 4000, 200);
    delay(1800); //2000 - 200 buzzer

  }

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER ) {  /// interrupt from internal RTC
    Serial.print(F("Passaggi:")); Serial.println(String(passaggi[hh]));
    Serial.print(F("Cumulati:")); Serial.println(String(cumulati[hh]));

    if (firstData) //lampeggia 2 volte, interrupt da RTC
      lampeggioLed(2);

    batt[hh] = 0;

    if (hh == 5 || hh == 8 || hh == 11 || hh == 14 || hh == 17 || hh == 22) {
      //         if (hh !=23 ) {

      passaggi[hh + 1] = 0;
      cumulati[hh + 1] = cumulati[hh];
      ore = hh + 1;
      int r = 0;
      codeFound = false;
      if ( hh == 22) {
        totaleGiorno = cumulati[hh];
        Serial.print(F("TotaleGiorno:")); Serial.println(String(totaleGiorno));
      }
      while (!codeFound && r < 3) {
        Serial.print(F("Send data - Tentative:"));
        Serial.println(r + 1);
        sendData();
        if (!codeFound) {
          delay(20000);
          r++;
        }
      }
      if (codeFound)
        sent = true;
      Serial.println();
    }

    if (hh == 23) {
      totaleGiorno = cumulati[hh];
      Serial.print(F("TotaleGiorno:")); Serial.println(String(totaleGiorno));
      ore = hh + 1;
      int r = 0;
      codeFound = false;
      while (!codeFound && r < 3) {
        Serial.print(F("Send data - Tentative:"));
        Serial.println(r + 1);
        sendData();
        if (!codeFound) {
          delay(20000);
          r++;
        }
      }
      if (codeFound)
        sent = true;
      Serial.println();
      //    giornoSim += 1;
      for (byte i = 0; i < 24; i++) {
        passaggi[i] = 0;
        cumulati[i] = 0;
        batt[i] = 0;
        temp[i] = 0;
      }
    }


    if (hh != 5 && hh != 8 && hh != 11 && hh != 14 && hh != 17 &&  hh != 23 && hh != 22) {
      //         if (hh != 23) {

      passaggi[hh + 1] = 0;
      cumulati[hh + 1] = cumulati[hh];
    }

    now = rtc.now();
    anno = now.year();
    mese = now.month();
    giorno = now.day();
    ora = now.hour();
    minuti = now.minute();
    secondi = now.second();
    if (!sent) {
      rtc.adjust(DateTime(anno, mese, giorno, ora, minuti, secondi));
      now = rtc.now();
    }
    showDate("Now after sending:", now);

    if (hh == 23)
    {
      if (firstData) {
        anno = annoFirstData;
        mese = meseFirstData;
        giorno = giornoFirstData;
        rtc.adjust(DateTime(anno, mese, giorno, ora, minuti, secondi));
        now = rtc.now();
        hh = now.hour();
      }
      dt0 = DateTime(anno, mese, giorno, 23, 55 , 0);
      dt0 = dt0 + TimeSpan(0, 1, 0, 0);
      int p = 40;
      dt0 = dt0 + TimeSpan(0, 0, 0, p);
    }

    if (hh != 23)
    {
      hh = now.hour();
      if (minuti < 55)
        dt0 = DateTime(anno, mese, giorno, hh, 55 , 0);
      else
        dt0 = DateTime(anno, mese, giorno, hh + 1, 55 , 0);
      int p = 40;
      dt0 = dt0 + TimeSpan(0, 0, 0, p);
    }
    showDate("Dt0:", dt0);
    firstData = false;
    sent = false;
  }
}





void lampeggioLed(byte t) {
  for (byte i = 0; i < t; i++) {
    tone(BUZZER, 2000, 300);
    delay(300);
  }
}

void tone(int pin, int frequency, int duration)
{
  ledcSetup(SOUND_PWM_CHANNEL, frequency, SOUND_RESOLUTION);  // Set up PWM channel
  ledcAttachPin(pin, SOUND_PWM_CHANNEL);                      // Attach channel to pin
  ledcWrite(SOUND_PWM_CHANNEL, SOUND_ON);
  delay(duration);
  ledcWrite(SOUND_PWM_CHANNEL, SOUND_OFF);
}


void print_wakeup_reason() {
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("\nWakeup caused by external signal using RTC");; break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("\nWakeup caused by external signal using PIR"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("\n****************\nWakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}


void showDate(const char* txt, const DateTime& dt) {
  Serial.print(txt);
  Serial.print(' ');
  Serial.print(dt.year(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.day(), DEC);
  Serial.print(' ');
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.print(dt.second(), DEC);


  Serial.println();
}

void showTimeSpan(const char* txt, const TimeSpan& ts) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.print(ts.days(), DEC);
  Serial.print(" days ");
  Serial.print(ts.hours(), DEC);
  Serial.print(" hours ");
  Serial.print(ts.minutes(), DEC);
  Serial.print(" minutes ");
  Serial.print(ts.seconds(), DEC);
  Serial.print(" seconds (");
  Serial.print(ts.totalseconds(), DEC);
  Serial.print(" total seconds)");
  Serial.println();
}
