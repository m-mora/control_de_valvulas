#include <Arduino.h>
#include <Servo.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

#define PIN_SOLAR   13
#define PIN_BOILER  15
#define PIN_BUTTON  12
#define SOLAR       1
#define BOILER      2
#define ON          0
#define OFF         180
#define STEP        30
#define STEP_DELAY  100
#define CLEARED     0
#define PRESSED     1

//Servo
Servo servo_solar;
Servo servo_boiler;
int current_selection;
int button_state = 0;


// web
// Set web server port number to 80
WiFiServer server(80);

void configModeCallback (WiFiManager *myWiFiManager);
void Web_server ();

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// --------- Functions -----------------------------
// Checks if button was pressed to change valve status
IRAM_ATTR void button_pressed () {
  Serial.println("BUTTON PRESSED!!!");
  button_state = PRESSED;
}

int check_selection() 
{
  // check if the pin was pressed
  // if pressed change the valves position
  if (current_selection == SOLAR) {
    return SOLAR;
  } else {
    return BOILER;
  }
}

void select_heater (int sel)
{
  int pos;
  if (sel == SOLAR) {
    // move the servo in steps
    for (pos = OFF; pos > ON; pos -= STEP) {
      servo_solar.write(pos);
      delay (STEP_DELAY);
    }
    // move the servo in steps
    for (pos = ON; pos < OFF; pos += STEP) {
      servo_boiler.write(pos);
      delay (STEP_DELAY);
    }
    // servo_solar.write(ON);
    // servo_boiler.write (OFF);
    current_selection = SOLAR;
  } else {
    // move the servo in steps
    for (pos = OFF; pos > ON; pos -= STEP) {
      servo_boiler.write(pos);
      delay (STEP_DELAY);
    }
    // move the servo in steps
    for (pos = ON; pos < OFF; pos += STEP) {
      servo_solar.write(pos);
      delay (STEP_DELAY);
    }
    // servo_boiler.write (ON);
    // servo_solar.write (OFF);
    current_selection = BOILER;
  }
}

void setup() {
  Serial.begin(115200);

  servo_solar.attach (PIN_SOLAR);
  servo_boiler.attach(PIN_BOILER);
  select_heater(SOLAR);
  pinMode (PIN_BUTTON,INPUT);
  Serial.println("Attaching int");
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), button_pressed, FALLING);
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  // Changes the hostname
  WiFi.setHostname("MyValveCtlr");
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  //if you get here you have connected to the WiFi
  Serial.println("connected...:)");
 
 server.begin();
}

void loop() {
  // if (digitalRead(PIN_BUTTON) == LOW) {
  //   button_state = PRESSED;
  //   Serial.println ("Button pressed");
  // }

  if (button_state == PRESSED) {
    if (check_selection() == SOLAR) {
     select_heater (BOILER);
      Serial.println("Boiler");
    } else {
     select_heater (SOLAR);
      Serial.println ("Solar");
    }
    button_state = CLEARED;
  }
  delay (200);
  Web_server();
}

// ------------------------------------------------------------------------------------
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void Web_server () {
  String State;
   WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns on and off
            if (header.indexOf("GET /solar/on") >= 0) {
              Serial.println("Solar on");
              State = "on";
              select_heater (SOLAR);
            } else if (header.indexOf("GET /solar/off") >= 0) {
              Serial.println("Solar off");
              State = "off";
              select_heater (BOILER);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Seleccion de Calentador</h1>");
            
            // Display current state, and ON/OFF Solar heater 
            //State = (current_selection == SOLAR) ? "on" : "off";
            client.println("<p>Solar - state " + State + "</p>");
            // If the State is off, it displays the ON button       
            if (State=="off") {
              client.println("<p><a href=\"/solar/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/solar/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    Serial.println (header);
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}