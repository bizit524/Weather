
#include <Arduino.h>

//setup tm1637 display
#include <TM1637Display.h>
//math functions
#include <stdio.h>      /* printf */
#include <math.h>       /* floor */
//neopixel library
#include "Adafruit_NeoPixel.h"
//wifi library
#include <ESP8266WiFi.h>



//set pins for display
#define CLK 2//pins definitions for TM1637 and can be changed to other ports       
#define DIO 13
TM1637Display display(CLK, DIO);

//setup neopixels
// Configuration you can optionally change (but probably want to keep the same):
#define PIXEL_PIN       4                      // Pin connected to the NeoPixel data input.
#define PIXEL_COUNT     21                      // Number of NeoPixels.
#define PIXEL_TYPE      NEO_GRBW + NEO_KHZ800   // Type of the NeoPixels (see strandtest example).
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE); // create NeoPixels object

// setup arrays for SnowandRain function
float redStates[PIXEL_COUNT];
float blueStates[PIXEL_COUNT];
float greenStates[PIXEL_COUNT];
float whiteStates[PIXEL_COUNT];

//setup for snowandrain function
const char *srCheck;
#include <ArduinoJson.h>

//wifi name and pass
const char SSID[]     = "-";//
const char PASSWORD[] = "---------";//

// Use your own API key by signing up for a free developer account.
// http://www.wunderground.com/weather/api/
#define WU_API_KEY "-----------"

// Specify your favorite location one of these ways.
#define WU_LOCATION "-------------"


// 30 minutes between update checks. The free developer account has a limit
// on the  number of calls so don't go wild.
//#define DELAY_NORMAL    (20*60*1000)
//debug time 
#define DELAY_NORMAL    (10*1*1000)
// 60 minute delay between updates after an error
#define DELAY_ERROR     (10*10*1000)

//#define WUNDERGROUND "api.wunderground.com"
//debug json server
#define WUNDERGROUND "my-json-server.typicode.com"

// HTTP request
const char WUNDERGROUND_REQ[] =
    //"GET /api/" WU_API_KEY "/conditions/q/" WU_LOCATION ".json HTTP/1.1\r\n"
    //debug jsonserver
    "GET -----------------"
    "User-Agent: ESP8266/0.1\r\n"
    "Accept: */*\r\n"
    "Host: "WUNDERGROUND"\r\n"
    "Connection: close\r\n"
    "\r\n";

bool showWeather(char *json);

void SnowandRain(const char *WeatherType, const char *Speed);

uint8_t DecimalNumber(int number);

void lightPixels(uint32_t color);

void handleCondition(String weather1);

void setup()
{
  
  // Initialize NeoPixels and turn them off.
  pixels.begin();
  lightPixels(pixels.Color(0, 0, 0, 0));

  //set display brightness of tm1637 display
  display.setBrightness(0x0f);
  //setup digit arrary 
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  //start 7 seg display
  display.setSegments(data);
  //start serial
  Serial.begin(115200);
  //setup for SnowandRain function
  for(uint16_t l = 0; l < PIXEL_COUNT; l++) 
  {
    redStates[l] = 0;
    greenStates[l] = 0;
    blueStates[l] = 0;
    whiteStates[l] = 0;
  }


  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
}

static char respBuf[4096];

