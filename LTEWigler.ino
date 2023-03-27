/*
  FILE: AllFunctions.ino
  AUTHOR: Koby Hale
  PURPOSE: Test functionality
*/

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024  // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

/*
   Tests enabled
*/
#define TINY_GSM_TEST_GPRS true
#define TINY_GSM_TEST_GPS false
#define TINY_GSM_POWERDOWN false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "h2g2";  //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>

//#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
////#include <StreamDebugger.h>
//StreamDebugger debugger(SerialAT, Serial);
//TinyGsm modem(debugger);
//#else
TinyGsm modem(SerialAT);
//#endif

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60           // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD 115200
#define PIN_DTR 25
#define PIN_TX 27
#define PIN_RX 26
#define PWR_PIN 4

#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13
#define LED_PIN 12


int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;

// Server details
const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
const int port = 80;


TinyGsmClient client(modem);
HttpClient http(client, server, port);


void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);



  Serial.println("\nWait...");

  delay(1000);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  if (!modem.restart()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  Serial.println("Initializing modem...");
  if (!modem.init()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  delay(500);
  Serial.println("Modem Info: " + modemInfo);


  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }


  modem.sendAT("+CFUN=0 ");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=0  false ");
  }
  delay(200);

  /*
      2 Automatic
      13 GSM only
      38 LTE only
      51 GSM and LTE only
    * * * */
  String res;
  res = modem.setNetworkMode(38);
  if (res != "1") {
    DBG("setNetworkMode  false ");
    return;
  }
  delay(200);

  /*
      1 CAT-M
      2 NB-Iot
      3 CAT-M and NB-IoT
    * * */
  res = modem.setPreferredMode(1);
  if (res != "1") {

    DBG("setPreferredMode  false ");
    return;
  }
  delay(200);

  /*AT+CBANDCFG=<mode>,<band>[,<band>…]
     * <mode> "CAT-M"   "NB-IOT"
     * <band>  The value of <band> must is in the band list of getting from  AT+CBANDCFG=?
     * For example, my SIM card carrier "NB-iot" supports B8.  I will configure +CBANDCFG= "Nb-iot ",8
     */
  /* modem.sendAT("+CBANDCFG=\"NB-IOT\",8 ");
     if (modem.waitResponse(10000L) != 1) {
         DBG(" +CBANDCFG=\"NB-IOT\" ");
     }
     delay(200);*/

  modem.sendAT("+CFUN=1 ");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=1  false ");
  }
  delay(200);



  SerialAT.println("AT+CGDCONT?");
  delay(500);
  if (SerialAT.available()) {
    input = SerialAT.readString();
    for (int i = 0; i < input.length(); i++) {
      if (input.substring(i, i + 1) == "\n") {
        pieces[counter] = input.substring(lastIndex, i);
        lastIndex = i + 1;
        counter++;
      }
      if (i == input.length() - 1) {
        pieces[counter] = input.substring(lastIndex, i);
      }
    }
    // Reset for reuse
    input = "";
    counter = 0;
    lastIndex = 0;

    for (int y = 0; y < numberOfPieces; y++) {
      for (int x = 0; x < pieces[y].length(); x++) {
        char c = pieces[y][x];  //gets one byte from buffer
        if (c == ',') {
          if (input.indexOf(": ") >= 0) {
            String data = input.substring((input.indexOf(": ") + 1));
            if (data.toInt() > 0 && data.toInt() < 25) {
              modem.sendAT("+CGDCONT=" + String(data.toInt()) + ",\"IP\",\"" + String(apn) + "\",\"0.0.0.0\",0,0,0,0");
            }
            input = "";
            break;
          }
          // Reset for reuse
          input = "";
        } else {
          input += c;
        }
      }
    }
  } else {
    Serial.println("Failed to get PDP!");
  }


  Serial.println("\n\n\nWaiting for network...");
  if (!modem.waitForNetwork()) {
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }
}

void loop() {

  // Restart takes quite some time
  // To skip it, call init() instead of restart()




  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  //if (modem.isGprsConnected()) { Serial.println("GPRS connected"); }


  Serial.println(F("Performing HTTP GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  /*Serial.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    Serial.println("    " + headerName + " : " + headerValue);
    Serial.println("    ");
  }
  */

  int length = http.contentLength();
  if (length >= 0) {
    //Serial.print(F("Content length is: "));
    //Serial.println(length);
  }
  if (http.isResponseChunked()) {
    //Serial.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  Serial.println(F("Response:"));
  Serial.println(body);

  //Serial.print(F("Body length is: "));
  //Serial.println(body.length());

  // Shutdown

  http.stop();
  Serial.println(F("Server disconnected"));
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();








  
}

