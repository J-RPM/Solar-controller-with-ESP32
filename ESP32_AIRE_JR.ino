/*
                          --- Audio information ---
                   >>> Download: Game_Audio Library <<<
   http://www.buildlog.net/blog/wp-content/uploads/2018/02/Game_Audio.zip
     https://www.buildlog.net/blog/2018/02/game-audio-for-the-esp32/

                  >>> Download: XT_DAC_Audio Library <<<
     https://www.xtronical.com/wp-content/uploads/2018/03/XT_DAC_Audio.zip
 http://www.xtronical.com/basics/audio/digitised-speech-sound-esp32-playing-wavs/
        https://www.xtronical.com/basics/audio/playing-wavs-updated/

                                 --- oOo ---
                                
                              Modified by: J_RPM
                               http://j-rpm.com/
                        https://www.youtube.com/c/JRPM
              (v1.46) Shows the power values of a solar inverter
                UTF-8 Spanish characters and Pac-Man animations
                ESP32_Time_8BCD_JR.ino  >>> ESP32_Solar_JR.ino
                              >>> March of 2023 <<<

              An optional OLED display is added to Januarshow the Time an Date,
                  adding some new routines and modifying others.

                              >>> HARDWARE <<<
                  LIVE D1 mini ESP32 ESP-32 WiFi + Bluetooth
                https://es.aliexpress.com/item/32816065152.html
                
            HW-699 0.66 inch OLED display module, for D1 Mini (64x48)
               https://es.aliexpress.com/item/4000504485892.html

                    MAX7219 8-digit LED 7-segment Serial
               https://es.aliexpress.com/item/32956123955.html

                           >>> IDE Arduino <<<
                        Model: WEMOS MINI D1 ESP32
       Add URLs: https://dl.espressif.com/dl/package_esp32_index.json
                     https://github.com/espressif/arduino-esp32

 ____________________________________________________________________________________
*/
//////////////////////////////////////////////////////////
// Two time zones, voice prompts and Pac-Man animations //
//////////////////////////////////////////////////////////
static String HWversion = "v1.54"; // Control loads, depending on solar production

////////////////////////// INVERTER //////////////////////////////////////////////
static String IPinverter = "192.168.1.112";
static String sendInverter1 = "http://" + IPinverter + "/solar_api/v1/GetPowerFlowRealtimeData.fcgi";
static String sendInverter2 = "http://" + IPinverter + "/solar_api/v1/GetInverterInfo.cgi";
static String spaces8 = "        ";
static String msgErrInverter = "INVERTER ERROR";
static String msgErrWiFi = "WIFI ERROR";

int StatusCode = 7;
int ErrorCode = 0;

int inverter_mode = 2;  // GRID Power for default to matrix display
bool errInverter = false;
bool display_inverter = true;
long clkTimeF = 0;      // Inverter Check Timer
long clkTimeO = 0;      // Timer to Blink over Grid Power
long clkTimeA = 0;      // AIRE Check Timer
float E_Total = 0;      // Total kWh 
float E_Day = 0;        // Day kWh  
int P_PV = 0;           // Solar W 
int P_Load = 0;         // Load W 
int P_Grid = 0;         // Grid W 
bool overGrid = false;  // Status to Blink over Grid Power

// Control loads, depending on solar production
bool Manual_AIRE = true;
bool Status_AIRE = false;
bool Status_BACK = false;
bool AC1 = true;
int Min_AC = 3;         // Minutes between two AC automatic actions
int Cont_Min = 0;       // Minute counter
int AC_ON = -1500;
int AC_OFF = -100;

// AC
#define IN_AIRE  2   
#define OUT_PULSA  14 
String msgAIRE = "GRID";

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <time.h>
#include "LedController.hpp"
#include "Game_Audio.h";
#include "SoundData.h";

Game_Audio_Class GameAudio(26,0); 
Game_Audio_Wav_Class waw0(w0); 
Game_Audio_Wav_Class waw1(w1); 
Game_Audio_Wav_Class waw2(w2); 
Game_Audio_Wav_Class waw3(w3); 
Game_Audio_Wav_Class waw4(w4); 
Game_Audio_Wav_Class waw5(w5); 
Game_Audio_Wav_Class waw6(w6); 
Game_Audio_Wav_Class waw7(w7); 
Game_Audio_Wav_Class waw8(w8); 
Game_Audio_Wav_Class waw9(w9); 
Game_Audio_Wav_Class wawMin(Minutos); 
Game_Audio_Wav_Class wawHor(Horas); 
Game_Audio_Wav_Class wawSon(sonLas); 
Game_Audio_Wav_Class wawTone(Tone); 
Game_Audio_Wav_Class pmEat(Eating); 
Game_Audio_Wav_Class pmDeath(pacmanDeath);
Game_Audio_Wav_Class wawIP(wIP);
Game_Audio_Wav_Class wawPunto(wPunto);

#include <DNSServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <Adafruit_GFX.h>
//*************************************************IMPORTANT******************************************************************
#include "Adafruit_SSD1306.h" // Copy the supplied version of Adafruit_SSD1306.cpp and Adafruit_ssd1306.h to the sketch folder
#define  OLED_RESET 0         // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

String CurrentTime, CurrentDate, nDay, webpage = "";
bool display_EU = true;
int matrix_speed = 25;

static String zone1= "SPAIN";
static String zone2= "JAPAN";
bool T_Zone2 = false;
bool pac = false;
bool alTXT = false;

// Hourly announcements from 10 a.m. to 11 p.m.
int alarm_H = 10;
bool sound_chime = false;
int h,m,s,m2;
int myH;
boolean Hide_OLED = false;
boolean Night_OLED = false;

// Turn on debug statements to the serial output
#define DEBUG  1
#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif

//UART 2
#define RXD2 16
#define TXD2 17
String TxDate="";
String TxTime="";

// ESP32 -> Matrix 
#define DIN_PIN 32 
#define CLK_PIN 27 
#define CS_PIN  25 

// Define the number of bytes you want to access (first is index 0)
#define EEPROM_SIZE 14

////////////////////////// MATRIX //////////////////////////////////////////////
bool _scroll = false;
bool display_date = true;
long clkTime = 0;
int brightness = 5;  //DUTY CYCLE: 11/32
String mDay;
long timeConnect;
String mText;

//////////////////////////////////////////////////////////////////////////////

WiFiClient client;
const char* Timezone    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
                                                           // See below for examples
const char* ntpServer   = "es.pool.ntp.org";               // Or, choose a time server close to you, but in most cases it's best to use pool.ntp.org to find an NTP server
                                                           // then the NTP system decides e.g. 0.pool.ntp.org, 1.pool.ntp.org as the NTP syem tries to find  the closest available servers
                                                           // EU "0.europe.pool.ntp.org"
                                                           // US "0.north-america.pool.ntp.org"
                                                           // See: https://www.ntppool.org/en/                                                           
int  gmtOffset_sec      = 0;    // UK normal time is GMT, so GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
int  daylightOffset_sec = 7200; // In the UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset

WebServer server(80); 

/*
 pin 32 is connected to the DIN 
 pin 27 is connected to the CLK 
 pin 25 is connected to CS 
 We have only a single MAX72XX.
 */