void loop()
{
  // TODO check for disconnect from AP
  
  // Open socket to WU server port 80
  Serial.print(F("Connecting to "));
  Serial.println(WUNDERGROUND);

  // Use WiFiClient class to create TCP connections
  WiFiClient httpclient;
  const int httpPort = 80;
  if (!httpclient.connect(WUNDERGROUND, httpPort)) 
  {
    Serial.println(F("connection failed"));
    delay(DELAY_ERROR);
    return;
  }

  // This will send the http request to the server
  Serial.print(WUNDERGROUND_REQ);
  httpclient.print(WUNDERGROUND_REQ);
  httpclient.flush();

  // Collect http response headers and content from Weather Underground
  // HTTP headers are discarded.
  // The content is formatted in JSON and is left in respBuf.
  int respLen = 0;
  bool skip_headers = true;
  while (httpclient.connected() || httpclient.available()) 
  {
    if (skip_headers) 
  {
      String aLine = httpclient.readStringUntil('\n');
      //Serial.println(aLine);
      // Blank line denotes end of headers
      if (aLine.length() <= 1) 
    {
        skip_headers = false;
      }
    }
    else 
  {
      int bytesIn;
      bytesIn = httpclient.read((uint8_t *)&respBuf[respLen], sizeof(respBuf) - respLen);
      Serial.print(F("bytesIn ")); Serial.println(bytesIn);
      if (bytesIn > 0) 
    {
        respLen += bytesIn;
        if (respLen > sizeof(respBuf)) respLen = sizeof(respBuf);
      }
      else if (bytesIn < 0) 
    {
        Serial.print(F("read error "));
        Serial.println(bytesIn);
      }
    }
    delay(1);
  }
  httpclient.stop();

  if (respLen >= sizeof(respBuf)) 
  {
    Serial.print(F("respBuf overflow "));
    Serial.println(respLen);
    delay(DELAY_ERROR);
    return;
  }
  // Terminate the C string
  respBuf[respLen++] = '\0';
  Serial.print(F("respLen "));
  Serial.println(respLen);
  //Serial.println(respBuf);
 
  if (showWeather(respBuf)) 
  {
    delay(DELAY_NORMAL);
  }
  else 
  {
    delay(DELAY_ERROR);
  }

 
}

