// derived from https://github.com/wero1414/ESPWeatherStation
// GPL2.0 License applies to this code.

#define DHT_DEBUG  1
#define DEBUG_ESP_WIFI
#define DEBUG_ESP_PORT Serial

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "secret.h" // defines IDs and PASSWDs


#define DHTPIN          2   //Pin to attach the DHT - on D1 mini, what's labeled as D4 is GPIO2
#define DHTTYPE DHT22       //type of DTH  

// WiFi //
const char* ssid     = MYSSID;
const char* password = SSIDPASSWD;
const int sleepTimeS = 600; // in seconds; 18000 for Half hour, 300 for 5 minutes etc.
const char vfname[] =  __FILE__ ;
const char vtimestamp[] =  __DATE__ " " __TIME__;
const char versionstring[] = "20180104.1630.1";

///////////////Weather////////////////////////
char wu_host [] = "weatherstation.wunderground.com";
char wu_WEBPAGE [] = "/weatherstation/updateweatherstation.php";
char wu_ID [] = MYWUID;
char wu_PASSWORD [] = WUPASSWD;
char WU_cert_fingerprint[] = "12 DB BB 24 8E 0F 6F D4 63 EC 45 DD 5B ED 37 D7 6F B1 5F E5";

///////////////Phant////////////////////////
//char host [] = "10.XXX.XXX.XXX";
//char WEBPAGE [] = "/weatherstation/updateweatherstation.php";
//char ID [] = MYWUID;
//char PASSWORD [] = WUPASSWD;
//char *WU_cert_fingerprint = "12 DB BB 24 8E 0F 6F D4 63 EC 45 DD 5B ED 37 D7 6F B1 5F E5";



/////////////IFTTT/////////////////////// not currently used
//const char* host = "maker.ifttt.com";//dont change
//const String IFTTT_Event = "YourEventName";
//const int puertoHost = 80;
//const String Maker_Key = "YourMakerKey";
//String conexionIF = "POST /trigger/"+IFTTT_Event+"/with/key/"+Maker_Key +" HTTP/1.1\r\n" +
//                  "Host: " + host + "\r\n" +
//                  "Content-Type: application/x-www-form-urlencoded\r\n\r\n";
//////////////////////////////////////////

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

  //Send data to Weather Underground
  Serial.print("sending data to ");
  Serial.println(wu_host);

   // Using HTTPS protocol
   WiFiClientSecure client;
   if (!client.connect(wu_host, 443)) {
     Serial.println("Conection Fail");
    return;
   }
   if (client.verify(WU_cert_fingerprint, wu_host)) {
     Serial.println("certificate matches");
   } else {
     Serial.println("certificate doesn't match");
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
  
  Serial.println("-----Response-----");
  while (client.connected())
  {
    if (client.available())
    {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }
  Serial.println("----------");
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

