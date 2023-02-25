//Libraries
#include <NsfdEspWMS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include "StringSplitter.h"

//Pin Connections
#define WiFi_RST_IN 19
#define WiFi_RST_OUT 18
#define CONN_LED_RED    21
#define CONN_LED_GREEN  22
#define CONN_LED_BLUE   23

// ########## MOTOR CONTROL ##########
const int MOTOR_DUTY_CYCLE = 200;
// Motor-1
const int MOTOR_LEFT_PIN_1 = 27;
const int MOTOR_LEFT_PIN_2 = 26;
const int MOTOR_LEFT_PIN_ENB = 14; // PWM Control Pins
// Motor-2
const int MOTOR_RIGHT_PIN_1 = 17;
const int MOTOR_RIGHT_PIN_2 = 16;
const int MOTOR_RIGHT_PIN_ENB = 4; // PWM Control Pins
// Setting PWM properties
const int FREQ = 30000;
const int PWM_CHANNEL_MOTOR_LEFT = 1;
const int PWM_CHANNEL_MOTOR_RIGHT = 2;
const int RESOLUTION = 8;

//Operation Configurations
bool opStarted = false;
bool opStopForce = false;
String opState = "";
String opUid = "";

//Wifi Configurations
WiFiClient wifiClient;
NsfdEspWMS wm;
int wifiRstBtnPressMax = 10;
int wifiRstBtnState = 0;
int connectionCheckMax = 30;
String wifiDefaultName = "Hi-Rob";
String wifiDefaultPassword = "12345678";

//Mqtt Configurations
const char* mqttServer = "mqtt.hirob.io";
PubSubClient psClient(wifiClient);
String mqttUserName = "drk";
String mqttPassword = "Amca151200";
String mqttTopicBase = "hirob/";
String mqttTopicDevice = "";
String mqttTopicConnect = "hirob/connect";
String mqttTopicStatu = "hirob/statu";

//Device COnfigurations
String uId;
String deviceId;
String uartData;
bool debug = true;
String ver = "v1.0.0";

//Remote Firmware Update Configurations
long fwContentLength = 0;
bool fwIsValidContentType = false;
String fwHost = "";
String fwFile = "";

//AT Configurations
String atBase = "AT";
String atOk = "AT+OK";
String atDevice = "AT+DEVICE=";
String atStatu = "AT+STATU=";
String atUpdate = "AT+UPDATE=";
String atIsAlive = "AT+ISALIVE";
String atReset = "AT+RESET";
String atDdsDown = "AT+DDSDOWN";
String atOpStop = "AT+OPSTOP";
String atDdsUp = "AT+DDSUP";
String atFreq = "AT+FREQ=";
String atReport = "AT+REPORT=";
String atError = "AT+ERROR=";

//Error Types
String errOpStart = "OPSTART";
String errFreq  = "FREQ";

//Operation Statu Types
String opStatuFree = "FREE";
String opStatuStarted = "STARTED";
String opStatuDone = "DONE";
String opStatuError = "ERROR";

//Motor Type
String motorLeft = "MOTOR_LEFT";
String motorRight = "MOTOR_RIGHT";

//Motor Statu Type
String motorStop = "STOP";
String motorForward = "FORWARD";
String motorBackward = "BACKWARD";

//Motor Statu
String motorLeftStatu = motorStop;
String motorRightStatu = motorStop;

//Connection State Machine
String currentConnLedState = "";
String currentMqttLedState = "";
String connLedStateSettingDevice = "SETTING_DEVICE";
String connLedStateWaitingWifi = "WAITING_WIFI";
String connLedStateErrorWifi = "ERROR_WIFI";
String connLedStateSuccessWifi = "SUCCESS_WIFI";
String mqttLedStateWaitingMqtt = "WAITING_MQTT";
String mqttLedStateErrorMqtt = "ERROR_MQTT";
String mqttLedStateSuccessMqtt = "SUCCESS_MQTT";

String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