bool showWeather(char *json)
{
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  StaticJsonBuffer<3*1024> jsonBuffer;

  // Skip characters until first '{' found
  // Ignore chunked length, if present
  char *jsonstart = strchr(json, '{');
  //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
  if (jsonstart == NULL) 
  {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;

  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) 
  {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }
  
  // Extract weather info from parsed JSON
  JsonObject& current = root["current_observation"];
  const float temp_f = current["temp_f"];
  Serial.print(temp_f, 1); Serial.print(F(" F, "));
  const float temp_c = current["temp_c"];
  Serial.print(temp_c, 1); Serial.print(F(" C, "));
  String temp_c_string = current["temp_c"];
  const char *humi = current[F("relative_humidity")];
  Serial.print(humi);   Serial.println(F(" RH"));
  
  const char *weather = current["weather"];
  srCheck = current["weather"];
  //test diff weather patterns with const char *weather = "Sunny"; or w/e one you want
  Serial.println(weather);

  const char *pressure_mb = current["pressure_mb"];
  Serial.println(pressure_mb);
  const char *observation_time = current["observation_time_rfc822"];
  Serial.println(observation_time);

  // Extract local timezone fields
  const char *local_tz_short = current["local_tz_short"];
  Serial.println(local_tz_short);
  const char *local_tz_long = current["local_tz_long"];
  Serial.println(local_tz_long);
  const char *local_tz_offset = current["local_tz_offset"];
  Serial.println(local_tz_offset);
  
  int temp_string_length = temp_c_string.length();
     
  //checks to see what type of temperature it is 
  if (temp_string_length == 3)
  {
     Serial.println("its single digits positve"); 
     float temp_c_single_p = temp_c;
     int temp_c_single_p_add = temp_c_single_p *10;
     int temp_c_single_p_2nd_digit = temp_c_single_p_add - (floor(temp_c_single_p) *10 ); 
     int temp_decimal = floor(temp_c_single_p); 
     //segments left to right    
     data[2] = B00000000; 
     data[1] = B00000000;// blank the digit
     data[0] = DecimalNumber(temp_decimal); //gets number with dec point in raw form
     data[5] = display.encodeDigit(temp_c_single_p_2nd_digit);               
     data[4] = B00111001;// C letter
     data[3] = B01100011; //degree sign
     display.setSegments(data);
  }
  else if (temp_string_length == 4) 
  {
    if (temp_c_string.startsWith("-"))
    {
     Serial.println("its single digit negative");
     float temp_c_single_n = temp_c;
     int temp_c_single_n_1st_digit = temp_c;
     int temp_c_single_n_add = temp_c_single_n *10;
     int temp_c_single_n_2nd_digit = temp_c_single_n_add - (ceil(temp_c_single_n) *10 );
     int temp_decimal = ceil(temp_c_single_n_1st_digit*-1);    
     data[2] = B00000000; 
     data[1] = B01000000 ; // neg sign 
     data[0] = DecimalNumber(temp_decimal);//grabs the number in raw form with decimal 
     data[5] = display.encodeDigit(temp_c_single_n_2nd_digit*-1);
     data[4] = B00111001;  
     data[3] = B01100011;//degree sign 

     Serial.println(temp_c_single_n);
     Serial.println(temp_c_single_n_1st_digit);
     Serial.println(temp_c_single_n_2nd_digit);
     Serial.println(floor(temp_c_single_n_1st_digit*-1));
     Serial.println(DecimalNumber(temp_decimal));
     display.setSegments(data);
    }
    else
    {
     Serial.println("yay its double digit positive"); 
     float temp_c_double_p  = temp_c * 10; 
     int temp_c_double_p_final = floor(temp_c_double_p/100); 
     int temp_c_double_p_final_2nd_digit = floor((temp_c_double_p -(temp_c_double_p_final * 100))/10); 
     int temp_c_double_p_final_3rd_digit = (temp_c_double_p -(floor(temp_c)*10));
     int temp_decimal = floor(temp_c_double_p_final_2nd_digit); 
     data[2] = B00000000; 
     data[1] = display.encodeDigit(temp_c_double_p_final);
     data[0] = DecimalNumber(temp_decimal);  
     data[5] = display.encodeDigit(temp_c_double_p_final_3rd_digit);
     data[4] = B00111001;        
     data[3] = B01100011; 

     Serial.println(temp_c_double_p);
     Serial.println(temp_c_double_p_final);
     Serial.println(temp_c_double_p_final_2nd_digit);
     display.setSegments(data);
    }
  }  
  else if(temp_string_length == 5) 
  {
     Serial.println("its double digits negative");
     float temp_c_double_n = temp_c * 10;
     int temp_c_double_n_final = ceil(temp_c_double_n/100);
     int temp_c_double_n_final_2nd_digit = ceil((temp_c_double_n -(temp_c_double_n_final * 100))/10); 
     int temp_c_double_n_final_3rd_digit = (temp_c_double_n -(ceil(temp_c)*10));
     int temp_decimal = ceil(temp_c_double_n_final_2nd_digit*-1);   
     data[2] = B01000000 ; //neg sign
     data[1] = display.encodeDigit(temp_c_double_n_final*-1);
     data[0] = DecimalNumber(temp_decimal);
     data[5] = display.encodeDigit(temp_c_double_n_final_3rd_digit *-1); 
     data[4] = B00111001;        
     data[3] = B01100011; 
       
     Serial.println(temp_c_double_n);
     Serial.println(temp_c_double_n_final);
     Serial.println(temp_c_double_n_final_2nd_digit);
     display.setSegments(data);
  }   
  //starts weather of leds 
  handleCondition(weather);
  return true;
}
//function to create a manual decimal layout for numbers 0-9 example 1 would be "1."
uint8_t layout[10] = 
{
  B10111111,
  B10000110,
  B11011011,
  B11001111,
  B11100110,
  B11101101,
  B11111101,
  B10000111,
  B11111111,
  B11101111,
  };
  
uint8_t DecimalNumber (int number)
{
  if(number >= 0 && number <= 8) return layout[number];
  // otherwise return layout[9]
  return layout[9];
}

//divdies pixels up into sections of the triangles then passes the RGBW vaules and sets them 
void PixelSet(int section, int G, int R, int B, int W)
{
  //calculate section based on number of sections 
  int split_sections =   (PIXEL_COUNT/3)-1;
  //Serial.println (section);
  if (section == 1)
  {
   int i = 0; 
    while (i <= split_sections) 
    {
    pixels.setPixelColor( i, pixels.Color(G,R,B,W));
    //Serial.println (i);
    i++;
    
    }

  }
  
  if (section == 2)
  {
    int i = split_sections + 1; 
    while (i <= (((split_sections + 1)*2)-1) ) 
    {
    pixels.setPixelColor(i, pixels.Color(G,R,B,W));
    i++;
    //Serial.println (i);
    }
  
  }
  
  if (section == 3)
  {
    int i = ((split_sections + 1)*2); 
    while (i <= (((split_sections + 1)*3)-1)) 
    {
    pixels.setPixelColor(i, pixels.Color(G,R,B,W));
    i++;
    //Serial.println (i);
    }
  }    
  
}


