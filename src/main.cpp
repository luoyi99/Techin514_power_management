#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

const char* ssid = "UW MPSK";
const char* password = "]]:c4q-XQ4"; // Replace with your network password
#define DATABASE_URL "https://techin514-lab5-powermanagement-default-rtdb.firebaseio.com/" // Replace with your database URL
#define API_KEY "AIzaSyDZyj1C2Vhc-ct1WMQtKpl8t5Ac8rv6EAY" // Replace with your API key
#define STAGE_INTERVAL 10000 // 10 seconds each stage
#define MAX_WIFI_RETRIES 5 // Maximum number of WiFi connection retries

int uploadInterval = 1000; // 1 seconds each upload

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// HC-SR04 Pins
const int trigPin = 2;
const int echoPin = 3;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Function prototypes
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Run ultrasonic sensor only for 10 seconds
  Serial.println("Measuring distance for 10 seconds...");
  unsigned long startTime = millis();
  int invalidDistanceCnt = 0;
  bool validDistance = true;

  while (millis() - startTime < STAGE_INTERVAL) // 10 seconds
  {
    float currentDistance = measureDistance();

    if (currentDistance > 50)
    {
      invalidDistanceCnt++;
    }

    delay(500); // Delay between measurements
  }
  
  // Most measurements > 50, go to deep sleep for 60 seconds
  if (invalidDistanceCnt >= 10)
  {
    Serial.println("All measured distance > 50, going to deep sleep for 60 seconds...");
    esp_sleep_enable_timer_wakeup(60000 * 1000); // 60 seconds in microseconds
    esp_deep_sleep_start();
  }
  else 
  {
  // Turn on WiFi and turn on Firebase and send data every 2 second with distance measurements for 10 seconds
  Serial.println("Turning on WiFi and measuring for 5 seconds...");
  connectToWiFi();
  initFirebase();
  startTime = millis();
  while (millis() - startTime < 5000)
  {
    float currentDistance = measureDistance();
    sendDataToFirebase(currentDistance);
    delay(500); // Delay between measurements
  }


  // Go to deep sleep for 60 seconds
  Serial.println("Going to deep sleep for 60 seconds...");
  WiFi.disconnect();
  esp_sleep_enable_timer_wakeup(60000 * 1000); // in microseconds
  esp_deep_sleep_start();
  }
}

void loop(){
  // This is not used
}

float measureDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi()
{
  // Print the device's MAC address.
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > MAX_WIFI_RETRIES){
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase()
{
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void sendDataToFirebase(float distance){
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > uploadInterval || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.pushFloat(&fbdo, "test/distance", distance)){
      Serial.println("PASSED");
      Serial.print("PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("TYPE: " );
      Serial.println(fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
    }
    count++;
  }
}