//
//WiFi Setup
//
void wifiSetup() {
  if (debug) {
    Serial.println("Setting up Wifi..");
  }
  byte mac[6];
  WiFi.macAddress(mac);
  uId =  String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  uId.toLowerCase();
  deviceId = uId;
  if (debug) {
    Serial.println(deviceId);
  }
}

//
//Mqtt Setup
//
void mqttSetup() {
  if (debug) {
    Serial.println("Setting up Mqtt..");
  }
  mqttTopicDevice = mqttTopicBase + deviceId;
  psClient.setServer(mqttServer, 1883);
  psClient.setCallback(mqttCB);
}

//
//Multi Task Setup
//
void tasksSetup() {
  xTaskCreate(taskLedStateMachine, "TaskLedStateMachine", 10000, NULL, 1, NULL);
  xTaskCreate(taskWiFiSetup, "TaskWiFiSetup", 10000, NULL, 1, NULL);
  xTaskCreate(taskConnectWiFi, "TaskConnectWiFi", 10000, NULL, 1, NULL);
  xTaskCreate(taskConnectMqttServer, "TaskConnectMqttServer", 10000, NULL, 1, NULL);
  xTaskCreate(taskSendUartToMqtt, "TaskSendUartToMqtt", 10000, NULL, 1, NULL);
  xTaskCreate(taskSendIsAliveToMqtt, "TaskSendIsAliveToMqtt", 10000, NULL, 1, NULL);
  xTaskCreate(taskSendOpStatuToMqtt, "TaskSendOpStatuToMqtt", 10000, NULL, 1, NULL);
}

void pinSetup() {
  pinMode(WiFi_RST_IN, INPUT);
  pinMode(WiFi_RST_OUT, OUTPUT);
  digitalWrite(WiFi_RST_OUT, HIGH);
  pinMode(CONN_LED_RED,   OUTPUT);
  pinMode(CONN_LED_GREEN, OUTPUT);
  pinMode(CONN_LED_BLUE,  OUTPUT);
  pinMode(MOTOR_1_PIN_1,  OUTPUT);
  pinMode(MOTOR_1_PIN_2,  OUTPUT);
  pinMode(MOTOR_1_PIN_ENB,  OUTPUT);
  pinMode(MOTOR_2_PIN_1,  OUTPUT);
  pinMode(MOTOR_2_PIN_2,  OUTPUT);
  pinMode(MOTOR_2_PIN_ENB,  OUTPUT);
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  currentConnLedState = connLedStateSettingDevice;
  currentMqttLedState = mqttLedStateErrorMqtt;
  pinSetup();
  wifiSetup();
  mqttSetup();
  tasksSetup();
  finishOperation();
}

void loop() {
  delay(2000);
}

void motorMove(String motorType, String motorStatu) {
  if (motorType == motorLeft) {
    if (motorStatu == motorForward) {
  digitalWrite(MOTOR_LEFT_PIN_1, HIGH);
  digitalWrite(MOTOR_LEFT_PIN_2, LOW);
    } else if (motorStatu == motorBackward) {
 digitalWrite(MOTOR_LEFT_PIN_1, LOW);
  digitalWrite(MOTOR_LEFT_PIN_2, HIGH);
    } else {

    }
  } else if (motorType == motorRight) {
    if (motorStatu == motorForward) {
  digitalWrite(MOTOR_RIGHT_PIN_1, HIGH);
  digitalWrite(MOTOR_RIGHT_PIN_2, LOW);
    } else if (motorStatu == motorBackward) {
 digitalWrite(MOTOR_RIGHT_PIN_1, LOW);
  digitalWrite(MOTOR_RIGHT_PIN_2, HIGH);
    } else {

    }
  } else {}
}

