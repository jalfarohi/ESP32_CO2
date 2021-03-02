// JORGE ALFARO
// Weather Station
// Obj: Sensar Temp, Hum, PresAt, Humedad Suelo, Caida agua y CO2
// Board info : NodeMCU -32S

// Load required libraries
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <NTPClient.h>    //Time sync used to add timestamp to message
#include <PubSubClient.h>
#include <MHZ.h>
#include "iot.h"
#include "conf.h"
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 60000;

// WEB
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");
 
String payload;
// variables to Store values
// String s_Temp, s_Hum, s_Pres, s_Luz, s_Piso, s_Agua;
float f_temp, f_pressu, f_altid, f_hum, f_luz, f_piso, f_agua;
int cont, i_co2_uart, i_co2_pwm;

unsigned long getDataTimer = 0;

// Replace with your network credentials
const char* ssid     = "mired";
const char* password = "Eero1367!";

const int LuzPin = 32; // Luz
const int SoilPin = 35; // Hum tierra
const int AguaPin = 34; // Plub
#define CO2_IN 14
#define MH_Z19_RX 16  // D7
#define MH_Z19_TX 17  // D6
MHZ co2(MH_Z19_RX, MH_Z19_TX, CO2_IN, MHZ19B);

WiFiClientSecure secureClient = WiFiClientSecure();

PubSubClient mqttClient(secureClient);

