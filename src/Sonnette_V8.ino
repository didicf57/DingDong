/*********
  DING DONG Project for connected Door Alarm
  Dec 2022 - FP

  Use of Rui Santos project
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-input-data-html-form/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <esp_wps.h>
#include <AsyncElegantOTA.h>

#include "Index_LP_V1.7.h" // fichier page Web

// Variable gestion button
#define DebounceValue 200 // Debounce 200ms rebond
#define LongValue 5000    // Appui long 5s

#define CPT_WIFI_LOST_MAX 5    // Cycle reconnection avant Reboot
#define WIFI_TIMEOUT_LOOP 12000 // WiFi Connection attempt timeout loop (60s)
#define LED 2
#define PIN_BUTTON 23          // bouton sonnette
#define Timer_Slow 2000000     // 2 sec
#define Timer_Fast 500000      // 500 ms
#define Timer_Very_Fast 100000 // 100ms

#define Timer_Button 20 // Affichage bouton appuye 20x100ms = 2s et filtre notification
/*
Change the definition of the WPS mode from WPS_TYPE_PBC to WPS_TYPE_PIN in
the case that you are using pin type WPS
*/
#define ESP_WPS_MODE WPS_TYPE_PBC
#define ESP_MANUFACTURER "ESPRESSIF"
#define ESP_MODEL_NUMBER "ESP32"
#define ESP_MODEL_NAME "ESPRESSIF IOT"
#define ESP_DEVICE_NAME "ESP STATION"

AsyncWebServer server(80);

// Variable declaration
// use for HTML variable
const char *PARAM_STRING_NR_WiFi = "WiFi Network Name";
const char *PARAM_STRING_MDP_WiFi = "WiFi Password";
const char *PARAM_STRING_ID1_SPSH = "SimplePush ID 1";
const char *PARAM_STRING_ID2_SPSH = "SimplePush ID 2";
const char *PARAM_STRING_CONNECTION = "Connection";
const char *PARAM_STRING_ERASE = "RESET";
const char *PARAM_STRING_TEST[] = {"", "Test1", "Test2"};

String WLAN_Connect = "Not connected";
String WLAN_IP;
String NOTIF[] = {"", "unknown", "unknown"};
String TEST_BUTTON = "OFF";
String String_API;
String Test_Message = "This_is_a_test_message";
String DingDong_Message = "DING_DONG";
unsigned long lastInterrupt;
boolean Button_Now = true;
boolean Button_Valid = false;
hw_timer_t *My_timer = NULL; // Timer clignotement LED

long holdingTime;
long previousHoldingTime;
unsigned long firstButtonPressTime;
byte buttonState;
byte previousButtonState = HIGH;
boolean LongButton = false;

boolean Connection_request = false;
boolean Test[] = {false, false, false};

unsigned long startAttemptTime;

int Timer_Led = Timer_Fast;
int Timer_CPT_Led = Timer_Led;
int Timer_CPT_Button = 0;
int CPT_WIFI_LOST = 0;

static esp_wps_config_t config;

String readFile(fs::FS &fs, const char *path)
{
    // Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path, "r");
    if (!file || file.isDirectory())
    {
        Serial.println("- empty file or failed to open file");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent += String((char)file.read());
    }
    file.close();
    // Serial.println(fileContent);
    return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);
    File file = fs.open(path, "w");
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
}
void Reset_files()
{
    deleteFile(SPIFFS, "/N_WiFi.txt");
    deleteFile(SPIFFS, "/P_WiFi.txt");
    deleteFile(SPIFFS, "/ID1_SPSH.txt");
    deleteFile(SPIFFS, "/ID2_SPSH.txt");
    writeFile(SPIFFS, "/Connect.txt", "NoCNX");
}