//
//Proc for Received Mqtt Data
//
void mqttCB(char* topic, byte* message, unsigned int length) {
  if (debug) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
  }
  String messageTemp;

  for (int i = 0; i < length; i++) {
    if (debug) {
      Serial.print((char)message[i]);
    }
    messageTemp += (char)message[i];
  }
  if (debug) {
    Serial.println();
  }

  if (String(topic) == mqttTopicDevice) {
    if (messageTemp.indexOf(atBase) > -1) {
      if (debug) {
        Serial.println(messageTemp);
      }
      if (messageTemp.indexOf(atUpdate) > -1) {
        atProcUpdate(messageTemp);
      } else if (messageTemp.indexOf(atIsAlive) > -1) {
        atProcIsAlive();
      } else if (messageTemp.indexOf(atReset) > -1) {
        atProcReset();
      } else if (messageTemp.indexOf(atFreq) > -1) {
        atProcFreq(messageTemp);
      } else if (messageTemp.indexOf(atDdsDown) > -1) {
        atProcDdsDown();
      } else if (messageTemp.indexOf(atOpStop) > -1) {
        atProcOpStop();
      } else if (messageTemp.indexOf(atDdsUp) > -1) {
        atProcDdsUp();
      } else {
        Serial.println(messageTemp);
      }
    }
  }
}

//
//Start Freq
//AT+FREQ=qwe234***qwe234#1234-10#345-20
void atProcFreq(String messageTemp) {
  psClient.publish(mqttTopicDevice.c_str(), atOk.c_str());
  if (!opStarted) {
    freqIn = messageTemp.substring(messageTemp.indexOf(atFreq) + 8);
    xTaskCreate(taskSetFreq, "TaskSetFreq", 10000, NULL, 1, NULL);
  } else {
    psClient.publish(mqttTopicDevice.c_str(), (atError + errOpStart).c_str());
  }
}

//
//Dds Up
//AT+DDSUP
void atProcDdsUp() {
  psClient.publish(mqttTopicDevice.c_str(), atOk.c_str());
  DDS.up();
}

//
//Current Operation Stop
//AT+OPSTOP
void atProcOpStop() {
  psClient.publish(mqttTopicDevice.c_str(), atOk.c_str());
  opStopForce = true;
}

//
//Dds Down
//AT+DDSDOWN
void atProcDdsDown() {
  psClient.publish(mqttTopicDevice.c_str(), atOk.c_str());
  DDS.down();
}

//
//Device Is Alive ?
//AT+ISALIVE
void atProcIsAlive() {
  psClient.publish(mqttTopicDevice.c_str(), atOk.c_str());
}

//
//Reset Device
//AT+RESET
void atProcReset() {
  psClient.publish(mqttTopicDevice.c_str(), atOk.c_str());
  delay(100);
  ESP.restart();
}

//
//Over The Air Update Firmware
//TODO Change AT+UPDATE=fwHost,fwFile sample data
//AT+UPDATE=fwHost,fwFile
void atProcUpdate(String messageTemp) {
  psClient.publish(mqttTopicDevice.c_str(), atOk.c_str());
  String updatePath = messageTemp.substring(messageTemp.indexOf(atUpdate) + 10);
  fwHost = updatePath.substring(0, updatePath.indexOf(','));
  fwFile = updatePath.substring(updatePath.indexOf(',') + 1);
  execOTA();
}

//
//WiFi First Connect or Open Wifi Host
//
void taskWiFiSetup( void * parameter )
{
  WiFi.mode(WIFI_STA);
  bool res;
  res = wm.autoConnect(wifiDefaultName.c_str(), wifiDefaultPassword.c_str());
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    currentConnLedState = connLedStateWaitingWifi;
    if (debug) {
      Serial.println("Connecting to WiFi..");
    }
    delay(100);
  }
  currentConnLedState = connLedStateSuccessWifi;
  if (debug) {
    Serial.println("Connected..");
  }
  vTaskDelete( NULL );
}

//
//Clear States , Uid and Dds
//
void finishOperation() {
  opUid = "";
  opState = opStatuFree;
  DDS.down();
  delay(1000);
  opStopForce = false;
  opStarted = false;
}

