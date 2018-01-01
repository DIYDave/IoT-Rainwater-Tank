/*--------------------------------------------------------------
IoT Rain-Watertank
ESP8266 NodeMCU1.0(ESP12E)
Act as HTML-5 Webserver and manage multiple client tasks
Sends push notification over Prowl to iOS devices
Sends data to thingspeak.com
Mesures waterlevel with ultrasonic
Controls two pumps start / stop over HTML or radio remote control
copyright: d.waldesbuehl@hispeed.ch
On Youtube: https://www.youtube.com/watch?v=UjX3-XNddWY (Geman only)
--------------------------------------------------------------*/
#include <ESP8266WiFi.h>          // ESP library for all WiFi functions
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          // Manage auto connect to WiFi an fall back to AP   //https://github.com/tzapu/WiFiManager
#include <ArduinoOTA.h>           // Library for uploading sketch over the air (OTA)
#include <SPI.h>                  // Library for hardware or software driven SPI
#include <EEPROM.h>               // Library for handle teh EEPROM (for ESP: use "EEPROM.commit();" after  "EEPROM.write(i, i);" )
#include <ArduinoJson.h>          // Library for parsing Json Strings
#include <U8g2lib.h>              // Library to control the 128x64 Pixel OLED display with SH1106 chip  https://github.com/olikraus/u8g2
#include "FS.h"
#include <RBD_Timer.h>            // Debounce https://github.com/alextaujenis/RBD_Timer
#include <RBD_Button.h>           // Debaunce https://github.com/alextaujenis/RBD_Button


#define REQ_BUF_SZ   80         // limit the size of the received HTTP request

#define trigPin D0      // Trigger Pin  Ultrasonic
#define echoPin D1      // Echo Pin Ultrasonic
#define Abpumpen D3     // Relais Abpumpen
#define Giessen D4      // Relais Giessen

RBD::Button RFin(D2);

// OpenWeatherMap settings
const char weatherServer[] = "api.openweathermap.org";    // Address for openweathermap 
String nameOfCity = "YOUR CITY NAME HERE";                // City to receive weather forecast from
String apiID = "YOUR PERSONAL API KEY HERE";              // Personal API from openweathermap.org

// ThingSpeak settings
const int channelID = YOUR CHANNEL ID HERE;
String writeAPIKey = "YOUR PERSONAL API KEY HERE";          // Personal write API key for your ThingSpeak Channel
const char* TSserver = "api.thingspeak.com";

// Prowl iOS Push settings
const char ProwlServer[] = "api.prowlapp.com";
String ProwlapiID = "YOUR PERSONAL API KEY HERE";           // Personal API key for you Prowl
String Application = "IoT%20Regenwassertank";
String Event = "";         // Set in application "Wasserstand%20hoch";
String Message = "";       // Set in Application "Viel%20Regen%20erwartet";


//Intervall settings
unsigned long lastConnectionTime;                 // letzte Serverabfrage
unsigned long postingInterval = 5500;             // Intervall für Wetter-Server abfrage (nur erste nach Kaltstart)
unsigned long lastMeasureTime;                    // letzte Pegelmessung   
unsigned long measureInterval = 4 * 1000;         // Intervall der Pegelmessung
unsigned long lastScreenchange;                   // letzte Pegelmessung   
unsigned long ScreenchangeInterval = 7 * 1000;    // Intervall der Screenchange
unsigned long lastThingspeakTime;                 // last Thingspeak message
unsigned long ThingspeakInterval = 60 * 1000;     // Intervall for Thingspeak message
unsigned long lastProwlTime;                      // last Thingspeak message
unsigned long ProwlInterval = 3600 * 1000;        // Intervall for Prowl message 1 hour
unsigned long Timer1 = 0;                         // Timer for "pump on" over radio control. 

char HTTP_req[REQ_BUF_SZ] = {0};                  // Buffered HTTP request from client (browser) stored as null terminated string
char req_index = 0;                               // Index into HTTP_req buffer
String JSONtext;                                  // String to hold answere from openweathermap
int endResponse         = 0;                      // Verwendung für parsen
boolean startJson       = false;                  // Verwendung für parsen