void ButtonManagement()
{
    buttonState = digitalRead(PIN_BUTTON);
    if (buttonState == LOW && previousButtonState == HIGH && (millis() - firstButtonPressTime) > DebounceValue)
    {
        firstButtonPressTime = millis();
        LongButton = false;
    }
    holdingTime = (millis() - firstButtonPressTime);
    if (holdingTime > DebounceValue / 4)
    {
        if (buttonState == LOW && holdingTime > previousHoldingTime) //
        {
            // Serial.println("button is pressed");
            if (holdingTime > LongValue && LongButton == false)
            {
                Serial.println("long button press");
                LongButton = true;
                timerAlarmWrite(My_timer, Timer_Very_Fast, true);
                timerAlarmEnable(My_timer); // Just Enable
                writeFile(SPIFFS, "/Connect.txt", "NoCNX");
                deleteFile(SPIFFS, "/N_WiFi.txt");
                deleteFile(SPIFFS, "/P_WiFi.txt");
                WLAN_Connect = "WPS Started";
                Serial.println("Starting WPS");
                WiFi.onEvent(WiFiEvent);
                wpsInitConfig();
                wpsStart();
            }
        }
        if (buttonState == HIGH && previousButtonState == LOW)
        {
            if (holdingTime <= LongValue)
            {
                Serial.println("short button press");
                LongButton = true;
                TEST_BUTTON = "ON";
                Serial.println(Timer_CPT_Button);
                if (Timer_CPT_Button == 0)
                {
                    Serial.println("Envoi notification");
                    NOTIF[1] = Notif_SimplePush(readFile(SPIFFS, "/ID1_SPSH.txt").c_str(), DingDong_Message);
                    NOTIF[2] = Notif_SimplePush(readFile(SPIFFS, "/ID2_SPSH.txt").c_str(), DingDong_Message);
                    Timer_CPT_Button = Timer_Button;
                }
            }
        }
    }
    previousButtonState = buttonState;
    previousHoldingTime = holdingTime;

    if (Timer_CPT_Button > 0)
        Timer_CPT_Button--;
    else
        TEST_BUTTON = "OFF";
}

void wpsInitConfig()
{
    config.wps_type = ESP_WPS_MODE;
    strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
    strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
    strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
    strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}

void wpsStart()
{
    if (esp_wifi_wps_enable(&config))
    {
        Serial.println("WPS Enable Failed");
    }
    else if (esp_wifi_wps_start(0))
    {
        Serial.println("WPS Start Failed");
    }
}

void wpsStop()
{
    if (esp_wifi_wps_disable())
    {
        Serial.println("WPS Disable Failed");
    }
}

String wpspin2string(uint8_t a[])
{
    char wps_pin[9];
    for (int i = 0; i < 8; i++)
    {
        wps_pin[i] = a[i];
    }
    wps_pin[8] = '\0';
    return (String)wps_pin;
}

void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("Station Mode Started");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.println("Got IP");
        Serial.println("Connected to : " + String(WiFi.SSID()));
        Serial.println("Password : " + String(WiFi.psk()));
        Serial.print("Got IP: ");
        Serial.println(WiFi.localIP());
        writeFile(SPIFFS, "/N_WiFi.txt", String(WiFi.SSID()).c_str());
        writeFile(SPIFFS, "/P_WiFi.txt", String(WiFi.psk()).c_str());
        writeFile(SPIFFS, "/Connect.txt", "Cnx");
        WLAN_Connect = "Connected";
        timerAlarmWrite(My_timer, Timer_Slow, true);
        timerAlarmEnable(My_timer); // Just Enable
        break;
    // case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    //   Serial.println("Disconnected from station, attempting reconnection");
    //   WiFi.reconnect();
    //   break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:

        Serial.println("Success Stop WPS -> Start WiFi");
        wpsStop();
        delay(10);
        WiFi.reconnect();
        //delay(1000);
        Serial.println("Connected to : " + String(WiFi.SSID()));
        Serial.println("Password : " + String(WiFi.psk()));
        Serial.print("Got IP: ");
        Serial.println(WiFi.localIP());
        //delay(1000);
        if ( (WiFi.SSID() != "") & (WiFi.psk() != "") ) {
            writeFile(SPIFFS, "/N_WiFi.txt", String(WiFi.SSID()).c_str());
            writeFile(SPIFFS, "/P_WiFi.txt", String(WiFi.psk()).c_str());
            writeFile(SPIFFS, "/Connect.txt", "Cnx");
            WLAN_Connect = "Connected";
            timerAlarmWrite(My_timer, Timer_Slow, true);
            timerAlarmEnable(My_timer); // Just Enable  
        }
        break;
        
    case ARDUINO_EVENT_WPS_ER_FAILED:
        Serial.println("WPS Failed, retrying");
        wpsStop();
        wpsStart();
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
        Serial.println("WPS Timedout, retrying");
        wpsStop();
        wpsStart();
        break;
    case ARDUINO_EVENT_WPS_ER_PIN:
        Serial.println("WPS_PIN = " + wpspin2string(info.wps_er_pin.pin_code));
        break;
    default:
        break;
    }
}