IOT iotclient(secureClient, mqttClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void ReadBME()
{
  
  f_temp = bme.readTemperature();
  f_pressu = bme.readPressure() / 100.0F;
  f_altid = bme.readAltitude(SEALEVELPRESSURE_HPA);
  f_hum = bme.readHumidity();
}

// Leer CO2
void leerco2()
{
  i_co2_uart = co2.readCO2UART();
  Serial.print("PPMuart: ");

  if (i_co2_uart > 0) {
    Serial.print(i_co2_uart);
  } else {
    Serial.print("n/a");
  }

  i_co2_pwm = co2.readCO2PWM();
  Serial.print(", PPMpwm: ");
  Serial.print(i_co2_pwm);

}

// Leer humedad suelo
void leerSoil()
{   
   // leemos SOIL
    f_piso = analogRead(SoilPin);
}

// Leer intensidad solar
void leerluz()
{
     // leemos LUZ
    f_luz = analogRead(LuzPin);
}


String processor(const String& var){
// f_temp, f_pressu, f_altid, f_hum, f_luz, f_piso, f_agua; i_co2_uart
  if(var == "temp"){
    return String(f_temp);
  }
  else if(var == "presion"){
    return String(f_pressu);
  }
  else if(var == "altitud"){
    return String(f_altid);
  }
  else if(var == "hum"){
    return String(f_hum);
  }
  else if(var == "luz"){
    return String(f_luz);
  }
  else if(var == "soil"){
    return String(f_piso);
  }   
  else if(var == "co2"){
    return String(i_co2_pwm);
  }
  return String();
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Weather Station</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p { font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #50B8B4; color: white; font-size: 1rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
    .reading { font-size: 1.4rem; }
  </style>
</head>
<body>
  <div class="topnav">
    <h1>Estacion de Clima </h1>  
  </div>
  <div class="content">
    <div class="cards">
      <div class="card">
        <p><i class="fas fa-thermometer" style="color:#00add6;"></i> Temperatura</p><p><span class="reading"><span id="temp">%temp%</span> &deg;C</span></p>
      </div>
      <div class="card">
        <p><i class="fas fa-weight-hanging" style="color:#e1e437;"></i> Presion</p><p><span class="reading"><span id="presion">%presion%</span> hPa</span></p>
      </div>
      <div class="card">
        <p><i class="fas fa-mountain" style="color:#e1e437;"></i> Altura</p><p><span class="reading"><span id="altitud">%f_pressu%</span> mts</span></p>
      </div>
      <div class="card">
        <p><i class="fas fa-tint" style="color:#e1e437;"></i> Humedad</p><p><span class="reading"><span id="hum">%hum%</span> </span></p>
      </div>
      <div class="card">
        <p><i class="fas fa-sun" style="color:#e1e437;"></i> Int. Luminosa</p><p><span class="reading"><span id="luz">%luz%</span> </span></p>
      </div>
      <div class="card">
        <p><i class="fas fa-cloud-rain" style="color:#e1e437;"></i> Humedad Suelo</p><p><span class="reading"><span id="soil">%soil%</span> </span></p>
      </div>
      <div class="card">
        <p><i class="fas fa-fill-drip" style="color:#e1e437;"></i> CO2</p><p><span class="reading"><span id="co2">%co2%</span> ppm</span></p>
      </div>
      
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 source.addEventListener('temp', function(e) {
  console.log("temp", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);
 source.addEventListener('presion', function(e) {
  console.log("presion", e.data);
  document.getElementById("presion").innerHTML = e.data;
 }, false);
  source.addEventListener('altitud', function(e) {
  console.log("altitud", e.data);
  document.getElementById("altitud").innerHTML = e.data;
 }, false);
  source.addEventListener('hum', function(e) {
  console.log("hum", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
  source.addEventListener('luz', function(e) {
  console.log("luz", e.data);
  document.getElementById("luz").innerHTML = e.data;
 }, false);
  source.addEventListener('soil', function(e) {
  console.log("soil", e.data);
  document.getElementById("soil").innerHTML = e.data;
 }, false);
  source.addEventListener('co2', function(e) {
  console.log("co2", e.data);
  document.getElementById("co2").innerHTML = e.data;
 }, false);
}


</script>
</body>
</html>)rawliteral";

void setup(){    
  
  // initialize serial port
  Serial.begin(9600); 
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }  

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");

// Preheating CO2
 if (co2.isPreHeating()) {
    Serial.print("Preheating");
    while (co2.isPreHeating()) {
      Serial.print(".");
      delay(5000);
    }
    Serial.println();
  }

  iotclient.setup();
  iotclient.print_on_publish(true);
  timeClient.begin();
  
  // Inicia BME
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

 // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
  
}
void loop()
{
 
  
 if ((millis() - lastTime) > timerDelay) {
    ReadBME();
    leerSoil();
    leerluz();
    leerco2();
    
  //   f_temp, f_pressu, f_altid, f_hum, f_luz, f_piso, f_agua;
    // "{ \"C02\":%s,\"TI\":%s,\"TE\":%s,\"Al\":%s}"
    payload = "{\"time\":";
    payload += timeClient.getEpochTime();
    payload += ",\"Temp\":";
    payload += String(f_temp).c_str();
    payload += ",\"Hum\":";
    payload += String(f_hum).c_str();
    payload += ",\"Press\":";
    payload += String(f_pressu).c_str();
    payload += ",\"Luz\":";
    payload += String(f_luz).c_str();
    payload += ",\"soil\":";
    payload += String(f_piso).c_str();
    payload += ",\"CO2\":";
    payload += String(i_co2_pwm).c_str();    
    payload += "}";

    
    // sprintf(payload,dataBL,CO2,temp_int,temp_ext,Alco_Volt); 
    // const char dataBL[] = "C02:%c|TI:%c|TE:%c|Al:%c";  
    Serial.print("Topic:");
    Serial.println(payload);
    
    if (cont == 5)  // Publicamos a AWS cada 5 Min.
    {
      if (iotclient.publish(TOPIC_NAME, (char*) payload.c_str() ))
        {
            Serial.println("Successfully posted");
        }
        else
        {
            Serial.println(String("Failed to post to MQTT"));
        }
        cont = 0;
    } 
    cont = cont + 1;
    // delay(10000); // Cada 10 Seg.
        
// Send Events to the Web Server with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(f_temp).c_str(),"temp",millis());
    events.send(String(f_pressu).c_str(),"presion",millis());
    events.send(String(f_altid).c_str(),"altitud",millis());
    events.send(String(f_hum).c_str(),"hum",millis());
    events.send(String(f_luz).c_str(),"luz",millis());
    events.send(String(f_piso).c_str(),"soil",millis());
    events.send(String(i_co2_pwm).c_str(),"co2",millis());
    
    lastTime = millis();
  }


}