// Function to set all the NeoPixels to the specified color.
void lightPixels(uint32_t color) 
{
  for (int i=0; i<PIXEL_COUNT; ++i) 
  {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}

//parses json data and lights pixels based on weather condition
void handleCondition(String weather1) 
{

  lightPixels(pixels.Color(0, 0, 0, 0)); // reset all pixels to off


  String forecast = weather1;
  //parses json string data from the weather condition
  String rain = String("Rain");
  String overcast = String("Overcast");
  //String lightrain = String("Light Rain");
  String rainshower = String ("Rain Shower");
  String snow = String("Snow");
  String cloudy = String("Mostly Cloudy");
  String mostlycloudy = String("Mostly Cloudy");
  String partlycloudy = String("Partly Cloudy");
  String clearsky = String("Clear");
  String sunny = String("Sunny");
  String rainandsnow = String("Rain and Snow");
  String snowshower = String("Snow Shower");
 
  // These if statements compare the incoming weather variable to the stored conditions, and control the NeoPixels accordingly.

  
  // if there's rain in the forecast, tell the the first four pixels to be blue and the middle four pixels to be white (but don't draw them yet)
  if (forecast.equalsIgnoreCase(rain))
  {
    Serial.println("precipitation in the forecast today");
    const char *fast = "fast";
    const char *rain = "Rain";
    SnowandRain(rain, fast);
    /*pixels.setPixelColor(0, pixels.Color(0, 30, 200, 20));
    pixels.setPixelColor(1, pixels.Color(0, 30, 200, 20));
    pixels.setPixelColor(2, pixels.Color(0, 30, 200, 20));
    pixels.setPixelColor(3, pixels.Color(0, 30, 200, 20));
    pixels.setPixelColor(4, pixels.Color(0, 30, 200, 20));
    pixels.setPixelColor(5, pixels.Color(0, 30, 200, 20));
    pixels.setPixelColor(6, pixels.Color(0, 30, 200, 20));


    pixels.setPixelColor(7, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(8, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(9, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(10, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(11, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(12, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(13, pixels.Color(0, 0, 0, 255));*/
    //pixels.show();
    }
  
  // if there's snow in the forecast, tell the the first four pixels to be whiteish blue and the middle four pixels to be white (but don't draw them yet)
  if (forecast.equalsIgnoreCase(snow) || forecast.equalsIgnoreCase(rainandsnow) || forecast.equalsIgnoreCase(snowshower))
  {
    Serial.println("snow in the forecast today");
    const char *fast = "fast";
    const char *snow = "Snow";
    SnowandRain(snow, fast);
   /* pixels.setPixelColor(0, pixels.Color(0, 30, 175, 100));
    pixels.setPixelColor(1, pixels.Color(0, 30, 175, 100));
    pixels.setPixelColor(2, pixels.Color(0, 30, 175, 100));
    pixels.setPixelColor(3, pixels.Color(0, 30, 175, 100));
    pixels.setPixelColor(4, pixels.Color(0, 30, 175, 100));
    pixels.setPixelColor(5, pixels.Color(0, 30, 175, 100));
    pixels.setPixelColor(6, pixels.Color(0, 30, 175, 100));

    pixels.setPixelColor(7, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(8, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(9, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(10, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(11, pixels.Color(0, 0, 0, 255)); 
    pixels.setPixelColor(12, pixels.Color(0, 0, 0, 255));
    pixels.setPixelColor(13, pixels.Color(0, 0, 0, 255));*/  
    pixels.show();         
  }
  
  // if there's sun in the forecast, tell the last four pixels to be yellow (but don't draw them yet)
  if (forecast.equalsIgnoreCase(clearsky) || forecast.equalsIgnoreCase(sunny))
  {
    Serial.println("some kind of sun in the forecast today");
    PixelSet(2,255,128,0,0); // section, green,red,blue,white
    PixelSet(3,255,128,0,0);
    pixels.show();// light up the pixels    
  }
  // if there's sun in the forecast, tell the last four pixels to be yellow (but don't draw them yet)
  if (forecast.equalsIgnoreCase(overcast))
  {
    Serial.println("overcast no sun today ");
    PixelSet(1,0,0,0,255); // section, green,red,blue,white
    PixelSet(2,0,0,0,255);
    PixelSet(3,0,0,0,255);
    pixels.show();// light up the pixels    
  }
   // if its partly cloudy has 2 yellow and 1 white (but don't draw them yet)
  if (forecast.equalsIgnoreCase(partlycloudy))
  {    
    Serial.println("partly cloudy  forecast today");
    PixelSet(2,255,128,0,0);
    PixelSet(3,255,128,0,0);
    PixelSet(1,0,0,0,255);
    pixels.show(); // light up the pixels        
   }
   // if its mostly cloudy lights up 1 sun and 2 white (but don't draw them yet)
  if (forecast.equalsIgnoreCase(mostlycloudy))
  {    
    Serial.println("mostly cloudy forecast today");
    PixelSet(3,255,128,0,0);
    PixelSet(2,0,0,0,255);
    PixelSet(1,0,0,0,255);
    pixels.show(); // light up the pixels        
   }
   
}

void SnowandRain(const char *WeatherType, const char *Speed)
{
  
  Serial.print("its ");
  Serial.print(srCheck);
  Serial.print(" still, ");
  Serial.print(WeatherType);
    float fadeRate = .98;
    if (Speed == "fast")
    {
       fadeRate = 0.93;
    }
    if (Speed == "slow")
    {
       fadeRate = 0.97;
    }
   while (strcmp(srCheck,WeatherType)==0)
   { 
    
    //debug what is being passed 
    //Serial.println(WeatherType);
    //Serial.println(Speed);

    if (random(PIXEL_COUNT) == 1) {
        uint16_t i = random(PIXEL_COUNT);
        if (redStates[i] < 1 && greenStates[i] < 1 && blueStates[i] < 1 && whiteStates[i] < 1) 
        {
          redStates[i] = 0;
          greenStates[i] = 0;
          //changes to blue if rain, changes to white if snow
          if (WeatherType == "Snow")
          {
          blueStates[i] = 0;
          whiteStates[i] = random(256);
          }
          if (WeatherType == "Rain")
          {
          blueStates[i] = random(256);
          whiteStates[i] = 0;
          }
          
        }
      }
      
      for(uint16_t l = 0; l < PIXEL_COUNT; l++)
    {
        if (redStates[l] > 1 || greenStates[l] > 1 || blueStates[l] > 1 || whiteStates[l] > 1) 
      {
          pixels.setPixelColor(l, redStates[l], greenStates[l], blueStates[l], whiteStates[l]);
          
          if (redStates[l] > 1) 
      {
            redStates[l] = redStates[l] * fadeRate;
          } 
      else 
      {
            redStates[l] = 0;
          }
          if (greenStates[l] > 1) 
      {
            greenStates[l] = greenStates[l] * fadeRate;
          }
      else 
      {
            greenStates[l] = 0;
          }      
          if (blueStates[l] > 1) 
      {
            blueStates[l] = blueStates[l] * fadeRate;
          } 
      else 
      {
            blueStates[l] = 0;
          }
  
          if (whiteStates[l] > 1) 
      {
            whiteStates[l] = whiteStates[l] * fadeRate;
          }
      else 
      {
            whiteStates[l] = 0;
          }
          
        } 
      else 
      {
          pixels.setPixelColor(l, 0, 0, 0);
        }
      }
      pixels.show(); // light up the pixels
        if (Speed == "fast")
        {
          delay(6);
        }
        if (Speed == "slow")
        {
          delay(15);
        }
    
  } 
}
