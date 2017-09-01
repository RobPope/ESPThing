#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <RCSwitch.h>
#include <PubSubClient.h>

/**Declare variables**/
//Device Name
const char* deviceName = "ESP_Thing";
//Network Details
const char* ssid = "SSID";
const char* wifiPW = "Password";
const IPAddress staticIP = (192,168,4,1);

//MQTT
const IPAddress mqttServerIP = (192,168,1,1);
const char* mqttTopic = "#";
char message_buff[100];   // initialise storage buffer (i haven't tested to this capacity.)

//Variables to store after reboot
struct vars {
  char deviceName[16];
  char ssid[32];
  char wifiPW[64];
  IPAddress staticIP;
  IPAddress mqttServerIP;
  char mqttTopic[64];
};

void readVars() {
  Serial.println("Start Read");
  vars currentVars;
  EEPROM.get(0, currentVars);
  Serial.println("Got Vars");
  ssid = currentVars.ssid;
  Serial.println(ssid);
  wifiPW = currentVars.wifiPW;
  Serial.println(wifiPW);
}

void saveVars() {
  vars newVars = {
    "ESPTest1",
    "SSID",
    "PASSWORD",
    (192,168,1,1),
    (192,168,1,19),
    "hello/World"
  };
  
  EEPROM.put(0, newVars);
}


WiFiClient wifiClient;
ESP8266WebServer wifiServer(80);
PubSubClient client(mqttServerIP, 1883, callback, wifiClient);

void updatePage(){
  String pageStart = "<html><head><title='ESP Thing Update Page'></title></head><body><div id='header'><h1>ESP Update</h1></div><div id='form'><form>";
  String pageEnd = "</div></body></html>";

  //Create form
  String form = "ESP Name:</br><input type='text' name='deviceName'></br></br>";
  form += "SSID:</br><input type='text' name='ssid'></br>";
  form += "Password:</br><input type='text' name='WiFiPassword'></br>";
  form += "IP Address:</br><input type='text' name='staticIP'></br></br>";
  form += "MQTT Server:</br><input type='text' name='mqttServer'></br>";
  form += "MQTT Topic:</br><input type='text' name='mqttTopic'></br>";

  wifiServer.send(200, "text/plain", pageStart + form + pageEnd);
}


RCSwitch mySwitch = RCSwitch();

// Handle new items on the topic subscribed to
void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  
  Serial.println("Message arrived:  topic: " + String(topic));
  Serial.println("Length: " + String(length,DEC));
  
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  
  String msgString = String(message_buff);
  
  Serial.println("Payload: " + msgString);
  if (msgString == "1"){      // 1 = On
    mySwitch.switchOn(3,2);
    Serial.println("Switching Lamp On"); 
 }
 else if (msgString == "0"){  // 0 = Off
    mySwitch.switchOff(3,2);
    Serial.println("Switching Lamp Off"); 
 }
}



void setup() {
  
  Serial.begin(115200);
  Serial.println("Booting");
  saveVars();
  Serial.println("Entered Setup");
  readVars();
  
  IPAddress Ip(192,168,1,45);
  IPAddress Gateway(192,168,1,1);
  IPAddress Subnet(255,255,255,0);
  WiFi.config(Ip, Gateway, Subnet);
  WiFi.mode(WIFI_STA);
//  WiFi.begin(ssid, wifiPW);
//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    Serial.println("Connection Failed! Rebooting...");
//    delay(5000);
//    ESP.restart();
//  }

  //WiFi Server to serve update page
  wifiServer.begin();
  Serial.println("WiFi Server Started on " + WiFi.localIP());
  wifiServer.on("/", updatePage);


  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("MQTT_Client2a");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //  connection to broker script.
  if (client.connect("arduinoClient")) {
    client.subscribe(mqttTopic);   // i should probably just point this to the varialbe above.
  }

  // 433Mhz transmitter is connected to Arduino Pin #0  
  mySwitch.enableTransmit(0);
}

void loop() {
  ArduinoOTA.handle();

  //saveVars();
  // Check for new items on topic
  client.loop();

  //Webserver
  wifiServer.handleClient();
}