void IRAM_ATTR onTimer()
{                                         // Int de clignotement LED
    digitalWrite(LED, !digitalRead(LED)); // sous programme clignotement LED
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

String Notif_SimplePush(String ID_SPSH, String Message)
{
    String RESULT_NOTIF;
    Serial.println("Lancement Notification");
    Serial.print("   Message SimplePush : ");
    Serial.println(Message);
    if (ID_SPSH != "")
    {
        WiFiClient client;
        HTTPClient http; // Declare an object of class HTTPClient
        // Specify request destination

        String_API = "http://api.simplepush.io/send/";
        String_API = String_API + ID_SPSH + "/";
        String_API = String_API + Message;

        Serial.println(String_API);
        http.begin(client, String_API);
        int httpCode = http.GET(); // Send the request
        Serial.print("httpCode : ");
        Serial.println(httpCode);

        switch (httpCode)
        {
        case 200:
            RESULT_NOTIF = "Message sent";
            break;
        case 400:
            RESULT_NOTIF = "Bad request";
            break;
        case 404:
            RESULT_NOTIF = "Not Found";
            break;
        case 500:
            RESULT_NOTIF = "Internal Server Error";
            break;

        default:
            RESULT_NOTIF = http.errorToString(httpCode).c_str();
            break;
        }
    }
    else
        RESULT_NOTIF = "Missing ID";
    return (RESULT_NOTIF); // Return result of HTTP API request OK, erreur..
}

// Replaces placeholder with stored values
String processor(const String &var)
{
    if (var == "inputString_N_WiFi")
    {
        return readFile(SPIFFS, "/N_WiFi.txt");
    }
    else if (var == "inputString_ID1_SPSH")
    {
        return readFile(SPIFFS, "/ID1_SPSH.txt");
    }
    else if (var == "inputString_ID2_SPSH")
    {
        return readFile(SPIFFS, "/ID2_SPSH.txt");
    }
    else if (var == "inputString_WLAN_Connect")
    {
        return WLAN_Connect;
    }
    else if (var == "inputString_WLAN_IP")
    {
        return WLAN_IP;
    }
    else if (var == "inputString_RSSI")
    {
        return String(WiFi.RSSI());
    }
    else if (var == "inputString_NOTIF1")
    {
        return NOTIF[1];
    }
    else if (var == "inputString_NOTIF2")
    {
        return NOTIF[2];
    }
    else if (var == "inputString_BUTTON")
    {
        return TEST_BUTTON;
    }
    return String();
}

void WiFiManagement()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (readFile(SPIFFS, "/Connect.txt") == "Cnx")
        {
            Serial.println("Connection lost");
            timerAlarmWrite(My_timer, Timer_Fast, true);
            timerAlarmEnable(My_timer); // Just Enable
            WiFi.disconnect();
            delay(1000);
            WLAN_Connect = "Connection lost - Restart " + String(CPT_WIFI_LOST);
            Serial.println("Relance Connection WIFI BOX");
            WiFi.begin(readFile(SPIFFS, "/N_WiFi.txt").c_str(), readFile(SPIFFS, "/P_WiFi.txt").c_str());
            startAttemptTime = millis();

            while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_LOOP)
            {
                delay(50);
                Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                WLAN_Connect = "Connected";
                CPT_WIFI_LOST = 0;
                WLAN_IP = WiFi.localIP().toString();
                timerAlarmWrite(My_timer, Timer_Slow, true);
                timerAlarmEnable(My_timer); // Just Enable timer
                WiFi.setAutoReconnect(true);
                WiFi.persistent(true);
            }
            else
            {
                WLAN_Connect = "Not Connected";
                CPT_WIFI_LOST++;
                Serial.print("CPT_WIFI_LOST : ");
                Serial.println(CPT_WIFI_LOST);
                if (CPT_WIFI_LOST == CPT_WIFI_LOST_MAX)
                {
                    Serial.println("RESTART");
                    ESP.restart();
                }
            }
        }
    }
}

void NotifManagement()
{
    if (Test[1])
    {
        NOTIF[1] = Notif_SimplePush(readFile(SPIFFS, "/ID1_SPSH.txt").c_str(), Test_Message);
        Test[1] = false;
    }
    if (Test[2])
    {
        NOTIF[2] = Notif_SimplePush(readFile(SPIFFS, "/ID2_SPSH.txt").c_str(), Test_Message);
        Test[2] = false;
    }
}

