/*
The code below has been pulled from 2 main sources which are:
ESP8266 Web Server to storing ap config to EEPROM by dogrocker - https://gist.github.com/dogrocker/f998dde4dbac923c47c1
and
SKIH3113-Sensor by Ahmad Hanis
- https://github.com/ahmadhanis/SKIH3113-Sensor/blob/main/apmode_example/apmode_example.ino
- https://github.com/ahmadhanis/SKIH3113-Sensor/blob/main/wifi_example/wifi_example.ino
*/

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Define the EEPROM size
#define EEPROM_SIZE 512

// Define pin numbers for LEDs
int wifiled = 16;
int eepromled = 4;

// Variables to store WiFi credentials, device ID, and LED status from EEPROM
String essid, epass, edev, eLED;

// Variables for HTTP server response
String st, content;
int statusCode;

// AP mode SSID and password
const char* ssidap = "Syafu_Server";
const char* passap = "";

// Create an instance of the web server on port 80
ESP8266WebServer server(80);

// Timeout for WiFi connection in milliseconds
const unsigned long wifiTimeout = 30000;

void setup() {
  delay(1000); // Delay for stability
  Serial.begin(115200); // Initialize serial communication
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM

  // Uncomment to reset EEPROM during debugging
  // resetEEPROM();

  // Set pin modes for the LEDs
  pinMode(wifiled, OUTPUT);
  pinMode(eepromled, OUTPUT);
  delay(500);

  // Turn off WiFi LED initially
  digitalWrite(wifiled, LOW);
  Serial.println("Start Wifi");
  Serial.println("Reading Data");

  // Read stored data from EEPROM
  readData();

  // Begin WiFi connection with stored credentials
  WiFi.begin(essid.c_str(), epass.c_str());
  unsigned long startTime = millis();

  // Wait for WiFi connection
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime >= wifiTimeout) {
      // If timeout occurs, switch to AP mode
      Serial.println("Wi-Fi connection timed out. Switching to AP mode.");
      Serial.println("AP Mode. Please connect to http://192.168.4.1 to configure");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ssidap, passap);
      Serial.println(WiFi.softAPIP());
      createWebServer(); // Set up web server for configuration
      server.begin();
      break;
    }
    delay(1000);
    Serial.print(".");
  }

  // If connected to WiFi, print network details
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi!");
    digitalWrite(wifiled, HIGH);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Netmask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    // Set the EEPROM LED status based on stored value
    if (eLED == "0") {
      digitalWrite(eepromled, LOW);
    } else {
      digitalWrite(eepromled, HIGH);
    }
  }
}

void loop() {
  // Handle client requests to the web server
  server.handleClient();
}

void createWebServer() {
  // Define the root URL ("/") handler
  server.on("/", []() {
    // Generate HTML content for configuration page
    content = "<html><head><style>.button {background-color: #3CBC8D;";
    content += "color: white;padding: 5px 10px;text-align: center;text-decoration: none;";
    content += "display: inline-block;font-size: 14px;margin: 4px 2px;cursor: pointer;}";
    content += "input[type=text],[type=password]{width: 100%;padding: 5px 10px;";
    content += "margin: 5px 0;box-sizing: border-box;border: none;";
    content += "background-color: #3CBC8D;color: white;}</style></head><body>";
    content += "<h1>WIFI MANAGER</h1><br>";
    content += "<h3>Current Settings</h3>";
    content += "<table><tr><td><label> Device ID</label></td><td><label>" + edev + "</label></td></tr>";
    content += "<tr><td><label> WiFi SSID</label></td><td><label>" + essid + "</label></td></tr>";
    content += "<tr><td><label> WiFi Pasword</label></td><td><label>" + epass + "</label></td></tr>";
    content += "<tr><td><label> LED Status (1=on/0=off)</label></td><td><label>" + eLED + "</label></td></tr></table><br><br>";
    content += "<form method='get' action='settings'>";
    content += "<h3>New WiFi Settings</h3>";
    content += "<table><tr><td><label>WiFi SSID</label></td><td><input type='text' name = 'setid' length=32 ></td></tr>";
    content += "<tr><td><label> WiFi Password</label></td><td><input type='password' name = 'setpass' length=32></td></tr>";
    content += "<tr><td><label>Device ID</label></td><td><input type='text' name = 'setdev' length=32 ></td></tr>";
    content += "<tr><td><label>Set LED</label></td><td><input type='text' name = 'setLED' length=1 ></td></tr>";
    content += "<tr><td></td><td><input class=button type='submit'></td></tr></table></form>";
    content += "</body></html>";
    server.send(200, "text/html", content);
  });

  // Define the settings URL ("/settings") handler
  server.on("/settings", []() {
    // Get new settings from form submission
    String setid = server.arg("setid");
    String setpass = server.arg("setpass");
    String setdev = server.arg("setdev");
    String setLED = server.arg("setLED");
    Serial.println("WiFi: " + setid);
    Serial.println("Pass: " + setpass);
    Serial.println("Device ID: " + setdev);
    Serial.println("LED Status: " + setLED);

    Serial.println("Writing to EEPROM");

    // Write new WiFi SSID to EEPROM
    for (int i = 0; i < 32; i++) {
      EEPROM.write(i, (i < setid.length()) ? setid[i] : 0);
    }
    // Write new WiFi password to EEPROM
    for (int i = 32; i < 64; i++) {
      EEPROM.write(i, (i - 32 < setpass.length()) ? setpass[i - 32] : 0);
    }
    // Write new device ID to EEPROM
    for (int i = 64; i < 96; i++) {
      EEPROM.write(i, (i - 64 < setdev.length()) ? setdev[i - 64] : 0);
    }
    // Write new LED status to EEPROM
    EEPROM.write(96, (setLED.length() > 0) ? setLED[0] : '0');

    // Commit the changes to EEPROM
    EEPROM.commit();
    Serial.println("Write successful");

    // Read the updated data
    readData();
    content = "<h1>Data Saved!</h1>";
    server.send(200, "text/html", content);

    // Restart the ESP8266 to apply new settings
    delay(2000);
    ESP.restart();
  });
}

void readData() {
  Serial.println("Reading From EEPROM..");
  essid = "";
  epass = "";
  edev = "";
  eLED = "";

  // Read WiFi SSID from EEPROM
  for (int i = 0; i < 32; i++) {
    char c = char(EEPROM.read(i));
    if (c != 0) essid += c;
  }
  Serial.println("Reading Wifi ssid: " + essid);

  // Read WiFi password from EEPROM
  for (int i = 32; i < 64; i++) {
    char c = char(EEPROM.read(i));
    if (c != 0) epass += c;
  }
  Serial.println("Reading Wifi Password: " + epass);

  // Read device ID from EEPROM
  for (int i = 64; i < 96; i++) {
    char c = char(EEPROM.read(i));
    if (c != 0) edev += c;
  }
  Serial.println("Reading Device ID: " + edev);

  // Read LED status from EEPROM
  eLED = char(EEPROM.read(96));
  Serial.println("LED Status EEPROM: " + eLED);

  Serial.println("Reading successful.....");
}

// Function to reset all EEPROM values to 0
void resetEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  delay(500);
}