int16_t Sollwert;                                 // Sollwert Bezug: 0 = Unterkannte Überlauf (negative Zahlen)
int Istwert         = -19;                        // Tanktiefe ab unterkannte Überlauf = 900mm = 270px für Sollwert Faktor 3,333  (negative Zahlen)
int Offset          = -115;                       // Offset from US-sensor to undersite of qverflow tube
int Hysterese       = 20;                         // Hyteresis for flush pump
int LitterWennVoll  = 930;                        // Wasser volumen bei Überlauf 
int AktLiter;                                     // Actual water quantity in tank
float rain[4], wind[4], temp[4], rainNext3h;      // Forecast data of weather
float totalRain ;                                 // Sum of rain in the forecast
bool entleerPumpeAuto;
long FiltVal;                                     // Gemittleter Messwert von Ultraschall
byte FilterZaehler = 0, FilterAnzahl = 5;         // Werte für mittelwertbildung des Istwertes (Gleitende Mittelwertbildung / Tiefpass)
bool weatherOK      = 0;                          // 1 if answere from weather server OK
boolean MainScreen   = 0;
boolean WebLow;                                   // Merker für Low vom Web. Zusammen mit "GiesspumpeWeb" verwendet
boolean GiessPumpWeb;                             // Wird ein AJAX-Zyklus lang 1 wenn Checkbox "Giessen" angeklickt wird
bool rfLow, merkerGiessen;                        // Funk Merker für steigende Flanke   // Hilfsmerker für Flipflop
bool clientInUse;                                 // verhindert gleichzeitige Anfragen an Server
bool ProwlOK = 0;


//********** build Instances
WiFiServer server(80);
WiFiClient client2;         // Client für Verbindung mit Wetterserver
// SSoftware SPI for OLED display !CS-pin unused (connected direct to ground)
U8G2_SH1106_128X64_VCOMH0_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ D5, /* data=*/ D6, /* cs=*/ D9, /* dc=*/ D8, /* reset=*/ D7); 

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup(){   // ******************* Begin of Setup *******************
  Serial.begin(115200);

  // Setup an start file system.
  if (!SPIFFS.begin()){
    Serial.println("SPIFFS nicht initialisiert!");   
    while(1) {                                       // ohne SPIFFS geht es sowieso nicht...
      yield();
    }
  }
  Serial.println("SPIFFS ok");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(Abpumpen, OUTPUT);   digitalWrite(Abpumpen, HIGH);
  pinMode(Giessen, OUTPUT);    digitalWrite(Giessen, HIGH);
  RFin.invertReading();                   //Flip the value of the raw button state reading so when the button isPressed() it will register as isReleased(), 
  RFin.setDebounceTimeout(200);          // Input with Pullup per default (debouncetime [ms])

  // Setup and start OLED
  u8g2.begin();
  u8g2.setContrast(110);
  u8g2.clearBuffer();          // clear the internal memory
  OLED_Screen1();              // Wellcome screen

  // Setup an start WiFi
  WiFiManager wifiManager;
  IPAddress _ip = IPAddress(192, 168, 0, 9);
  IPAddress _gw = IPAddress(192, 168, 0, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);
  
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  //wifiManager.setAPCallback(configModeCallback);        // Call routine to show infos when in Config Mode
  //wifiManager.resetSettings(); // For test only

  if (!wifiManager.autoConnect("Pegelstand")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(2000);
    ESP.reset();
    delay(3000);
  }
  
  //if you get here you have connected to the WiFi
  ArduinoOTA.begin();
  server.begin();
  
  OLED_Screen2();             // WiFi connect screen
  delay(1000);
  
  EEPROM.begin(50);
  byte high = EEPROM.read(0);   //read the first half of setpoint
  byte low = EEPROM.read(1);    //read the second half of setpoint
  Sollwert = (high << 8) + low;
         
} //----------- SETUP END -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop(){   // ******************* Begin of Loop *********************************
  ArduinoOTA.handle();

  WiFiClient client = server.available();               // Client (Browser) verbunden?
  if (client && !clientInUse) {  // got client?
    getRequest(client);
  }

   if (millis() - lastMeasureTime > measureInterval) {
    ultrasonic();                                       // Call ultrasonic function           
    calculation();
  }

  if ((millis() - lastThingspeakTime > ThingspeakInterval && !clientInUse)) { 
    thinspeakPost();                                      // Send actual values to thingspeak.com
  }

  if ((millis() - lastConnectionTime > postingInterval && !clientInUse)) { 
    weatherRequest();                                      // Request to openweathermap.com
  }
  
  if (client2.available()) {                            // connectetd with Server     
    readJson (client2.read());                          // Call function and give received char to it
  }
  
  if (millis() - lastScreenchange > ScreenchangeInterval) {  
    Screenchange();                                     //change the OLED Screen info
    if (weatherOK){ postingInterval = 605 * 1000;}      // if weather forecast successfull received -> set interval to 10 min.
  }

  if ((millis() - lastProwlTime > ProwlInterval) && weatherOK && !clientInUse) {
    if (rainNext3h * 20 > (Istwert*-1)) {                // Rain in mm/m2 * colecting area (20m2)
      Event = "Der%20aktuelle%20Pegel%20beträgt:%20"; Event += Istwert; Event += "%20mm";
      Message = "Es%20wird%20in%20den%20naechsten%203%20Stunden%20"; Message += rainNext3h; Message += "%20mm/m2%20Regen%20erwartet!";
      ProwlPush();                                        // Send request to Prowl
    } 
  }                                     

  if (ProwlOK == 0 && weatherOK && !clientInUse){    // test only after first start
    Event = "Der%20aktuelle%20Pegel%20beträgt:%20"; Event += Istwert; Event += "%20mm";
    Message = "Es%20wird%20in%20den%20naechsten%203%20Stunden%20"; Message += rainNext3h; Message += "%20mm/m2%20Regen%20erwartet!";
    ProwlPush();   
  }
  
  entleeren();                                          //switch pump on / off
  giessen();                                            //switch pump on / off

  if (merkerGiessen){
    digitalWrite(Giessen, LOW);                 // Pump On
  } else{
    digitalWrite(Giessen, HIGH);                // Pump off
  }

} //----------- LOOP END ---------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////
//    ESP act as Client                                                                               //
//    Client (ESP) sendet Anfrage für JSON an Wetter Server und wertet die Antwort aus                //
//    Zudem werden alle 60 sek. aktuelle Messwerte an Thinkspeak.com gesendet
////////////////////////////////////////////////////////////////////////////////////////////////////////
void thinspeakPost(){                               // Send data to thingspeak.com
  clientInUse = 1;
  client2.stop();                                       // close any connection before send a new request.  This will free the socket on the WiFi
    if (client2.connect(TSserver, 80)) {
    // Construct API request body
  String body = "field1=";
         body += String(Sollwert);
         body += "&field2=";
         body += String(Istwert);  
         body += "&field3=";
         body += String(AktLiter);             
         body += "&field4=";
         body += String(totalRain);  
         body += "&field5=";
         body += String(digitalRead(Abpumpen));  
         body += "&field6=";
         body += String(digitalRead(Giessen));                   
    client2.print("POST /update HTTP/1.1\n");
    client2.print("Host: api.thingspeak.com\n");
    client2.print("Connection: close\n");
    client2.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client2.print("Content-Type: application/x-www-form-urlencoded\n");
    client2.print("Content-Length: ");
    client2.print(body.length());
    client2.print("\n\n");
    client2.print(body);
    client2.print("\n\n");
  }
  lastThingspeakTime = millis();
  clientInUse = 0;
}