LedController<1,1> lc;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial2.begin(4800, SERIAL_8N1, RXD2, TXD2);
  timeConnect = millis();

  // AC
  pinMode(OUT_PULSA, OUTPUT);     // Output PULSA (ON/OFF)
  digitalWrite(OUT_PULSA, LOW);   // PULSA = OFF
  pinMode(IN_AIRE, INPUT_PULLUP); // Status Input Pull-Up (Status AC: ON/OFF)
  clkTimeA = millis();  
  readConfig();
  load_AC();

  lc=LedController<1,1>(DIN_PIN,CLK_PIN,CS_PIN);

  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.activateAllSegments();
  /* Set the brightness to a medium values */
  lc.setIntensity(brightness);
  /* and clear the display */
  lc.clearMatrix();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15,0);   
  display.println(F("NTP"));

  display.setTextSize(1);   
  display.setCursor(2,16);   
  display.println(F("TIME-INVER"));
  display.setCursor(15,26);   
  display.println(HWversion);

  display.setCursor(10,38);   
  display.println(F("Sync..."));
  display.display();

  mText = spaces8 + "NTP "; 
  if (T_Zone2==false) {
    mText = mText + zone1;    
  }else{
    mText = mText + zone2;    
  }
  mText = mText + "  " + HWversion + " ";
  scrollText();
  delay(1000);

  //------------------------------
  //WiFiManager intialisation. Once completed there is no need to repeat the process on the current board
  WiFiManager wifiManager;
  display_AP_wifi();

  // A new OOB ESP32 will have no credentials, so will connect and not need this to be uncommented and compiled in, a used one will, try it to see how it works
  // Uncomment the next line for a new device or one that has not connected to your Wi-Fi before or you want to reset the Wi-Fi connection
  // wifiManager.resetSettings();
  // Then restart the ESP32 and connect your PC to the wireless access point called 'ESP32_AP' or whatever you call it below
  // Next connect to http://192.168.4.1/ and follow instructions to make the WiFi connection

  // Set a timeout until configuration is turned off, useful to retry or go to sleep in n-seconds
  wifiManager.setTimeout(180);
  
  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "ESP32_AP" and waits in a blocking loop for configuration
  if (!wifiManager.autoConnect("ESP32_AP")) {
    PRINTS("\nFailed to connect and timeout occurred");
    display_AP_wifi();
    display_flash();
    reset_ESP32();
  }
  // At this stage the WiFi manager will have successfully connected to a network,
  // or if not will try again in 180-seconds
  //---------------------------------------------------------
  // 
  PRINT("\n>>> Connection Delay(ms): ",millis()-timeConnect);
  if(millis()-timeConnect > 30000) {
    PRINTS("\nTimeOut connection, restarting!!!");
    reset_ESP32();
  }

  // Print the IP of Inverter and Webpage
  PRINT("\nIPinverter >>> ", IPinverter + "\n");
  PRINT("\nUse this URL to connect -> http://",WiFi.localIP());
  PRINTS("/");
  
  if (sound_chime == true){
    GameAudio.PlayWav(&wawTone, false, 1.0);
    while(GameAudio.IsPlaying()){ }    // wait until done
    delay(100);
    GameAudio.PlayWav(&wawIP, false, 1.0);
    while(GameAudio.IsPlaying()){ }    // wait until done
  }
 
  display_ip();
  if (sound_chime == true) playIP();
  display_flash();

  // Syncronize Time and Date
  SetupTime();

  // Select mode: TIME/MESSAGE
  checkServer();
 
  // Debug first message 
  String stringMsg = "ESP32_Solar_JR " + HWversion + " - RTC: ";
  if (T_Zone2 == false) {
    stringMsg = stringMsg + zone1;    
  }else{
    stringMsg = stringMsg + zone2;    
  }
  stringMsg = stringMsg + " - IP: " + WiFi.localIP().toString() + "\n";
  PRINT("\nstringMsg >>> ", stringMsg + "\n");

  if (sound_chime == true){
    soundTime();
    delay(400);
    soundEnd();
  }

}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void loop() {
  // Wait for a client to connect and when they do process their requests
  server.handleClient();

  // Update and refresh of the date and time on the displays
  if (millis() % 60000) UpdateLocalTime();

  // Load Time and check Alarms
  h = (CurrentTime.substring(0,2)).toInt();
  m = (CurrentTime.substring(3,5)).toInt();
  s = (CurrentTime.substring(6,8)).toInt();

  convert_EU_Time();
  if (sound_chime == true && s <= 5 && _scroll == false){
    checkAlarm();
  }

  //TX RS232 >>> Time & Date 
  if (s > 20 && s < 50 && m2 != m){
    if (m%2 == 1) {
      UpdateLocalTime();
      Serial.write(0x5C);
      Serial2.write(0x5C);
      Serial.println(TxTime);
      Serial2.println(TxTime);
    }else {
      Serial.write(0x5C);
      Serial2.write(0x5C);
      Serial.println(TxDate);
      Serial2.println(TxDate);
    }
    m2 = m;  
  }

  //////////////////////////////////////
  //       Check WiFi IP             //
  //////////////////////////////////////
  if ((WiFi.localIP().toString()) == "0.0.0.0") {
    mText = spaces8 + msgErrWiFi + spaces8 ;
    display_scroll_msg(); 
    reset_ESP32();
    return;
  }   
  //////////////////////////////////////

  //////////////////////////////////////
  //     Inverter Display ON/OFF      //
  //////////////////////////////////////
  if (display_inverter == true){
    oled_power();
    if (s >= 58 || s <= 2){
      matrix_time();
    }else {
      // Interval to refresh SOLAR query
      if(millis()-clkTimeF > 3000) {
        overGrid = false;
        matrix_power();
        getInverter();
        clkTimeF = millis();
        
        // AC check
        // Check error Inverter connection
        if (errInverter == true) {
          mText = spaces8 + msgErrInverter + spaces8 ;
          display_scroll_msg();    
        }else {
          testAire();
        }
      }else {
        matrix_power();
      }
 
      
      // Check Status Code of Inverter
      if ((millis()-clkTime > 30000) && (StatusCode != 7)) { // clock for 30s, then scrolls STATUS - ERROR
        mText = spaces8 + "STATUS " + String(StatusCode) + " - ERROR " + String(ErrorCode) + spaces8;
        display_scroll_msg();    
      }
    }
  }else {
    Oled_Time();
    matrix_time();
  }
  //////////////////////////////////////
  
  //////////////////////////////////////
  //   Show date on matrix display    //
  //////////////////////////////////////
  if (display_date == true) {
    if(millis()-clkTime > 30000) { // clock for 30s, then scrolls for about 5s
      if (alTXT == false) {
        alTXT = true;  
        mText = mDay; 
      }else {
        alTXT = false;  
        if ((WiFi.localIP().toString()) == "0.0.0.0") {
          mText = msgErrWiFi;
        }else {
          mText = "IP: " + WiFi.localIP().toString();
        }
      }
      mText = spaces8 + mText + spaces8 ;
      display_scroll_msg();
    }
  }
  //////////////////////////////////////
  
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readConfig(){
  // Initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);

  // 0 - Display Status 
  display_EU = EEPROM.read(0);
  PRINT("\ndisplay_EU: ",display_EU);

  // 1 - Display Date  
  display_date = EEPROM.read(1);
  PRINT("\ndisplay_date: ",display_date);
  
  // 2 - Matrix Brightness  
  brightness = EEPROM.read(2);
  PRINT("\nbrightness: ",brightness);
  lc.setIntensity(brightness);
  
  // 3 - Display Inverter  
  display_inverter = EEPROM.read(3);
  PRINT("\ndisplay_inverter: ",display_inverter);

  // 4 - Inverter Mode  
  inverter_mode = EEPROM.read(4);
  if (inverter_mode > 2) {
    inverter_mode  = 2;
    EEPROM.write(4, inverter_mode);
  }
  PRINT("\ninverter_mode: ",inverter_mode);

  // 5 - Speed Matrix (delay)  
  matrix_speed = EEPROM.read(5);
  if (matrix_speed < 10 || matrix_speed > 40) {
    matrix_speed = 25;
    EEPROM.write(5, matrix_speed);
  }
  PRINT("\nmatrix_speed: ",matrix_speed);

  PRINT("\nalarm_H: ",alarm_H);


  // 7 - Night_OLED (true/false)
  Night_OLED = EEPROM.read(7);
  if (Night_OLED > 1) {
    Night_OLED = true;
    EEPROM.write(7, Night_OLED);
  }
  PRINT("\nNight_OLED: ",Night_OLED);
 
  
  // 8 - AC1 (true/false)
  AC1 = EEPROM.read(8);
  if (AC1 > 1) {
    AC1 = true;
    EEPROM.write(8, AC1);
  }
  PRINT("\nUmbral AC1: ",AC1);

  
  // 9 - sound_chime (ON/OFF)
  sound_chime = EEPROM.read(9);
  if (sound_chime > 1) {
    sound_chime = true;
    EEPROM.write(9, sound_chime);
  }
  PRINT("\nCarrillon: ",sound_chime);

  // 13 - Time Zone
  T_Zone2 = EEPROM.read(13);
  PRINT("\nT_Zone2: ",T_Zone2);

   // Close EEPROM    
  EEPROM.commit();
  EEPROM.end();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
// See below for examples
// Or, choose a time server close to you, but in most cases it's best to use pool.ntp.org to find an NTP server
// then the NTP system decides e.g. 0.pool.ntp.org, 1.pool.ntp.org as the NTP syem tries to find  the closest available servers
// EU "0.europe.pool.ntp.org"
// US "0.north-america.pool.ntp.org"
// See: https://www.ntppool.org/en/                                                           
// UK normal time is GMT, so GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
// In the UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean SetupTime() {
  char* Timezone;
  char* ntpServer;
  int gmtOffset_sec;
  int daylightOffset_sec;
  
  // Select Time Zone (Spain/Japan)
  if (T_Zone2 == false) {
    Timezone= "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; 
    ntpServer= "es.pool.ntp.org";
    gmtOffset_sec= 0;
    daylightOffset_sec= 7200;
  }else{
    Timezone= "UTC-9";  
    ntpServer= "ntp.nict.jp"; 
    gmtOffset_sec= 0;
    daylightOffset_sec= 0;
  }
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov");  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setenv("TZ", Timezone, 1);                                            // setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset();                                                                    // Set the TZ environment variable
  delay(1000);
  bool TimeStatus = UpdateLocalTime();
  return TimeStatus;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
boolean UpdateLocalTime() {
  struct tm timeinfo;
  time_t now;
  time(&now);

  //See http://www.cplusplus.com/reference/ctime/strftime/
  // %w >>> Weekday as a decimal number with Sunday as 0 (0-6)
  static String txWDay[7] = {"7","1","2","3","4","5","6"};
  static String esWDay[7] = {"DOMINGO","LUNES","MARTES","MIERCOLES","JUEVES","VIERNES","SABADO"};
  static String esMonth[13] = {"ENERO","FEBRERO","MARZO","ABRIL","MAYO","JUNIO","JULIO","AGOSTO","SEPTIEMBRE","OCTUBRE","NOVIEMBRE","DICIEMBRE"};
  String esDate;
  char output[30];
  
  if (T_Zone2 == false && display_EU == true) {
    strftime(output, 30, "%w", localtime(&now));
    mDay = esWDay[atoi(output)];
  }else {
    strftime(output, 30, "%A", localtime(&now));
    mDay = output;
  }
  strftime(output, 30, "%a. ", localtime(&now));
  nDay = output; 

  //TX Sincro Date & Time
  strftime(output, 30, "%w", localtime(&now));
  String dTX = txWDay[atoi(output)];
  strftime(output, 30,"%d%m%y", localtime(&now));
  TxDate = String("sd") + output + dTX;
  strftime(output, 30, "%H%M%S", localtime(&now));
  TxTime = String("st") + output;

  
  if (display_EU == true) {
    strftime(output, 30,"%d-%m", localtime(&now));
    CurrentDate = nDay + output;
    //%m  Month as a decimal number (01-12)
    if (T_Zone2 == false) {
      strftime(output, 30,", %d", localtime(&now));
      esDate = mDay + output;
      strftime(output, 30,"%m", localtime(&now));
      mDay = esDate + " " + esMonth[atoi(output)-1];
      strftime(output, 30," %Y", localtime(&now));
    }else {
      strftime(output, 30,", %d %B %Y", localtime(&now));
    }
    mDay = mDay + output;
    strftime(output, 30, "%H:%M:%S", localtime(&now));
    CurrentTime = output;
  }
  else { 
    strftime(output, 30, "%m-%d", localtime(&now));
    CurrentDate = nDay + output;
    strftime(output, 30, ", %B %d, %Y", localtime(&now));
    mDay = mDay + output;
    strftime(output, 30, "%r", localtime(&now));
    CurrentTime = output;
  }
  return true;
}
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//------------------ OLED DISPLAY -----------------//
/////////////////////////////////////////////////////
void Oled_Time() { 
  display.clearDisplay();
  if (Hide_OLED == false) {
    display.setCursor(2,0);   // center date display
    display.setTextSize(1);   
    display.println(CurrentDate);
  
    display.setTextSize(2);   
    display.setCursor(8,16);  // center Time display
    if (CurrentTime.startsWith("0")){
      display.println(CurrentTime.substring(1,5));
    }else {
      display.setCursor(0,16);
      display.println(CurrentTime.substring(0,5));
    }
    
    if (display_EU == true) {
      display.setCursor(7,33); // center Time display
      if (_scroll) {
        display.print("----");
      }else{
        display.print("(" + CurrentTime.substring(6,8) + ")");
      }
    }else {
      if (_scroll) {
        display.print("----");
      }else{
        display.print("(" + CurrentTime.substring(6,8) + ")");
      }
      display.setTextSize(1);
      display.setCursor(40,33); 
      display.print(CurrentTime.substring(8,11));
    }
  }
  display.display();
}
/////////////////////////////////////////////////////
void oled_power() { 
  display.clearDisplay();
  if (Hide_OLED == false) {
    int p;
    display.setCursor(8,0);   // center time display
    display.setTextSize(1);   
    display.println(CurrentTime);
  
    // Power select to OLED dispaly
    display.setTextSize(2);   
  
    if (errInverter == true) {
      display.setCursor(0,12);  // ERROR
      display.println(F("ERROR"));
      display.setCursor(0,32); 
      display.println(F("INVER"));
    }else {
      if (inverter_mode == 0) {
        if (StatusCode != 7 && P_PV == 0) {
          display.setTextSize(1);   
          display.setCursor(0,16); 
          display.println(F("Status-ERR"));
          display.setTextSize(2);   
          String msg =  String(StatusCode) + "-" + String(ErrorCode);
          p = String(msg).length(); 
          p = (80-(p*16))/2;
          display.setCursor(p,32); 
          display.println(msg);
        }else {
          display.setCursor(0,12);  // SOLAR
          display.println(F("SOLAR"));
          p = String(P_PV).length(); 
          p = (80-(p*16))/2;
          display.setCursor(p,32); 
          display.println(P_PV);
        }
      }else if (inverter_mode == 1) {
        display.setCursor(8,12);  // LOAD
        display.println(F("LOAD"));
        p = String(P_Load).length(); 
        p = (80-(p*16))/2;
        display.setCursor(p,32); 
        display.println(P_Load);
      }else {
        //display.setCursor(8,12);  // GRID
        //display.println(F("GRID"));
        display.setCursor(0,12);  // G-AON
        display.println(msgAIRE);
  
        p = String(P_Grid).length(); 
        p = (80-(p*16))/2;
        display.setCursor(p,32); 
        display.println(P_Grid);
      }
    }
  }
  display.display();
}
/////////////////////////////////////////////////////

//------------------ MATRIX DISPLAY ---------------//
/////////////////////////////////////////////////////
void matrix_time() {
  int n;
  int p;

  // USA mode
  if (CurrentTime.substring(9,10)!= "") {   
    p=0;
    lc.setChar(0,1,' ',false);
    if (CurrentTime.substring(9,10)== "A") {
      lc.setChar(0,0,'A',false);
    }else {
      lc.setChar(0,0,'P',false);
    }
  // EU mode
  }else {                                   
    p=1;
    lc.setChar(0,7,' ',false);
    lc.setChar(0,0,' ',false);
    if (CurrentTime.startsWith("0")){
      lc.setChar(0,6,' ',false);
    }
  }

  n = (CurrentTime.substring(0,1)).toInt();
  if (n!=0) {
    lc.setDigit(0,7-p,n,false);
  }else {
    lc.setChar(0,7,' ',false);
  }
  
  n = (CurrentTime.substring(1,2)).toInt();
  lc.setDigit(0,6-p,n,true);
  n = (CurrentTime.substring(3,4)).toInt();
  lc.setDigit(0,5-p,n,false);
  n = (CurrentTime.substring(4,5)).toInt();
  lc.setDigit(0,4-p,n,true);
  n = (CurrentTime.substring(6,7)).toInt();
  lc.setDigit(0,3-p,n,false);
  n = (CurrentTime.substring(7,8)).toInt();
  lc.setDigit(0,2-p,n,false);
}
//////////////////////////////////////////////////////////////////////////////
void matrix_power() {
  int n;
  int p;
  String nTag;
  
  // Interval to Blink over Grid Power
  if (millis()-clkTimeO > 300){
    clkTimeO = millis();
    if (P_Grid <= -1000 && errInverter == false) {
      overGrid = !overGrid;
    }else {
      overGrid = false;
    }
  }

  // Power select to matrix dispaly
  if (inverter_mode == 0) {
    nTag = (String)P_PV;
    lc.setChar(0,7,'S',false);
  }else if (inverter_mode == 1) {
    nTag = (String)P_Load;
    lc.setChar(0,7,'L',false);
  }else {
    nTag = (String)P_Grid;
    lc.setChar(0,7,'G',false);
  }
  if (overGrid == true) {
    lc.setChar(0,7,' ',false);
  }
  lc.setChar(0,6,'=',false);
  lc.setChar(0,5,' ',false);

  // Fill Power
  p = nTag.length(); 
  for (int i = 4; i > p-2 ; i--) {
    lc.setChar(0,i,' ',false);
  }

  int j = p-1 ;
  for (int i = 0; i < p; i++) {
    if (nTag.substring(j,j+1) == "-") {
      lc.setChar(0,i,'-',false);
    }else {
      n = (nTag.substring(j,j+1)).toInt();
      j--;
      if (i == 3) {
        lc.setDigit(0,i,n,true);
      }else {
        lc.setDigit(0,i,n,false);
      }
    }
  }
}
//////////////////////////////////////////////////////////////////////////////
void matrix_AC() {
  String nTag;
  char char_array[6];
  oled_power();
  
  if (msgAIRE == "G-PUL") {
    nTag = "PULSA";  
  }else if (msgAIRE == "G-AON") {
    nTag = " A-ON";  
  }else if (msgAIRE == "G-AOF") {
    nTag = "A-OFF";  
  }else if (msgAIRE == "G-MON") {
    nTag = " M-ON";  
  }else if (msgAIRE == "STOP") {
    nTag = " " +  msgAIRE;  
  }else {
    nTag = msgAIRE;  
  }
  nTag.toCharArray(char_array, 6);
  
  // Show AC action on matrix dispaly
  lc.setChar(0,7,'A',false);
  lc.setChar(0,6,'C',false);
  lc.setChar(0,5,'=',false);
  for(int i = 0; i < 5; i++) {
    lc.setChar(0,4-i,char_array[i],false);
  }
}
//////////////////////////////////////////////////////////////////////////////
void scrollText() {
  _scroll = true;
  // Length (with one extra character for the null terminator)
  int mText_len = mText.length()+1; 
  
  // Prepare the character array (the buffer) 
  char char_array[mText_len];
  
  // Copy it over 
  mText.toCharArray(char_array, mText_len);
  
  for(int i = 0; i < mText_len-8; i++) {
    lc.setChar(0,7,char_array[i],false);
    lc.setChar(0,6,char_array[i+1],false);
    lc.setChar(0,5,char_array[i+2],false);
    lc.setChar(0,4,char_array[i+3],false);
    lc.setChar(0,3,char_array[i+4],false);
    lc.setChar(0,2,char_array[i+5],false);
    lc.setChar(0,1,char_array[i+6],false);
    lc.setChar(0,0,char_array[i+7],false);
    delay(matrix_speed*10);
    server.handleClient();
  }
  _scroll = false;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// A short method of adding the same web page header to some text
//////////////////////////////////////////////////////////////////////////////
void append_webpage_header() {
  // webpage is a global variable
  webpage = ""; // A blank string variable to hold the web page
  webpage += "<!DOCTYPE html><html>"; 
  webpage += "<style>html { font-family:tahoma; display:inline-block; margin:0px auto; text-align:center;}";
  webpage += "#mark      {border: 5px solid #316573 ; width: 1020px;} "; 
  webpage += "#header    {background-color:#C3E0E8; width:1000px; padding:10px; color:#13414E; font-size:32px;}";
  webpage += "#section   {background-color:#E6F5F9; width:980px; padding:10px; color:#0D7693 ; font-size:20px;}";
  webpage += "#footer    {background-color:#C3E0E8; width:980px; padding:10px; color:#13414E; font-size:24px; clear:both;}";
 
  webpage += ".button {box-shadow: 0px 10px 14px -7px #276873; background:linear-gradient(to bottom, #599bb3 5%, #408c99 100%);";
  webpage += "background-color:#599bb3; border-radius:8px; color:white; padding:13px 32px; display:inline-block; cursor:pointer;";
  webpage += "text-decoration:none;text-shadow:0px 1px 0px #3d768a; font-size:50px; font-weight:bold; margin:2px;}";
  webpage += ".button:hover {background:linear-gradient(to bottom, #408c99 5%, #599bb3 100%); background-color:#408c99;}";
  webpage += ".button:active {position:relative; top:1px;}";
 
  webpage += ".button2 {box-shadow: 0px 10px 14px -7px #8a2a21; background:linear-gradient(to bottom, #f24437 5%, #c62d1f 100%);";
  webpage += "background-color:#f24437; text-shadow:0px 1px 0px #810e05; }";
  webpage += ".button2:hover {background:linear-gradient(to bottom, #c62d1f 5%, #f24437 100%); background-color:#f24437;}";
  
  webpage += ".line {border: 3px solid #666; border-radius: 300px/10px; height:0px; width:80%;}";
  
  webpage += "input[type=\"text\"] {font-size:42px; width:90%; text-align:left;}";
  
  webpage += "input[type=range]{height:61px; -webkit-appearance:none;  margin:10px 0; width:65%;}";
  webpage += "input[type=range]:focus {outline:none;}";
  webpage += "input[type=range]::-webkit-slider-runnable-track {width:70%; height:30px; cursor:pointer; animate:0.2s; box-shadow: 2px 2px 5px #000000; background:#C3E0E8;border-radius:10px; border:1px solid #000000;}";
  webpage += "input[type=range]::-webkit-slider-thumb {box-shadow:3px 3px 6px #000000; border:2px solid #FFFFFF; height:50px; width:50px; border-radius:15px; background:#316573; cursor:pointer; -webkit-appearance:none; margin-top:-11.5px;}";
  webpage += "input[type=range]:focus::-webkit-slider-runnable-track {background: #C3E0E8;}";
  webpage += "</style>";
 
  webpage += "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/animate.css/4.1.1/animate.min.css\"/>";
  webpage += "<html><head><title>NTP Time & Inverter</title>";
  webpage += "</head>";
  webpage += "<script>";
  webpage += "function SendBright()";
  webpage += "{";
  webpage += "  strLine = \"\";";
  webpage += "  var request = new XMLHttpRequest();";
  webpage += "  strLine = \"BRIGHT=\" + document.getElementById(\"bright_form\").Bright.value;";
  webpage += "  request.open(\"GET\", strLine, false);";
  webpage += "  request.send(null);";
  webpage += "}";
  webpage += "</script>";
  webpage += "<div id=\"mark\">";
  webpage += "<div id=\"header\"><h1 class=\"animate__animated animate__flash\">NTP Time & Inverter " + HWversion + "</h1>";
}
//////////////////////////////////////////////////////////////////////////////
void button_Home() {
  webpage += "<p><a href=\"\\HOME\"><type=\"button\" class=\"button\">Refresh WEB</button></a></p>";
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void NTP_Clock_home_page() {
  append_webpage_header();
  webpage += "<h3 class=\"animate__animated animate__fadeInLeft\">RTC:";
  if (T_Zone2 == false) webpage += zone1; else webpage += zone2;
  webpage += " - ";
  if (display_EU == true) webpage += "EU"; else webpage += "USA";
  webpage += " mode  - B:";
  webpage += brightness;
  webpage += " - Chime:";
  if (sound_chime == false) webpage += "OFF"; else webpage += "ON";
 
  if (display_inverter == true) {
    webpage += "<p>#Inverter IP: ";
    webpage += IPinverter;
    if (StatusCode != 7 ) {
      webpage += " [S:" + String(StatusCode);
      webpage += "-C:" + String(ErrorCode);
    }else if (errInverter == true) {
      webpage += " [ERR";
    }else {
      webpage += " [OK";
    }
    webpage += "]#</p>";
   
    webpage += "<p>Total: ";
    webpage += String(E_Total); 
    webpage += "kWh | Day: ";
    webpage += String(E_Day);
    webpage += "kWh";
  
    webpage += "</p><p>Sol: ";
    webpage += String(P_PV); 
    webpage += "W | Load: ";
    webpage += String(P_Load);
    webpage += "W | Grid: ";
    webpage += String(P_Grid);
    webpage += "W";
    webpage += "</p>";
  };
  webpage += "</h3>";

  webpage += "<div id=\"section\">";
  button_Home();
  webpage += "<p><a href=\"\\DISPLAY_MODE_USA\"><type=\"button\" class=\"button\">USA mode</button></a>";
  webpage += "<a href=\"\\DISPLAY_MODE_EU\"><type=\"button\" class=\"button\">EU mode</button></a></p>";

  webpage += "<p><a href=\"\\DISPLAY_DATE\"><type=\"button\" class=\"button\">Show Date";
  if (display_date==true) webpage += " = ON"; 
  webpage += "</button></a>";
  webpage += "<a href=\"\\DISPLAY_NO_DATE\"><type=\"button\" class=\"button\">Only Time";
  if (display_date==false) webpage += " = ON"; 
  webpage += "</button></a></p>";

  webpage += "<p><a href=\"\\CARRILLON\"><type=\"button\" class=\"button\">Chime ";
  if (sound_chime==true) webpage += "OFF"; else webpage += "ON"; 
  webpage += "</button></a>";
  webpage += "<a href=\"\\sPAC\"><type=\"button\" class=\"button\">Pac-Man</button></a>";
  webpage += "<a href=\"\\SOUND\"><type=\"button\" class=\"button\">Sound TIME</button></a></p>";
 
  webpage += "<form id=\"bright_form\">";
  webpage += "<a>Brightness<br>MIN(0) <input type=\"range\" name=\"Bright\" min=\"0\" max=\"15\" value=\"";
  webpage += brightness;
  webpage += "\"> (15)MAX</a></form>";

  webpage += "<p><a href=\"\"><type=\"button\" onClick=\"SendBright()\" class=\"button\">Send Brightness</button></a>";
  if (Night_OLED == true ) {
    webpage += "<a href=\"\\NIGHT\"><type=\"button\" class=\"button\">OLED: Night</button></a></p>";
  }else {
    webpage += "<a href=\"\\DAY\"><type=\"button\" class=\"button\">OLED: Day</button></a></p>";
  }
  
  webpage += "<p><a href=\"\\DISPLAY_INVERTER\"><type=\"button\" class=\"button\">Inverter";
  if (display_inverter==true) webpage += " = ON"; 
  webpage += "</button></a>";
  webpage += "<a href=\"\\DISPLAY_NO_INVERTER\"><type=\"button\" class=\"button\">Only Time";
  if (display_inverter==false) webpage += " = ON"; 
  webpage += "</button></a></p>";
  
  webpage += "<p><a href=\"\\DISPLAY_INVERTER_MODE=0\"><type=\"button\" class=\"button\">Solar";
  if ((inverter_mode==0) && (display_inverter==true)) webpage += " #"; 
  webpage += "</button></a>";
  webpage += "<a href=\"\\DISPLAY_INVERTER_MODE=1\"><type=\"button\" class=\"button\">Load";
  if ((inverter_mode==1) && (display_inverter==true)) webpage += " #"; 
  webpage += "</button></a>";
  webpage += "<a href=\"\\DISPLAY_INVERTER_MODE=2\"><type=\"button\" class=\"button\">";
  if (display_inverter==false) webpage += "Grid"; else webpage += msgAIRE;
  if ((inverter_mode==2) && (display_inverter==true)) webpage += " #"; 
  webpage += "</button></a>";

  if (display_inverter==true) {
    webpage += "<a href=\"\\UMBRAL_AC\"><type=\"button\" class=\"button\">";
    if (AC1==true) webpage += "1"; else webpage += "2"; 
    webpage += "</button></a>";

    webpage += "<a href=\"\\AIR_C\"><type=\"button\" class=\"button\">AC:";
    if (Status_AIRE==true) webpage += "ON"; else webpage += "OFF"; 
    webpage += "</button></a>";
  }

  
  
  webpage += "</p><hr class=\"line\">";

  webpage += "<p><a href=\"\\RESTART_1\"><type=\"button\" class=\"button\">RTC: ";
  webpage += zone1;
  webpage += "</button></a>";
  webpage += "<a href=\"\\RESTART_2\"><type=\"button\" class=\"button\">RTC: ";
  webpage += zone2;
  webpage += "</button></a></p>";
  webpage += "<br><p><a href=\"\\RESET_WIFI\"><type=\"button\" class=\"button button2\">Reset WiFi</button></a></p>";
  webpage += "</div>";
  end_webpage();
}
//////////////////////////////////////////////////////////////////////////////
void reset_wifi() {
  append_webpage_header();
  webpage += "<p><h2>New WiFi Connection</h2></p></div>";
  webpage += "<div id=\"section\">";
  webpage += "<p>&#149; Connect WiFi to SSID: <b>ESP32_AP</b></p>";
  webpage += "<p>&#149; Next connect to: <b><a href=http://192.168.4.1/>http://192.168.4.1/</a></b></p>";
  webpage += "<p>&#149; Make the WiFi connection</p>";
  button_Home();
  webpage += "</div>";
  end_webpage();
  delay(1000);
  WiFiManager wifiManager;
  wifiManager.resetSettings();      // RESET WiFi in ESP32
  reset_ESP32();
}
//////////////////////////////////////////////////////////////
void web_reset_ESP32() {
  append_webpage_header();
  webpage += "<p><h2>Restarting ESP32...</h2></p></div>";
  webpage += "<div id=\"section\">";
  button_Home();
  webpage += "</div>";
  end_webpage();
  delay(1000);
  reset_ESP32();
}
//////////////////////////////////////////////////////////////
void end_webpage(){
  webpage += "<div id=\"footer\">Copyright &copy; J_RPM 2021</div></div></html>\r\n";
  server.send(200, "text/html", webpage);
  PRINTS("\n>>> end_webpage() OK! ");
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void display_mode_toggle() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(0, display_EU);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void display_date_toggle() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(1, display_date);
  end_Eprom();
}
//////////////////////////////////////////////////////////////////////////////
void brightness_matrix() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(2, brightness);
  //sendCmdAll(CMD_INTENSITY,brightness);
  lc.setIntensity(brightness);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void display_inverter_toggle() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(3, display_inverter);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void display_inverter_mode() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(4, inverter_mode);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void display_matrix_speed() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(5, matrix_speed);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void oled_night_toggle() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(7, Night_OLED);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void save_AC1() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(8, AC1);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void save_Chime() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(9, sound_chime);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void set_Zone2() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(13, T_Zone2);
  end_Eprom();
}
//////////////////////////////////////////////////////////////
void end_Eprom() {
  EEPROM.commit();
  EEPROM.end();
}
//////////////////////////////////////////////////////////////
void reset_ESP32() {
  //sendCmdAll(CMD_SHUTDOWN,0);
  lc.clearMatrix();
  ESP.restart();
  delay(5000);
}
//////////////////////////////////////////////////////////////
void display_AP_wifi() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(4,0);   
  display.println(F("ENTRY"));
  display.setCursor(10,16);   
  display.println(F("WiFi"));
  display.setTextSize(1);   
  display.setCursor(0,32);   
  display.println(F("ESP32_AP:1"));
  display.setCursor(0,40);   
  display.println(F("92.168.4.1"));
  display.display();
}
//////////////////////////////////////////////////////////////
void display_flash() {
  for (int i=0; i<8; i++) {
    lc.setIntensity(0);
    display.invertDisplay(true);
    display.display();
    delay(80);
    lc.setIntensity(15);
    display.invertDisplay(false);
    display.display();
    delay(80);
  }
  lc.setIntensity(brightness);
}
//////////////////////////////////////////////////////////////
void display_ip() {
  // Print the IP address MATRIX
  mText= spaces8 + "IP: " + WiFi.localIP().toString();
  scrollText();

  // Display OLED 
  display.clearDisplay();
  display.setTextSize(2);   
  display.setCursor(4,8);   
  display.print("ENTRY");
  display.setTextSize(1);   
  display.setCursor(0,26);   
  display.print("http://");
  display.print(WiFi.localIP());
  display.println("/");
  display.display();
}
//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
const char *err2Str(wl_status_t code){
  switch (code){
  case WL_IDLE_STATUS:    return("IDLE");           break; // WiFi is in process of changing between statuses
  case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; // case configured SSID cannot be reached
  case WL_CONNECTED:      return("CONNECTED");      break; // successful connection is established
  case WL_CONNECT_FAILED: return("PASSWORD_ERROR"); break; // password is incorrect
  case WL_DISCONNECTED:   return("CONNECT_FAILED"); break; // module is not configured in station mode
  default: return("??");
  }
}
////////////////////////////////////////////////
// Check clock config
////////////////////////////////////////////////
void _display_mode_usa() {
  display_EU = false;
  PRINTS("\n-> DISPLAY_MODE_USA");
  display_mode_toggle();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _display_mode_eu() {
  display_EU = true;
  PRINTS("\n-> DISPLAY_MODE_EU");
  display_mode_toggle();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _display_date() {
  display_date = true;
  PRINTS("\n-> DISPLAY_DATE");
  display_date_toggle();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _display_no_date() {
  display_date = false;
  PRINTS("\n-> DISPLAY_NO_DATE");
  display_date_toggle();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _display_inverter() {
  display_inverter = true;
  PRINTS("\n-> DISPLAY_INVERTER");
  display_inverter_toggle();
  
  PRINTS("\n-> DISPLAY_INVERTER MODE");
  display_inverter_mode();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _display_no_inverter() {
  display_inverter = false;
  PRINTS("\n-> DISPLAY_NO_INVERTER");
  display_inverter_toggle();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _inverter_0() {
  inverter_mode = 0;     
  PRINTS("\n-> INVERTER=0 (P_PV)");
  _display_inverter();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _inverter_1() {
  display_inverter = true;
  inverter_mode = 1;     
  PRINTS("\n-> INVERTER=1 (P_Load)");
  _display_inverter();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _inverter_2() {
  display_inverter = true;
  inverter_mode = 2;     
  PRINTS("\n-> INVERTER=2 (P_Grid)");
  _display_inverter();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _bright_0() {
  PRINTS("\n-> BRIGHT=0");
  brightness = 0;     //DUTY CYCLE: 1/32 (MIN) 
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_1() {
  PRINTS("\n-> BRIGHT=1");
  brightness = 1;    
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_2() {
  PRINTS("\n-> BRIGHT=2");
  brightness = 2;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_3() {
  PRINTS("\n-> BRIGHT=3");
  brightness = 3;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_4() {
  PRINTS("\n-> BRIGHT=4");
  brightness = 4;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_5() {
  PRINTS("\n-> BRIGHT=5");
  brightness = 5;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_6() {
  PRINTS("\n-> BRIGHT=6");
  brightness = 6;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_7() {
  PRINTS("\n-> BRIGHT=7");
  brightness = 7;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_8() {
  PRINTS("\n-> BRIGHT=8");
  brightness = 8;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_9() {
  PRINTS("\n-> BRIGHT=9");
  brightness = 9;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_10() {
  PRINTS("\n-> BRIGHT=10");
  brightness = 10;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_11() {
  PRINTS("\n-> BRIGHT=11");
  brightness = 11;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_12() {
  PRINTS("\n-> BRIGHT=12");
  brightness = 12;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_13() {
  PRINTS("\n-> BRIGHT=13");
  brightness = 13;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_14() {
  PRINTS("\n-> BRIGHT=14");
  brightness = 14;
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _bright_15() {
  PRINTS("\n-> BRIGHT=15");
  brightness = 15;     //DUTY CYCLE: 31/32 (MAX)
  _save_bright();
}
/////////////////////////////////////////////////////////////////
void _save_bright(){
  brightness_matrix();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _night(){
  Night_OLED = false;
  PRINTS("\n-> OLED: Day");
  oled_night_toggle();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _day(){
  Night_OLED = true;
  PRINTS("\n-> OLED: Night");
  oled_night_toggle();
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _restart_1() {
  T_Zone2=false;
  PRINT("\n>>> SYNC Time: ",zone1);
  set_Zone2();
  _restart();
}
/////////////////////////////////////////////////////////////////
void _restart_2() {
  T_Zone2=true;
  PRINT("\n>>> SYNC Time: ",zone2);
  set_Zone2();
  _restart();
}
/////////////////////////////////////////////////////////////////
void _restart() {
  PRINTS("\n-> RESTART");
  web_reset_ESP32();
}
/////////////////////////////////////////////////////////////////
void _reset_wifi() {
  PRINTS("\n-> RESET_WIFI");
  reset_wifi();
}
/////////////////////////////////////////////////////////////////
void _home() {
  PRINTS("\n-> HOME");
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _carrillon() {
  sound_chime = !sound_chime;
  PRINT("\n-> Carrillon = ", sound_chime);
  responseWeb();
  save_Chime();
}
/////////////////////////////////////////////////////////////////
void _umbral_ac() {
  AC1 = !AC1;
  load_AC();
 
  PRINT("\n-> Umbral AC1 = ", AC1);
  responseWeb();
  save_AC1();
}
/////////////////////////////////////////////////////////////////
void load_AC(){
  if (AC1==true) {
    AC_ON = -1500;
    AC_OFF = -100;
  }else{
    AC_ON = -1700;
    AC_OFF = -300;
  }
  PRINT("\n-> AC_ON = ", AC_ON);
  PRINT("\n-> AC_OFF = ", AC_OFF);
}
/////////////////////////////////////////////////////////////////
void _air_c() {
  Manual_AIRE = true;
  pulsaAIRE();
  PRINTS("\n-> AC = ON/OFF");
  responseWeb();
}
/////////////////////////////////////////////////////////////////
void _pacman() {
  if (pac==false) responseWeb();
  _scroll=true;
  soundEnd();
  delay(400);
}
/////////////////////////////////////////////////////////////////
void _sound() {
  responseWeb();
  soundTime();
  delay(400);
  soundEnd();
}
/////////////////////////////////////////////////////////////////
void responseWeb(){
  PRINTS("\nS_RESPONSE");
  NTP_Clock_home_page();
  PRINTS("\n-> SendPage");
}
/////////////////////////////////////////////////////////////////
void checkServer(){
  server.begin();  // Start the WebServer
  PRINTS("\nWebServer started");
  // Define what happens when a client requests attention
  server.on("/", _home);
  server.on("/DISPLAY_MODE_USA", _display_mode_usa); 
  server.on("/DISPLAY_MODE_EU", _display_mode_eu); 
  server.on("/DISPLAY_DATE", _display_date);
  server.on("/DISPLAY_NO_DATE", _display_no_date);
  
  server.on("/DISPLAY_INVERTER", _display_inverter);
  server.on("/DISPLAY_NO_INVERTER", _display_no_inverter);
  
  server.on("/DISPLAY_INVERTER_MODE=0", _inverter_0); 
  server.on("/DISPLAY_INVERTER_MODE=1", _inverter_1); 
  server.on("/DISPLAY_INVERTER_MODE=2", _inverter_2); 

  server.on("/BRIGHT=0", _bright_0); 
  server.on("/BRIGHT=1", _bright_1); 
  server.on("/BRIGHT=2", _bright_2); 
  server.on("/BRIGHT=3", _bright_3); 
  server.on("/BRIGHT=4", _bright_4); 
  server.on("/BRIGHT=5", _bright_5); 
  server.on("/BRIGHT=5", _bright_6); 
  server.on("/BRIGHT=7", _bright_6); 
  server.on("/BRIGHT=8", _bright_8); 
  server.on("/BRIGHT=9", _bright_9); 
  server.on("/BRIGHT=10", _bright_10); 
  server.on("/BRIGHT=11", _bright_11); 
  server.on("/BRIGHT=12", _bright_12); 
  server.on("/BRIGHT=13", _bright_13); 
  server.on("/BRIGHT=14", _bright_14); 
  server.on("/BRIGHT=15", _bright_15); 
  server.on("/NIGHT", _night);
  server.on("/DAY", _day);
  server.on("/HOME", _home);
  server.on("/CARRILLON", _carrillon);
  server.on("/UMBRAL_AC", _umbral_ac);
  server.on("/AIR_C", _air_c);
  server.on("/sPAC", _pacman);
  server.on("/SOUND", _sound);
  server.on("/RESTART_1", _restart_1);  
  server.on("/RESTART_2", _restart_2);  
  server.on("/RESET_WIFI", _reset_wifi);
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
void testBCD() {
  for(int n=0;n<8;n++) {
    for(int i=0;i<8;i++) {
      lc.setDigit(0,i,8,true);
    }
    delay(80);
    lc.clearMatrix();
    delay(80);
  }
}
/////////////////////////////////////////////////////////////////
void checkAlarm(){
  // Hourly announcements from 'alarm_H' to 11 p.m. 
  if (myH >= alarm_H && m == 0) {
    soundAlarm(myH);
  }
}
/////////////////////////////////////////////////////////////////
void check_Hide_OLED(){
  if ((myH >= 7 && myH != 23) || Night_OLED == false) {
    Hide_OLED = false;
  }else {
    Hide_OLED = true;
  }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
void soundAlarm(int n){
  _scroll=true;
  PRINT("\nAlarm: ", String(n) + ":00" );
  lc.setIntensity(15);
  GameAudio.PlayWav(&wawTone, false, 1.0); 
  while(GameAudio.IsPlaying()){ } // wait until done
  lc.setIntensity(0);
  delay(400);
  lc.setIntensity(15);
  soundTime();
  lc.setIntensity(0);
  delay(600);
  soundEnd();
}/////////////////////////////////////////////////////////////////
void soundEnd(){
  _scroll=true;
  if (display_inverter == false){
    Oled_Time();
  }
  pacmanEffect();
  GameAudio.PlayWav(&pmDeath, false, 1.0);
  testBCD();
  UpdateLocalTime();
  lc.setIntensity(brightness);
  _scroll=false;
}
/////////////////////////////////////////////////////////////////
void pacmanEffect(){
  bool p=false;
  lc.setIntensity(15);
  for (int i=8; i>=0; i--) {
   GameAudio.PlayWav(&pmEat, false, 1.0);
   lc.setChar(0,i+1,' ',false);
   if (p==true){
      p=false;
      lc.setChar(0,i,'G',false);
    }else {
      p=true;
      lc.setChar(0,i,'C',false);
    }
    while(GameAudio.IsPlaying()){ }
  }
}
/////////////////////////////////////////////////////////////////
void soundTime(){
  _scroll=true;
  lc.setIntensity(15);
  UpdateLocalTime();
  Oled_Time();
  matrix_time();
  PRINT("\n>>> SOUND Time: ", CurrentTime);
  PRINTS("\n");
  GameAudio.PlayWav(&wawSon, false, 1.0);
  while(GameAudio.IsPlaying()){ }    // wait until done
  playWawT(CurrentTime.substring(0,2).toInt());
  GameAudio.PlayWav(&wawHor, false, 1.0);
  while(GameAudio.IsPlaying()){ }    // wait until done
  delay(200);
  playWawT(CurrentTime.substring(3,5).toInt());
  GameAudio.PlayWav(&wawMin, false, 1.0);
  while(GameAudio.IsPlaying()){ }    // wait until done
  _scroll=false;
}
/////////////////////////////////////////////////////////////////
void playWawT(int w){
  int wH = w/10;
  if (wH > 0) playWawN(wH);
  playWawN(w%10);
}
/////////////////////////////////////////////////////////////////
void playWawN(int n){
  switch(n) {
    case 0: GameAudio.PlayWav(&waw0, false, 1.0); break;  
    case 1: GameAudio.PlayWav(&waw1, false, 1.0); break;
    case 2: GameAudio.PlayWav(&waw2, false, 1.0); break;  
    case 3: GameAudio.PlayWav(&waw3, false, 1.0); break;  
    case 4: GameAudio.PlayWav(&waw4, false, 1.0); break;  
    case 5: GameAudio.PlayWav(&waw5, false, 1.0); break;  
    case 6: GameAudio.PlayWav(&waw6, false, 1.0); break;  
    case 7: GameAudio.PlayWav(&waw7, false, 1.0); break;  
    case 8: GameAudio.PlayWav(&waw8, false, 1.0); break;  
    case 9: GameAudio.PlayWav(&waw9, false, 1.0); break;
  }
  while(GameAudio.IsPlaying()){ }    // wait until done
}
/////////////////////////////////////////////////////////////////
void playIP(){
  String dg;
  String mIP = WiFi.localIP().toString();
  unsigned int carIP = mIP.length();
  for (int i=0; i<=(carIP-1); i++) {
    dg = mIP.substring(i,i+1);
    if (dg==".") {
      GameAudio.PlayWav(&wawPunto, false, 1.0);
    }else {
      playWawN(dg.toInt());
    }
    while(GameAudio.IsPlaying()){ }    // wait until done
  }
}
/////////////////////////////////////////////////////////////////
void testAire(){
  setAIRE();
  if(millis()-clkTimeA > 60000){
    clkTimeA = millis();
    Cont_Min = Cont_Min - 1;

    // Test AUTO AC ON/OFF
    if  ((msgAIRE == "G-AOF" || msgAIRE == "G-AON") && Cont_Min < 1){
      if((P_Grid < AC_ON && Status_AIRE == false) || (P_Grid > AC_OFF && Status_AIRE == true)){
        Manual_AIRE = false;
        pulsaAIRE(); // >>> PULSA AC (AUTO) 
      }
    }
   }
  }
 /////////////////////////////////////////////////////////////////
void pulsaAIRE() {
  digitalWrite(OUT_PULSA, HIGH);  // PULSA = ON
  msgAIRE="G-PUL";
  matrix_AC();
  delay (3000);
  digitalWrite(OUT_PULSA, LOW);  // PULSA = OFF

  // Show action AC: Start/Stop
  if(Status_AIRE == false){
    msgAIRE="START";
    matrix_AC();
    delay (10000);
  }else {
    msgAIRE="STOP";
    matrix_AC();
    delay (2000);
  }
  
  // Show status AC
  setAIRE();
  matrix_AC();
  delay (1000);
}
 /////////////////////////////////////////////////////////////////
void setAIRE() {
  if(digitalRead(IN_AIRE) == HIGH){
      Status_AIRE = true;
  }else{
      Status_AIRE = false;
  }
  
  // Reload, minutes between two automatic AC actions
  if (Status_BACK != Status_AIRE) {
    Status_BACK = Status_AIRE;
    clkTimeA = millis();
    Cont_Min = Min_AC;             
  }
 
  if (Manual_AIRE == true){
    if(Status_AIRE == true){  // MANUAL & ON
      msgAIRE="G-MON";
    }else { // MANUAL & OFF >>> AUTO
      Manual_AIRE = true;
      msgAIRE="G-AOF";
    }
  }else{
    if(Status_AIRE == true){  // AUTO & ON
      msgAIRE="G-AON";
    }else { // AUTO & OFF 
      Manual_AIRE = true;
      msgAIRE="G-AOF";
    }
  }
  oled_power();
}
/////////////////////////////////////////////////////////////////
void getInverter() {
  queryInverter(1);
  if (errInverter == true) return;
  // When the inverter is stopped, the status and alarm code are checked
  if (StatusCode != 7 || P_PV == 0) { 
    queryInverter(2);
  }
}
/////////////////////////////////////////////////////////////////
void queryInverter(int query){
  int reciveCode;
  HTTPClient http;
  
  if (query == 1) {
    http.begin(sendInverter1);
    reciveCode = http.POST(sendInverter1);
  }else {
    http.begin(sendInverter2);
    reciveCode = http.POST(sendInverter2);
  }
  
  if(reciveCode > 0 ){
    if(reciveCode == 200){
      errInverter = false;
      String strInverter = http.getString();
      if (query == 1) {
        filerJson1(strInverter);
      }else {
        filerJson2(strInverter);
      }
    }
  }else {
    errInverter = true;
    Serial.print("Polling error, code: ");
    Serial.println(reciveCode);
  }
  http.end();
 }
/////////////////////////////////////////////////////////////////
 void filerJson1(String input){
  StaticJsonDocument<0> filter;
  filter.set(true);
  StaticJsonDocument<768> doc;

  DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  JsonObject Body_Data = doc["Body"]["Data"];
  JsonObject Body_Data_Site = Body_Data["Site"];
  
  E_Total = Body_Data_Site["E_Total"]; 
  E_Total = E_Total / 1000;
  E_Day = Body_Data_Site["E_Day"]; 
  E_Day = E_Day / 1000;
  P_PV = Body_Data_Site["P_PV"];
  P_Load = Body_Data_Site["P_Load"]; 
  P_Load = P_Load * (-1);
  P_Grid = Body_Data_Site["P_Grid"]; 
  
  Serial.println (F("--------------------"));
  Serial.println (CurrentTime);
  Serial.println (F("--------------------"));
  
  Serial.print (F("E_Total: "));
  Serial.print (E_Total);
  Serial.println (F(" kWh"));
  
  Serial.print ("E_Day: ");
  Serial.print (E_Day);
  Serial.println (F(" kWh"));

  Serial.print (F("P_PV: "));
  Serial.print (P_PV);
  Serial.println (F(" W"));
 
  Serial.print (F("P_Load: "));
  Serial.print (P_Load);
  Serial.println (F(" W"));
  
  Serial.print (F("P_Grid: "));
  Serial.print (P_Grid);
  Serial.println (F(" W"));
    
}
/////////////////////////////////////////////////////////////////
 void filerJson2(String input){
  StaticJsonDocument<0> filter;
  filter.set(true);
  StaticJsonDocument<768> doc;
  
  DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  JsonObject Body_Data_1 = doc["Body"]["Data"]["1"];
  ErrorCode = Body_Data_1["ErrorCode"];
  StatusCode = Body_Data_1["StatusCode"]; 
  
  Serial.print (F("StatusCode: "));
  Serial.println (StatusCode);
  Serial.print (F("ErrorCode: "));
  Serial.println (ErrorCode);
}
/////////////////////////////////////////////////////////////////
void convert_EU_Time() {
  //Convert to EU time for hide OLED
  String modT = CurrentTime.substring(9,10);
  if (modT == "P" && h!=12) {
    myH=h+12;
  }else if (modT == "A" && h==12) {
    myH=0;
  }else {
    myH=h;
  }
  //PRINT("\nTime: ", CurrentTime + " -> EU:" + myH + " s:" + String(s));

  check_Hide_OLED();
}
/////////////////////////////////////////////////////////////////
void display_scroll_msg() {
  _scroll = true;
  UpdateLocalTime();
  if (display_inverter == false){
    Oled_Time();
  }else {
    oled_power();
  }
  scrollText();
  UpdateLocalTime();
  clkTime = millis();
  _scroll = false;
}
////////////////////////// END //////////////////////////////////
/////////////////////////////////////////////////////////////////
