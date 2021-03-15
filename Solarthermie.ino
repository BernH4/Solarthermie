// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "credentials.h" //My Wifi and MQTT credentials

//ESP Stuff
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;
const char* mqtt_server = "192.168.178.143";
/* const char* mqqt_username = MQTT_USERN; */
/* const char* mqtt_password = MQTT_PASSWD; */

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
//

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
/* DeviceAddress roofThermometer, tankThermometer; */

// Assign address manually. The addresses below will need to be changed
// to valid device addresses on your bus. Device address can be retrieved
// by using either oneWire.search(deviceAddress) or individually via
// sensors.getAddress(deviceAddress, index)
DeviceAddress roofThermometer = { 0x28, 0xA7, 0xEC, 0x75, 0xD0, 0x01, 0x3C, 0xD5 };
DeviceAddress tankThermometer   = { 0x28, 0xE0, 0x73, 0x75, 0xD0, 0x01, 0x3C, 0xFD };

void setup_wifi() {

  delay(10);
  // start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    /* if (client.connect(clientId.c_str(), mqqt_username, mqtt_password)) { */
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("sensors/bad/bme280/debug", "MQTT connection established");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup(void)
{
  /* pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output */
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // start serial port
  Serial.begin(9600);
  /* delaystartup(5); */
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Search for devices on the bus and assign based on an index. Ideally,
  // you would do this to initially discover addresses on the bus and then
  // use those addresses and manually assign them (see above) once you know
  // the devices on your bus (and assuming they don't change).
  //
  // method 1: by index
  if (!sensors.getAddress(roofThermometer, 0)) Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(tankThermometer, 1)) Serial.println("Unable to find address for Device 1");

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices,
  // or you have already retrieved all of them. It might be a good idea to
  // check the CRC to make sure you didn't get garbage. The order is
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to roofThermometer
  //if (!oneWire.search(roofThermometer)) Serial.println("Unable to find address for roofThermometer");
  // assigns the seconds address found to tankThermometer
  //if (!oneWire.search(tankThermometer)) Serial.println("Unable to find address for tankThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(roofThermometer);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(tankThermometer);
  Serial.println();

  // set the resolution to 9 bit per device
  sensors.setResolution(roofThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(tankThermometer, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(roofThermometer), DEC);
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(tankThermometer), DEC);
  Serial.println();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.print(tempC);
  client.publish("sensors/test/ds18b20/temp1", String(tempC).c_str());
  /* Serial.print(" Temp F: "); */
  /* Serial.print(DallasTemperature::toFahrenheit(tempC)); */
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

//void delaystartup(seconds)
//{
//  for(int i = seconds; i == 0; i--;)
//  {
//    delay(1000);
//  }
//}

/*
   Main function, calls the temperatures in a loop.
*/
void loop(void)
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  /* print the device information */
  printData(roofThermometer);
  printData(tankThermometer);
}