void ProwlPush(){ 
  clientInUse = 1;
  client2.stop();                                       // close any connection before send a new request.  This will free the socket on the WiFi
  ProwlOK = 1;
  if (client2.connect(ProwlServer, 80)) {             // if there's a successful connection:
    client2.println("POST /publicapi/add?&apikey=" + ProwlapiID + "&application=" + Application + "&event=" + Event + "&description=" + Message + " HTTP/1.1");
    client2.println("Host: api.prowlapp.com");
    client2.println("User-Agent: ArduinoWiFi/1.1");
    client2.println("Connection: close");
    client2.println();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection with Prowl failed");
  }
  lastProwlTime = millis();
  clientInUse = 0;
}

void weatherRequest() {                                    // makes a HTTP request to OpenWeather.com
  clientInUse = 1;
  client2.stop();                                       // close any connection before send a new request.  This will free the socket on the WiFi
  weatherOK = 0;
  if (client2.connect(weatherServer, 80)) {             // if there's a successful connection:
    client2.println("GET /data/2.5/forecast?q=" + nameOfCity + "&APPID=" + apiID + "&mode=json&units=metric&cnt=4 HTTP/1.1");
    client2.println("Host: api.openweathermap.org");
    client2.println("User-Agent: ArduinoWiFi/1.1");
    client2.println("Connection: close");
    client2.println();
    //Serial.println("Server Connected");
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
  lastConnectionTime = millis();
  clientInUse = 0;
}


void readJson (char c){    
  if (endResponse == 0 && startJson == true) {          // If equal { and } the json is complettly read
    //Serial.println(JSONtext);
    JsonParse(JSONtext.c_str());                            // parse string "text" in parseJson function
    JSONtext = "";                                          // clear text string for the next time
    startJson = false;                                  // set startJson to false to indicate that a new message has not yet started
  }
  if (c == '{') {
    startJson = true;                                   // set startJson true to indicate json message has started
    endResponse++;                                      // For each { increment
  }
  if (c == '}') {
    endResponse--;                                      // For each } decrement
  }
  if (startJson == true) {
    JSONtext += c;
  }
}


void JsonParse(const char * jsonString){                // Parsing the JSON String from Server
  DynamicJsonBuffer  jsonBuffer;                        // StaticJsonBuffer<3000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonString);  
  
  if (!root.success()) {                                // Test if parsing succeeds.
    Serial.println("parseObject() failed");
    return;
  } 
  totalRain = 0;
  for (int i=0; i <= 3; i++){
    String weather = root["list"][i]["weather"][0]["main"];
    if (weather = "Rain") {
      rain[i] = root["list"][i]["rain"]["3h"];
      rainNext3h = rain[0];
      totalRain += rain[i];
      //Serial.println(rain[i]);        
    }   
  }
  //Serial.println();
  for (int i=0; i <= 3; i++){
      wind[i] = root["list"][i]["wind"]["speed"];
      //Serial.println(wind[i]);        
  }
  //Serial.println();
    for (int i=0; i <= 3; i++){
      temp[i] = root["list"][i]["main"]["temp"];
      //Serial.println(temp[i]);        
  }
  weatherOK = 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//    ESP act as Server                                                                               //    
//    Server liest die "GET" Anfrage des Client (Browser)und wertet sie aus                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////
void getRequest(WiFiClient cl){                           
  cl.setNoDelay(1);
  boolean currentLineIsBlank = true;
  while (cl.connected()) {
    if (cl.available()) {                               // client data available to read
      char c = cl.read();                               // read 1 byte (character) from client                
      // buffer first part of HTTP request 
      if (req_index < (REQ_BUF_SZ - 1)) {               // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
        HTTP_req[req_index] = c;                        // save HTTP request character
        req_index++;
      }
      // last line of client request is blank and ends with \n
      // respond to client only after last line received
      if (c == '\n' && currentLineIsBlank) { 
        //Serial.println(HTTP_req);            
        cl.println("HTTP/1.1 200 OK");                  // send a standard http response header

        if (StrContains(HTTP_req, "/xml")) {            // Ajax request - send XML file
          cl.println("Content-Type: text/xml");         // send rest of HTTP header
          cl.println("Connection: keep-alive");
          cl.println();
          ProcessingParameters();
          // send XML file containing input states
          XML_response(cl);
        }
        else {                                          // web page request
          // send rest of HTTP header                      
          cl.println("Content-Type: text/html");
          cl.println("Connection: keep-alive");
          cl.println();
          // send web page
            String sHTML ;
            String filename = "/index.html"; //server->uri(); 
            File webFile = SPIFFS.open(filename, "r");  // load website from SPIFFS                       
          if (webFile) {
            const int bufSize = 1760;                   // Maximum für "client.write"
            byte clientBuf[bufSize];
            int clientCount = 0;
   
            while (webFile.available()){
              clientBuf[clientCount] = webFile.read();
              clientCount++;  
              if (clientCount > bufSize-1)
              {          
                cl.write((const uint8_t *)clientBuf, bufSize);
                clientCount = 0;
              }
            }
            // final < bufSize byte cleanup packet
            if (clientCount > 0) cl.write((const uint8_t *)clientBuf, clientCount);
    
            webFile.close();                            // close the file:
          } else {
            Serial.println("file not found");
          }
        }           
          req_index = 0;                                // reset buffer index and all buffer elements to 0
          StrClear(HTTP_req, REQ_BUF_SZ);
          break;
      }
      // every line of text received from the client ends with \r\n
      if (c == '\n') {
          // last character on line of received text
          // starting new line with next character read
          currentLineIsBlank = true;
      } 
      else if (c != '\r') {
          // a text character was received from client
          currentLineIsBlank = false;
      }
    } // end if (client.available())
  } // end while (client.connected())
  delay(1);      // give the web browser time to receive the data
  cl.stop(); // close the connection
} // end function


// Read received GET request an act
void ProcessingParameters(void){
  if (StrContains(HTTP_req, "Sollwert=")) {
    String str(HTTP_req);
    //Gibt den Wert zw. Zeichen 18 und Fundort von &nocache als Int zurück
    int SollwertNew = str.substring(18,str.indexOf("&nocache")).toInt();
    if (SollwertNew != Sollwert) {
      if (SollwertNew <= 0 && SollwertNew >= -250) {
        Sollwert = SollwertNew;
        EEPROM.write(0, highByte(Sollwert)); 
        EEPROM.write(1, lowByte(Sollwert)); 
        EEPROM.commit();
        //Serial.println(Sollwert);
      }  
    }
  }
  if (StrContains(HTTP_req, "Giessen=1")) {
      GiessPumpWeb = 1;     
  }
  else if (StrContains(HTTP_req, "Giessen=0")) {
      GiessPumpWeb = 0; 
  }
 }

// send the XML file to Browser (AJAX request)
void XML_response(WiFiClient cl){
    String XML ="";
    XML="<?xml version=\"1.0\"?>";
    XML+="<response>";
    XML+=  "<inhalt>";
    XML+=     "Inhalt: "; XML+=AktLiter;  XML+=" Liter";
    XML+=  "</inhalt>";   
    XML+=  "<istpegel>";
    XML+= Istwert;
    XML+=  "</istpegel>"; 
    XML+=  "<sollpegel>";
    XML+= Sollwert; 
    XML+=  "</sollpegel>"; 
    XML+=  "<wasserstand>";
    XML+=   AktLiter/3.43;            // Anpassung an Höhe des Wassers in Pixel
    XML+=  "</wasserstand>";
    XML+=  "<regenmenge>";
    char result[6];
    dtostrf(totalRain, 4, 1, result);
    XML+=result;
    XML+=  "</regenmenge>";
    XML+=  "<giessen>"; 
    XML+=digitalRead(Giessen);
    XML+=  "</giessen>";  
    XML+=  "<pumpen>"; 
    XML+=digitalRead(Abpumpen);
    XML+=  "</pumpen>";        
    XML+="</response>";
    cl.print(XML);
    //Serial.println(XML);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//    Other subroutines                                                                               //    
//                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////

void ultrasonic(){
  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration/5.82;
  FiltVal = ((FiltVal * FilterZaehler) + distance) / (FilterZaehler + 1); // Rollender Mittlwert
  if (FilterZaehler < FilterAnzahl){FilterZaehler ++;}                    // Damit bei Kaltstart schon gültige Werte entstehen
  // Serial.println(FiltVal);
  lastMeasureTime = millis();
}


void calculation(){
  Istwert = (FiltVal *-1) - Offset;
  AktLiter = LitterWennVoll + Istwert;
  if (Istwert >= Sollwert + Hysterese){    // Pumpe Ein
    entleerPumpeAuto = true;
  }
  if (Istwert <= Sollwert){               // Pumpe Aus
    entleerPumpeAuto = false;
  }
}


void entleeren(){
  if (entleerPumpeAuto == true){
    digitalWrite(Abpumpen, LOW);        // Pumpe Ein
  }     
   else{
    digitalWrite(Abpumpen, HIGH);   
   }      
}


void giessen(){                 // With debouncing of the RF Input
  // Control from radio control
  if (RFin.onPressed()){
    Timer1 = millis();                            //For switch on
    if (merkerGiessen){
      merkerGiessen = 0; 
    }
  }
  
  if (RFin.isPressed()){
    if (millis() - Timer1 > 1000){                // Switch on, if key on radio control is pressed for min. 1Sec. 
      merkerGiessen = 1;      
    }
  }else{
    Timer1 = 0;
  }

 // Control from Web 
  if (GiessPumpWeb && !merkerGiessen ) { 
   merkerGiessen = 1;                       // Pump on (Output in Loop)
   GiessPumpWeb = 0;
   //Serial.println(merkerGiessen);
  }
  if (GiessPumpWeb && merkerGiessen ) { 
   merkerGiessen = 0;                       // Pump off (Output in Loop)
   GiessPumpWeb = 0;
    //Serial.println(merkerGiessen);
  }  
} 


// sets every element of str to 0 (clears array)
void StrClear(char *str, char length){
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found, Returns 0 if string not found
char StrContains(char *str, char *sfind){
    char found = 0;
    char index = 0;
    char len;
    len = strlen(str); 
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }
    return 0;
}