//
//Check And Start Freq Operations
//
void taskSetFreq( void * parameter )
{
  finishOperation();
  opStarted = true;
  StringSplitter *spFreq = new StringSplitter(freqIn, '#', 100);
  int icFreq = spFreq->getItemCount();
  if (debug) {
    Serial.println(icFreq);
  }
  for (int i = 0; i < icFreq; i++) {
    String freq = spFreq->getItemAtIndex(i);
    if (debug) {
      Serial.println(freq);
    }
  }
  if (icFreq < 2) {
    psClient.publish(mqttTopicDevice.c_str(), (atError + errFreq).c_str());
  } else {
    opUid = spFreq->getItemAtIndex(0);
    opState = opStatuStarted;
    for (int i = 1; i < icFreq; i++) {
      opState = i;
      if (!opStopForce) {
        String freq = spFreq->getItemAtIndex(i);
        StringSplitter *spTime = new StringSplitter(freq, '-', 100);
        String freqOp = spTime->getItemAtIndex(0);
        String timeOp = spTime->getItemAtIndex(1);
        if (debug) {
          Serial.println(freq);
          Serial.println(freqOp);
          Serial.println(timeOp);
        }
        DDS.down();
        delay(1000);
        DDS.setfreq(freqOp.toInt(), phase);
        delay(timeOp.toInt() * 1000);
        DDS.down();
        delay(1000);
      }
    }
    opState = opStatuDone;
    psClient.publish(mqttTopicDevice.c_str(), (atReport + opUid + "-" + opStatuDone).c_str());
  }
  finishOperation();
  vTaskDelete( NULL );
}

//
//WiFi Connection
//
void taskConnectWiFi( void * parameter )
{
  int connectionCheck = 0;
  int wifiResetCheck = 0;
check_statu:
  wifiRstBtnState = digitalRead(WiFi_RST_IN);
  if (wifiRstBtnState) {
    wifiResetCheck++;
    if (debug) {
      Serial.println("Reset Wifi Pressed..");
      Serial.println(wifiResetCheck);
    }
    if (wifiResetCheck > wifiRstBtnPressMax) {
      wifiResetCheck = wifiRstBtnPressMax;
    }
    if (wifiResetCheck >= wifiRstBtnPressMax) {
      if (debug) {
        Serial.println("Resetting Wifi..");
      }
      wm.resetSettings();
      ESP.restart();

    }
  } else {
    wifiResetCheck = 0;
  }
  if (WiFi.status() == WL_CONNECTED) {
    connectionCheck = 0;
    currentConnLedState = connLedStateSuccessWifi;
    if (debug) {
      Serial.println("Connected..");
    }
  } else {
    connectionCheck++;
    if (connectionCheck >= connectionCheckMax) {
      connectionCheck = connectionCheckMax;
      currentConnLedState = connLedStateErrorWifi;
      if (debug) {
        Serial.println("No WiFi..");
      }
      wm.resetSettings();
      ESP.restart();
    } else {
      currentConnLedState = connLedStateWaitingWifi;
      if (debug) {
        Serial.println("Connecting to WiFi..");
      }
    }
  }
  delay(1000);
  goto check_statu;
  vTaskDelete( NULL );
}

//
//Mqtt Server Connection
//
void taskConnectMqttServer( void * parameter )
{
mqtt_reconnect:
  while (!psClient.connected()) {
    currentMqttLedState = mqttLedStateWaitingMqtt;
    if (debug) {
      Serial.print("Attempting MQTT connection...");
    }
    if (psClient.connect(deviceId.c_str(), mqttUserName.c_str(), mqttPassword.c_str())) {
      currentMqttLedState = mqttLedStateSuccessMqtt;
      if (debug) {
        Serial.println("Connected Mqtt : " + mqttTopicDevice);
      }
      psClient.subscribe(mqttTopicDevice.c_str());
    } else {
      currentMqttLedState = mqttLedStateErrorMqtt;
      if (debug) {
        Serial.print("failed, rc=");
        Serial.print(psClient.state());
        Serial.println(" try again in 5 seconds");
      }
      delay(5000);
    }
  }
  psClient.loop();
  delay(10);
  goto mqtt_reconnect;
  vTaskDelete( NULL );
}