void setup()
{
    Serial.begin(115200);
    // Configuration clignotement LED
    pinMode(LED, OUTPUT);
    My_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(My_timer, &onTimer, true);
    timerAlarmWrite(My_timer, Timer_Fast, true); // Configuration Timer blink timing
    timerAlarmEnable(My_timer);                  // Just Enable Timer

    pinMode(PIN_BUTTON, INPUT_PULLUP);
    //  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), ButtonAction , FALLING );

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    Serial.print("Setting AP (Access Point)…");
    //WiFi.mode(WIFI_MODE_APSTA);
    WiFi.mode(WIFI_MODE_STA); 

    /*WiFi.softAP(ssid, password);

    delay(100);

    IPAddress AP_LOCAL_IP(192, 168, 4, 4);
    IPAddress AP_GATEWAY_IP(192, 168, 4, 4);
    IPAddress AP_NETWORK_MASK(255, 255, 255, 0);

    if (!WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY_IP, AP_NETWORK_MASK))
    {
        Serial.println("AP Config Failed");
        return;
    } */
    // Send web page with input fields to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String inputMessage;

    if (request->hasParam(PARAM_STRING_ID1_SPSH)) {
      inputMessage = request->getParam(PARAM_STRING_ID1_SPSH)->value();
      writeFile(SPIFFS, "/ID1_SPSH.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STRING_ID2_SPSH)) {
      inputMessage = request->getParam(PARAM_STRING_ID2_SPSH)->value();
      writeFile(SPIFFS, "/ID2_SPSH.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STRING_ERASE)) {
      inputMessage = request->getParam(PARAM_STRING_ERASE)->value();
      Serial.println("Efface fichiers");
      Reset_files();
      timerAlarmWrite(My_timer, Timer_Fast, true);
      timerAlarmEnable(My_timer); //Just Enable
      WLAN_Connect = "Not connected";
      WiFi.disconnect();
    }
    else if (request->hasParam(PARAM_STRING_TEST[1])) {
      inputMessage = request->getHeader(1)->name().c_str();
      Test[1] = true;
    }
    else if (request->hasParam(PARAM_STRING_TEST[2])) {
      inputMessage = request->getHeader(1)->name().c_str();
      Test[2] = true;
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    //request->send(200, "text/text", inputMessage);
    request->send(204); });
    server.on("/WiFi_Name", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", String(readFile(SPIFFS, "/N_WiFi.txt")).c_str()); });
    server.on("/WLAN_Connect", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", String(WLAN_Connect).c_str()); });
    server.on("/WLAN_IP", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", WiFi.localIP().toString().c_str()); });
    server.on("/RSSI", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", String(WiFi.RSSI()).c_str()); });
    server.on("/NOTIF1", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", String(NOTIF[1]).c_str()); });
    server.on("/NOTIF2", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", String(NOTIF[2]).c_str()); });
    server.on("/BUTTON", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", String(TEST_BUTTON).c_str()); });
    // lancement mDNS
    if (MDNS.begin("doorbell"))
    {
        Serial.println("mDNS Started !");
    }
    // lancement serveur Web
    AsyncElegantOTA.begin(&server); // Start AsyncElegantOTA

    server.onNotFound(notFound);
    server.begin();

    // si redémarrage , verifie si connecté et relance la connexion.
    if (readFile(SPIFFS, "/Connect.txt") == "Cnx")
    {
        Serial.println("Lancement Connection WIFI BOX");
        WiFi.begin(readFile(SPIFFS, "/N_WiFi.txt").c_str(), readFile(SPIFFS, "/P_WiFi.txt").c_str());
        startAttemptTime = millis();

        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_LOOP)
        {
            delay(50);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            WLAN_Connect = "Connected";
            WLAN_IP = WiFi.localIP().toString();
            timerAlarmWrite(My_timer, Timer_Slow, true);
            timerAlarmEnable(My_timer); // Just Enable timer
            WiFi.setAutoReconnect(true);
            WiFi.persistent(true);
        }
    }
}

void loop()
{
    WiFiManagement();   // check the connection status
    ButtonManagement(); // check the button status
    NotifManagement();  // notification launch if needed
    delay(100);
}