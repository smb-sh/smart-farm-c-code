#include <DHT.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>


#define DHTTYPE DHT11   // DHT 11

#define dht_dpin D0
#define sm_vcc_pin D1
#define uv_vcc_pin D2
#define rain_pin D3
#define waterpump D4

DHT dht(dht_dpin, DHTTYPE); 

float uv_indexes[60];
float uv_samples[10];

float index_sum = 0;
float sample_sum = 0;

int counter = 0;
int sampler = 0;


const char *ssid =  "ssid";     // Enter your WiFi Name
const char *pass =  "password"; // Enter your WiFi Password
WiFiClient client;

#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "test" // Your Adafruit IO Username
#define MQTT_PASS "aio_asdffdsdfsdfsfdsadfsadS5" // Adafruit IO AIO key


int status = WL_IDLE_STATUS;



unsigned long lastConnectionTime = 10 * 60 * 1000;     // last time you connected to the server, in milliseconds
const unsigned long postInterval = 10 * 60 * 1000;  // posting interval of 10 minutes  (10L * 1000L; 10 seconds delay for testing)


const unsigned long Interval = 13000;
unsigned long previousTime = 0;

//Set up the feed you're publishing to

Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);
Adafruit_MQTT_Publish Moisture = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/Moisture"); // Moisture is the feed name where you will publish your data
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/Temperature");
Adafruit_MQTT_Publish Humidity = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/Humidity");
Adafruit_MQTT_Publish UV = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/UV");
Adafruit_MQTT_Publish Rain = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/Rain");


//Set up the feed you're subscribing to

Adafruit_MQTT_Subscribe Pump = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/Pump");

void setup()
{

  dht.begin();
  Serial.begin(9600);
  delay(700);
  

  
  mqtt.subscribe(&Pump);

  pinMode(waterpump, OUTPUT);
  pinMode(sm_vcc_pin, OUTPUT);
  pinMode(uv_vcc_pin, OUTPUT);
  pinMode(rain_pin, INPUT);
  

  digitalWrite(waterpump, LOW); // keep motor off initally
  

  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");              // print ... till not connected
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void loop()
{
  Serial.println("Starting...");
  unsigned long currentTime = millis();

  MQTT_connect();
  Serial.println("MQTT Connected");

  //moisturePercentage = ( 100.00 - ( (analogRead(moisturePin) / 1023.00) * 100.00 ) );
  Serial.println("Reading data");
  ///////////////////////////////////////////////////////////
  
  float uv = 0.00;
  float soil_moisture = 0.00;

  digitalWrite(sm_vcc_pin, HIGH);
  delay(500);
  soil_moisture = analogRead(0);
  digitalWrite(sm_vcc_pin, LOW);

  digitalWrite(uv_vcc_pin, HIGH);
  delay(500);
  uv = analogRead(0);
  digitalWrite(uv_vcc_pin, LOW);
  
  if(uv <= 227){
    uv = uv / 227;
    }
  else{
    uv = (uv / 100) - 1;
    }

  if(soil_moisture < 300){    //Caliberation
    soil_moisture = 300;
    }
    
  soil_moisture = map(soil_moisture, 1024, 300, 0, 90);   //Caliberation
  
  ////////////////////////////////////////////////////////////////////
  if (soil_moisture < 70) {
    digitalWrite(waterpump, HIGH);         // tun on motor
  }
  if (soil_moisture > 70) {
    digitalWrite(waterpump, LOW);          // turn off mottor
  }

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if(isnan(temperature)){
    temperature = 0.00;
    }
  if(isnan(humidity)){
    humidity = 0.00;
    }

  float rain = digitalRead(rain_pin);

  

  Serial.print("temperature: ");
  Serial.println(temperature);
  Serial.print("humidity: ");
  Serial.println(humidity);
  Serial.print("soil moisture: ");
  Serial.print(soil_moisture);
  Serial.println(" %");
  Serial.print("uv: ");
  Serial.println(uv);
  Serial.print("rain: ");
  Serial.println(rain);
  
  if (currentTime - previousTime >= Interval) {
    if (! Moisture.publish(soil_moisture)) //This condition is used to publish the Variable (moisturePercentage) on adafruit IO. Change thevariable according to yours.
    {}
    if (! Temperature.publish(temperature))
    {}
    if (! Humidity.publish(humidity))
    {
      //delay(30000);
    }
    if (! UV.publish(uv))
    {}
    if (! Rain.publish(rain))
    {}
    // if (! WeatherData.publish(icon))
    // {}

    previousTime = currentTime;
    Serial.println("data published");
  }

  Adafruit_MQTT_Subscribe * subscription;
  while ((subscription = mqtt.readSubscription(5000))) //Dont use this one until you are conrolling something or getting data from Adafruit IO.
  {


    if (subscription == &Pump)
    {
      //Print the new value to the serial monitor
      Serial.println((char*) Pump.lastread);
      if (!strcmp((char*) Pump.lastread, "OFF"))
      {
        digitalWrite(waterpump, HIGH);
      }
      if (!strcmp((char*) Pump.lastread, "ON"))
      {
        digitalWrite(waterpump, LOW);
      }
    }

  } // end while

  delay(12000);
  // client.publish(WeatherData, icon)
}

void MQTT_connect()
{
  
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected())
  {
    return;
  }
  
  mqtt.connect();

} // end func