//
//Set Led Status For Connection Type Every 100ms
//
//- SettingDevice -> Blink RED
//- WaitingWifi -> Blink Blue
//- Error Wifi -> Fixed RED
//- Success Wifi -> Fixed BLUE
//- Waiting Mqtt -> Blink GREEN
//- Error Mqtt -> Fixed BLUE
//- Success Mqtt -> Fixed GREEN
void taskLedStateMachine( void * parameter )
{

  bool isOpen = false;
led_state:

  if (currentConnLedState == connLedStateSettingDevice) {
    if (isOpen) {
      analogWrite(CONN_LED_RED,   0);
      analogWrite(CONN_LED_GREEN, 0);
      analogWrite(CONN_LED_BLUE,  0);
      isOpen = false;
    } else {
      analogWrite(CONN_LED_RED,   255);
      analogWrite(CONN_LED_GREEN, 0);
      analogWrite(CONN_LED_BLUE,  0);
      isOpen = true;
    }
  } else if (currentConnLedState == connLedStateWaitingWifi) {
    if (isOpen) {
      analogWrite(CONN_LED_RED,   0);
      analogWrite(CONN_LED_GREEN, 0);
      analogWrite(CONN_LED_BLUE,  0);
      isOpen = false;
    } else {
      analogWrite(CONN_LED_RED,   0);
      analogWrite(CONN_LED_GREEN, 0);
      analogWrite(CONN_LED_BLUE,  255);
      isOpen = true;
    }
  } else if (currentConnLedState == connLedStateErrorWifi) {
    analogWrite(CONN_LED_RED,   255);
    analogWrite(CONN_LED_GREEN, 0);
    analogWrite(CONN_LED_BLUE,  0);
    isOpen = true;
  } else if (currentConnLedState == connLedStateSuccessWifi) {
    if (currentMqttLedState == mqttLedStateWaitingMqtt) {
      if (isOpen) {
        analogWrite(CONN_LED_RED,   0);
        analogWrite(CONN_LED_GREEN, 0);
        analogWrite(CONN_LED_BLUE,  0);
        isOpen = false;
      } else {
        analogWrite(CONN_LED_RED,   0);
        analogWrite(CONN_LED_GREEN, 255);
        analogWrite(CONN_LED_BLUE,  0);
        isOpen = true;
      }
    } else if (currentMqttLedState == mqttLedStateErrorMqtt) {
      analogWrite(CONN_LED_RED,   0);
      analogWrite(CONN_LED_GREEN, 0);
      analogWrite(CONN_LED_BLUE,  255);
      isOpen = true;
    } else if (currentMqttLedState == mqttLedStateSuccessMqtt) {
      analogWrite(CONN_LED_RED,   0);
      analogWrite(CONN_LED_GREEN, 255);
      analogWrite(CONN_LED_BLUE,  0);
      isOpen = true;
    } else {

    }
  } else {

  }

  delay(100);
  goto led_state;
  vTaskDelete( NULL );
}


//
//Sended data for is alive to mqtt server every 1 minute -> drk/connect
//AT+DEVICE=deviceId-v1.0.0
void taskSendIsAliveToMqtt( void * parameter )
{
mqtt_isalive:
  delay(60000);
  String connData = atDevice + deviceId + "-" + ver;
  if (debug) {
    Serial.println("Sending Data MQTT");
    Serial.println(mqttTopicConnect);
    Serial.println(connData);
  }
  psClient.publish(mqttTopicConnect.c_str(), connData.c_str());
  goto mqtt_isalive;
  vTaskDelete( NULL );
}

