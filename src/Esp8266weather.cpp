// derived from https://github.com/wero1414/ESPWeatherStation
// GPL2.0 License applies to this code.

#define DHT_DEBUG  1
#define DEBUG_ESP_WIFI
#define DEBUG_ESP_PORT Serial

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "wifi_secrets.h" // defines IDs and PASSWDs
#include "wu_secrets.h" // these certs periodically expire, need a better solution
#include "phant_secrets.h"
#include "syslog.h"
#include "syslog_secrets.h"

#define DHTPIN          2   //Pin to attach the DHT - on D1 mini, what's labeled as D4 is GPIO2
#define DHTTYPE DHT22       //type of DTH  

// WiFi //
const char* ssid     = MYSSID;
const char* password = SSIDPASSWD;
const int sleepTimeS = 600; // in seconds; 18000 for Half hour, 300 for 5 minutes etc.
const char vfname[] =  __FILE__ ;
const char vtimestamp[] =  __DATE__ " " __TIME__;
const char versionstring[] = "20250601.0235.1";

///////////////Weather////////////////////////

char wu_ID [] = MYWUID;
char wu_PASSWORD [] = WUPASSWD;
char WU_cert_fingerprint[] = "12 DB BB 24 8E 0F 6F D4 63 EC 45 DD 5B ED 37 D7 6F B1 5F E5";

X509List wu_cert(cert_DigiCert_Global_Root_CA);
X509List phant_cert(cert_ISRG_Root_X1);

//////////////syslog//////////////////////////

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;

// Create a new syslog instance with LOG_KERN facility
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN);
int iteration = 1;

///// forward declarations /////
double dewPoint(double tempf, double humidity);
void sleepMode();

DHT dht(DHTPIN, DHTTYPE);

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

