#include <WiFi.h>
#include <PubSubClient.h>
#include <driver/ledc.h>

/* change it with your ssid-password */
const char *ssid = "Movsec 2";
const char *password = "movsec2022!";
/* this is the IP of PC/raspberry where you installed MQTT Server
on Wins use "ipconfig"
on Linux use "ifconfig" to get its IP address */
const char *mqtt_server = "192.168.1.55";
const char *topic = "moves";
/* create an instance of PubSubClient client */
WiFiClient espClient;
PubSubClient client(espClient);

// ########## MOTOR CONTROL ##########
// Motor-1
const int motorPin1 = 27;
const int motorPin2 = 26;
const int enablePin = 14; // PWM Control Pins

// Motor-2
const int motorPin3 = 17;
const int motorPin4 = 16;
const int enablePin2 = 4; // PWM Control Pins

// Setting PWM properties
const int freq = 30000;
const int pwmChannel1 = enablePin;
const int pwmChannel2 = enablePin2;
const int resolution = 8;

// ########## Direction Functions ###########

void rigth()
{
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
  delay(2000);
}

void left()
{
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  delay(2000);
}

void forward()
{
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  delay(2000);
}

void backward()
{
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, HIGH);
  delay(2000);
}

void allLow()
{
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
  delay(2000);
}

// ########## Duty Cycle PWM ##########
int dutyCycle1 = 200;
int dutyCycle2 = 200;

unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

char moves[100];
int moveIndex = 0;

void executeMoves()
{
  Serial.println("Executing Moves");

  for (int m = 0; m < 100; m++)
  {
    Serial.println(moves[m]);

    if (moves[m] == 'r')
    {
      Serial.println("r");
      rigth();
    }
    else if (moves[m] == 'l')
    {
      Serial.println("l");
      left();
    }
    else if (moves[m] == 'f')
    {
      Serial.println("f");
      forward();
    }
    else if (moves[m] == 'b')
    {
      Serial.println("b");
      backward();
    }
    else if (moves[m] == 's')
    {
      Serial.println("s");
      allLow();
    }
    else if (moves[m] == 'p')
    {
      Serial.println("p");
      break;
    }
    else
    {
      Serial.println("Error");
    }
  }
}

void receivedCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received: ");
  Serial.println(topic);

  Serial.print("payload: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }

  Serial.println();

  moves[moveIndex] = (char)payload[0];
  Serial.println(moves[moveIndex]);

  if (moves[moveIndex] == 'p')
  {
    Serial.println("p detected");
    executeMoves();
    moveIndex = 0;
  }
  else
  {
    moveIndex++;
  }
}

void mqttconnect()
{
  /* Loop until reconnected */
  while (!client.connected())
  {
    Serial.print("MQTT connecting ...");
    /* client ID */
    String clientId = "ESP32Client";
    /* connect now */
    if (client.connect(clientId.c_str(), "guest", "guest"))
    {
      Serial.println("connected");
      /* subscribe topic with default QoS 0*/
      client.subscribe(topic);
    }
    else
    {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      /* Wait 5 seconds before retrying */
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  /* set led as output to control led on-off */
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  pinMode(enablePin2, OUTPUT);

  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  /* configure the MQTT server with IPaddress and port */
  client.setServer(mqtt_server, 1883);
  /* this receivedCallback function will be invoked
  when client received subscribed topic */
  client.setCallback(receivedCallback);
}

void loop()
{
  allLow();
  /* if client was disconnected then try to reconnect again */
  if (!client.connected())
  {
    mqttconnect();
  }
  /* this function will listen for incomming
  subscribed topic-process-invoke receivedCallback */
  client.loop();
}