//
//Sended data for operation statu to mqtt server every 10 second -> drk/statu
//AT+STATU=deviceId-[FREE,uid-1,uid-2...]
void taskSendOpStatuToMqtt( void * parameter )
{
mqtt_statu:
  delay(10000);
  String statuData = "";
  if (opState == opStatuFree) {
    statuData = atStatu + deviceId + "-" + opState;
  } else {
    statuData = atStatu + deviceId + "-" + opUid + "-" + opState;
  }
  if (debug) {
    Serial.println("Sending Data MQTT");
    Serial.println(mqttTopicStatu);
    Serial.println(statuData);
  }
  psClient.publish(mqttTopicStatu.c_str(), statuData.c_str());
  goto mqtt_statu;
  vTaskDelete( NULL );
}

//
//Sended data for received uart to mqtt server -> drk/deviceId
void taskSendUartToMqtt( void * parameter )
{
receive_uart:
  if (Serial.available()) {
    delay(100);
    while (Serial.available() > 0) {
      uartData = Serial.readString();
    }
  }
  if (uartData.indexOf(atBase) > -1) {
    if (debug) {
      Serial.println(uartData);
    }
    psClient.publish(mqttTopicDevice.c_str(), uartData.c_str());
  }
  uartData = "";
  delay(10);
  goto receive_uart;
  vTaskDelete( NULL );
}

void execOTA() {
  WiFiClient fwClient;
  if (debug) {
    Serial.println("Connecting to: " + String(fwHost));
  }
  if (fwClient.connect(fwHost.c_str(), 80)) {
    if (debug) {
      Serial.println("Fetching Bin: " + String(fwFile));
    }
    fwClient.print(String("GET ") + fwFile + " HTTP/1.1\r\n" +
                   "Host: " + fwHost + "\r\n" +
                   "Cache-Control: no-cache\r\n" +
                   "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (fwClient.available() == 0) {
      if (millis() - timeout > 5000) {
        if (debug) {
          Serial.println("Client Timeout !");
        }
        fwClient.stop();
        return;
      }
    }
    while (fwClient.available()) {
      String line = fwClient.readStringUntil('\n');
      line.trim();
      if (!line.length()) {
        break;
      }
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          if (debug) {
            Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          }
          break;
        }
      }
      if (line.startsWith("Content-Length: ")) {
        fwContentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        if (debug) {
          Serial.println("Got " + String(fwContentLength) + " bytes from server");
        }
      }

      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        if (debug) {
          Serial.println("Got " + contentType + " payload.");
        }
        if (contentType == "application/octet-stream") {
          fwIsValidContentType = true;
        }
      }
    }
  } else {
    if (debug) {
      Serial.println("Connection to " + String(fwHost) + " failed. Please check your setup");
    }
  }

  if (debug) {
    Serial.println("contentLength : " + String(fwContentLength) + ", isValidContentType : " + String(fwIsValidContentType));
  }

  if (fwContentLength && fwIsValidContentType) {

    bool canBegin = Update.begin(fwContentLength);
    if (canBegin) {
      if (debug) {
        Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      }
      size_t written = Update.writeStream(fwClient);

      if (written == fwContentLength) {
        if (debug) {
          Serial.println("Written : " + String(written) + " successfully");
        }
      } else {
        if (debug) {
          Serial.println("Written only : " + String(written) + "/" + String(fwContentLength) + ". Retry?" );
        }
      }

      if (Update.end()) {
        if (debug) {
          Serial.println("OTA done!");
        }
        if (Update.isFinished()) {
          if (debug) {
            Serial.println("Update successfully completed. Rebooting.");
          }
          ESP.restart();
        } else {
          if (debug) {
            Serial.println("Update not finished? Something went wrong!");
          }
        }
      } else {
        if (debug) {
          Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        }
      }
    } else {
      if (debug) {
        Serial.println("Not enough space to begin OTA");
      }
      fwClient.flush();
    }
  } else {
    if (debug) {
      Serial.println("There was no content in the response");
    }
    fwClient.flush();
  }
}