void setup()
{
  int wifiwaitcount = 0;
  int wifistatus = WL_IDLE_STATUS;
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  dht.begin();
  delay(1000);
  Serial.println();
  Serial.print("file: ");
  Serial.println(vfname);
  Serial.print("timestamp (local time): ");
  Serial.println(vtimestamp);
  Serial.println();

  // Connect D0 to RST to wake up
  pinMode(D0, WAKEUP_PULLUP);

  // most of the time we will have been reconnected by now, so check for connection before .begin()
  // this avoids a problem that crops up when calling .begin() while already connected
  if ( (wifistatus = WiFi.status()) == WL_CONNECTED) {
    Serial.print("Connected to ");   Serial.println(WiFi.SSID());
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("Connecting to ");  Serial.println(ssid);
    Serial.println(password);
    WiFi.begin(ssid, password);
    while (((wifistatus = WiFi.status()) == WL_IDLE_STATUS) && (++wifiwaitcount < 60)) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
  }

  Serial.print("wifi status= ");     Serial.println(wifistatus);
  Serial.println( "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
  syslog.log(LOG_INFO, "wifistatus");

  // BMP test code
  // Serial.println("Pressure Sensor Test"); Serial.println("");

  /* Initialise the sensor */
  if (!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
  }

  /* Display some basic information on this sensor */
  // displaySensorDetails();
}

void loop() {
  //Check battery // currently not using this code
  //int level = analogRead(A0);
  //level = map(level, 0, 1024, 0, 100);
  //if(level<50)
  //{
  // mandarNot(); //Send IFTT
  // Serial.println("Low battery");
  // delay(500);
  //}

  float baromin = 100;
  float tempf = 1000;

  sensors_event_t event;
  bmp.getEvent(&event);

  /* Display the results (barometric pressure is measure in hPa) */
  if (event.pressure)
  {
    /* Display atmospheric pressue (sensor gives it in hPa or millibars */
    float baromhPa = event.pressure;
    Serial.print("Pressure:    ");     Serial.print(baromhPa);   Serial.println(" hPa");
    baromin = baromhPa * 0.0295300;
    Serial.print("        :    ");     Serial.print(baromin);    Serial.println(" inHg");
    float barommmhg = baromhPa * 0.750062;
    Serial.print("        :    ");     Serial.print(barommmhg);  Serial.println(" mmHg");
    
    /* Calculating altitude with reasonable accuracy requires pressure    *
       sea level pressure for your position at the moment the data is
       converted, as well as the ambient temperature in degress
       celcius.  If you don't have these values, a 'generic' value of
       1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA
       in sensors.h), but this isn't ideal and will give variable
       results from one day to the next.
     *                                                                    *
       You can usually find the current SLP value by looking at weather
       websites or from environmental information centers near any major
       airport.
     *                                                                    *
       For example, for Paris, France you can check the current mean
       pressure and sea level at: http://bit.ly/16Au8ol                   */

    /* First we get the current temperature from the BMP085 */
    float temperature;
    bmp.getTemperature(&temperature);
    Serial.print("Temperature: ");    Serial.print(temperature);  Serial.println(" C");
    tempf =  (temperature * 9.0) / 5.0 + 32.0;
   
    Serial.print("           : ");    Serial.print(tempf);       Serial.println(" F");

    /* Then convert the atmospheric pressure, and SLP to altitude         */
    /* Update this next line with the current SLP for better results      */
//// Not using altitude
    //float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    // Serial.print("Altitude:    ");
    // Serial.print(bmp.pressureToAltitude(seaLevelPressure,
    //                                    event.pressure));
    // Serial.println(" m");
    // Serial.println("");
  }
  else
  {
    Serial.println("Sensor error");
  }
  delay(1000);

  //Get sensor data
  float tempc = dht.readTemperature();
  float temp2f =  (tempc * 9.0) / 5.0 + 32.0;
  float humidity = dht.readHumidity();
  float dewptf = dewPoint(tempf, humidity);

  //local sensor data report
  Serial.println("+++++++++++++++++++++++++");
  Serial.print("tempF=     ");  Serial.print(temp2f);    Serial.println(" *F");
  Serial.print("tempC=     ");  Serial.print(tempc);    Serial.println(" *C");
  Serial.print("dew point= ");  Serial.println(dewptf);
  Serial.print("humidity=  ");  Serial.println(humidity);
  Serial.println("vvvvvvvvvvvvvvvvvvvvvvvvvv");

  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  Serial.println(" UTC");
  syslog.log(LOG_INFO, asctime(&timeinfo));


  ///////////////////////////////////
  //Send data to Weather Underground
  ///////////////////////////////////
  Serial.print("sending data to ");
  Serial.println(wu_host);

   // Using HTTPS protocol
   WiFiClientSecure client;
   Serial.print("Connecting to ");
   Serial.print(wu_host);

   // Serial.printf("Using certificate: %s\n", cert_DigiCert_Global_Root_CA);
   client.setTrustAnchors(&wu_cert);

   if (!client.connect(wu_host, 443)) {
     Serial.println("Conection Fail");
     // return;
   }

//   // Using HTTP protocol
//   WiFiClient client;
//   if (!client.connect(wu_host, 80)) {
//     Serial.println("Conection Fail");
//     return;
//   }

  // see http://wiki.wunderground.com/index.php/PWS_-_Upload_Protocol for details

  String ReqData = "ID=";         ReqData += wu_ID;
        ReqData += "&PASSWORD=";  ReqData += wu_PASSWORD; 
        ReqData += "&dateutc="    "now";
        ReqData += "&tempf=";     ReqData += tempf;
        ReqData += "&temp2f=";    ReqData += temp2f;
        ReqData += "&dewptf=";    ReqData += dewptf;
        ReqData += "&humidity=";  ReqData += humidity;
        ReqData += "&baromin=";   ReqData += baromin;
        ReqData += "&softwaretype=" "ESP8266%20WeatherStation02%20version%20";
        ReqData += versionstring;
        ReqData += "&action="    "updateraw" ;
  Serial.println("ReqData= " + ReqData);

  String WebReq = "GET ";        WebReq += wu_WEBPAGE;  WebReq += "?";
  WebReq += ReqData;             WebReq += " HTTP/1.1\r\n";
  WebReq += "Host: ";             WebReq += wu_host;    WebReq += "\r\n";
  WebReq += "Connection: "        "close"        "\r\n";
  WebReq += "\r\n\r\n"; // end of headers

  Serial.println("WebReq= " + WebReq);

  client.print(WebReq);
  syslog.log(LOG_INFO, "WeatherUnderground request sent");

  Serial.println("-----Response-----");
  while (client.connected())
  {
    if (client.available())
    {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      syslog.log(LOG_INFO, line);
    }
  }
  Serial.println("----------");

  ///////////////////////////////////
  //Send data to phant
  ///////////////////////////////////
  Serial.print("sending data to ");
  Serial.println(phant_host);

   // Using HTTPS protocol
   
   Serial.print("Connecting to ");
   Serial.println(phant_host);

   // Serial.printf("Using certificate: %s\n", cert_ISRG_Root_X1);
   client.setTrustAnchors(&phant_cert);

   if (!client.connect(phant_host, 443)) {
     Serial.println("Conection Fail");
    return;
   }

  ReqData =  "tempf=";     ReqData += tempf;
  ReqData += "&dewpoint=";  ReqData += dewptf;
  ReqData += "&tempc=";     ReqData += tempc;
  ReqData += "&humidity=";  ReqData += humidity;
  ReqData += "&baromin=";   ReqData += baromin;
  ReqData += "&utcdate=";   ReqData += asctime(&timeinfo);
        ReqData += "\r\n";
  Serial.println("ReqData= " + ReqData);

  WebReq = "POST /";        WebReq += phant_WebPage; WebReq += " HTTP/1.1\r\n";
  WebReq += "Host: ";             WebReq += phant_host;    WebReq += "\r\n";
  WebReq += "Phant-Private-Key: "; WebReq += phant_PrivKey; WebReq += "\r\n";
  WebReq += "Connection: "        "close"        "\r\n";
  WebReq += "Content-Length: ";   WebReq += ReqData.length(); WebReq += "\r\n";
  WebReq += "Content-Type: application/x-www-form-urlencoded\r\n";
  WebReq += "\r\n"; // end of headers
  WebReq += ReqData; // POST data

  Serial.println("WebReq= " + WebReq);

  client.print(WebReq);
  syslog.log(LOG_INFO, "Phant request sent");

 
  Serial.println("-----Response-----");
  while (client.connected())
  {
    if (client.available())
    {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      syslog.log(LOG_INFO, line);
    }
  }

  delay(2500);
  sleepMode();
}


double dewPoint(double tempf, double humidity) //Calculate dew Point
{
  double A0 = 373.15 / (273.15 + tempf);
  double SUM = -7.90298 * (A0 - 1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1) ;
  SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM - 3) * humidity;
  double T = log(VP / 0.61078);
  return (241.88 * T) / (17.558 - T);
}

//void mandarNot(){ // not using this yet
//  WiFiClient client;
//  if (!client.connect(host, puertoHost)) //Check connection
//  {
//    Serial.println("Failed connection");
//    return;
//  }
//  client.print(conexionIF);//Send information
//  delay(10);
//  while(client.available())
//  {
//    String line = client.readStringUntil('\r');
//    Serial.println(line);
//  }
//}

void sleepMode() {
  Serial.print("Going into deep sleep now...");
  syslog.log(LOG_INFO, "Going into deep sleep now...");
  delay(1000); // give time to send last messages
  
  ESP.deepSleep(sleepTimeS * 1000000);
}

/**************************************************************************/
/*
    Displays some basic information on this sensor from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information)
*/
/**************************************************************************/
void displaySensorDetails(void)
{
  sensor_t sensor;
  bmp.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" hPa");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" hPa");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" hPa");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

