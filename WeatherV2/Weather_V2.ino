
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <JSON_Decoder.h>
#include <OpenWeather.h>
#include <WiFiManager.h>   
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>

#define PIN            4
#define NUMPIXELS     21
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
// Just using this library for unix time conversion
#include <Time.h>
//setup tm1637 display
#include <TM1637Display.h>
//math functions
#include <stdio.h>      /* printf */
#include <math.h>       /* floor */

//set pins for display
#define CLK 2//pins definitions for TM1637 and can be changed to other ports       
#define DIO 13
TM1637Display display(CLK, DIO);

// =====================================================
// ========= User configured stuff starts here =========
// Further configuration settings can be found in the
// OpenWeather library "User_Setup.h" file

#define TIME_OFFSET 1UL * 3600UL // UTC + 0 hour

// OpenWeather API Details, replace x's with your API key
String api_key = "4ded61dd4bb2cb1f71be516fb8128e80"; // Obtain this from your OpenWeather account

// Set both your longitude and latitude to at least 4 decimal places
String latitude =  "55.9533"; // 90.0000 to -90.0000 negative for Southern hemisphere
String longitude = "3.1883"; // 180.000 to -180.000 negative for West

String units = "metric";  // or "imperial"
String language = "en";   // See notes tab

// =========  User configured stuff ends here  =========
// =====================================================

OW_Weather ow; // Weather forecast library instance

void setup() { 
  Serial.begin(9600); // Fast to stop it holding up the stream

  pixels.begin();
  //set display brightness of tm1637 display
  display.setBrightness(0x0f);
  //start 7 seg display
  display.setSegments(data);
  WiFiManager MyWifiManager;
  MyWifiManager.autoConnect("Weather Frame");
   Serial.println("WiFi connected");

}

void loop() {

  printCurrentWeather();

  // We can make 1000 requests a day
  delay(5 * 60 * 1000); // Every 5 minutes = 288 requests per day
}

/***************************************************************************************
**                          Send weather info to serial port
***************************************************************************************/
void printCurrentWeather()
{
  // Create the structures that hold the retrieved weather
  OW_current *current = new OW_current;
  OW_hourly *hourly = new OW_hourly;
  OW_daily  *daily = new OW_daily;

  Serial.print("\nRequesting weather information from OpenWeather... ");

  ow.getForecast(current, hourly, daily, api_key, latitude, longitude, units, language);

  Serial.println("Weather from Open Weather\n");

  // Position as reported by Open Weather
  Serial.print("Latitude            : "); Serial.println(ow.lat);
  Serial.print("Longitude           : "); Serial.println(ow.lon);
  // We can use the timezone to set the offset eventually...
  Serial.print("Timezone            : "); Serial.println(ow.timezone);
  Serial.println();

  if (current)
  {
    Serial.println("############### Current weather ###############\n");
    Serial.print("dt (time)        : "); Serial.print(strTime(current->dt));
    Serial.print("sunrise          : "); Serial.print(strTime(current->sunrise));
    Serial.print("sunset           : "); Serial.print(strTime(current->sunset));
    Serial.print("temp             : "); Serial.println(current->temp);
    Serial.print("feels_like       : "); Serial.println(current->feels_like);
    Serial.print("pressure         : "); Serial.println(current->pressure);
    Serial.print("humidity         : "); Serial.println(current->humidity);
    Serial.print("dew_point        : "); Serial.println(current->dew_point);
    Serial.print("uvi              : "); Serial.println(current->uvi);
    Serial.print("clouds           : "); Serial.println(current->clouds);
    Serial.print("visibility       : "); Serial.println(current->visibility);
    Serial.print("wind_speed       : "); Serial.println(current->wind_speed);
    Serial.print("wind_gust        : "); Serial.println(current->wind_gust);
    Serial.print("wind_deg         : "); Serial.println(current->wind_deg);
    Serial.print("rain             : "); Serial.println(current->rain);
    Serial.print("snow             : "); Serial.println(current->snow);
    Serial.println();
    Serial.print("id               : "); Serial.println(current->id);
    Serial.print("main             : "); Serial.println(current->main);
    Serial.print("description      : "); Serial.println(current->description);
    Serial.print("icon             : "); Serial.println(current->icon);

    Serial.println();
  }


  // Delete to free up space and prevent fragmentation as strings change in length
  delete current;
  delete hourly;
  delete daily;
}

/***************************************************************************************
**                          Convert unix time to a time string
***************************************************************************************/
String strTime(time_t unixTime)
{
  unixTime += TIME_OFFSET;
  return ctime(&unixTime);
}
