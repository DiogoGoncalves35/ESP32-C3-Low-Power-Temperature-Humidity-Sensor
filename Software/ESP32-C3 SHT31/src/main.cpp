// Libraries
#include <Arduino.h>
#include <Wire.h>
#include "SHT31.h"
#include <SPI.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>


// Mac Address
uint8_t broadcastAddress[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};    // Hub MAC

// Deep Sleep variables
#define uS_TO_S_FACTOR 1000000ULL  // Conversion
#define TIME_TO_SLEEP  300         // Time to deep sleep

// ESP-NOW variable
esp_now_peer_info_t peerInfo;

// ESP-NOW data structure   
typedef struct struct_message 
{
  byte   ID = X;               // Board identifier
  float  Temperature;          // Temperature variable
  byte   Humidity;             // Humidity variable
  byte   Battery_Percentage;   // Battery percentage variable
} struct_message;

struct_message Data;


// Variables to store last measurement
RTC_DATA_ATTR double RTC_Temperature = 0;
RTC_DATA_ATTR byte RTC_Humidity = 0;


// Variable to verify if a message was send
bool Send = false;


// Function to communicate with SHT31 sensor
void Sensor()
{ 
  // Sensor variable
  SHT31 SHT;

  // Variable for time cicle with current millis
  unsigned long Wait_Sensor = millis();

  //Cycle that ends after 50 ms or in case there is connection with sensor
  // ^ - XOR
  while((!SHT.begin(0x44, 6, 7)) ^ (millis() >= Wait_Sensor + 50))
  {
    // Delay 
    delayMicroseconds(1);
  }
  
  // Set I2C speed to 400 KHz
  Wire.setClock(400000);

  // Read, if error, start deep sleep
  if(SHT.read())
  {
    // Measurement and conversion
    Data.Temperature  = roundf(SHT.getTemperature()*10)/10;     // Temperature with only one decimal place
    Data.Humidity     = round(SHT.getHumidity());               // Humidity as int value
  }
  else
  {
    // Start deep sleep
    esp_deep_sleep_start();
  }

  // Compare measurements
  if(Data.Temperature == RTC_Temperature && Data.Humidity == RTC_Humidity)
  {
    // Start deep sleep
    esp_deep_sleep_start();
  }
  else
  {
    // Store values 
    RTC_Temperature = Data.Temperature;
    RTC_Humidity    = Data.Humidity;
  }
}


// Function called automatically after message is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  // If message is sent with sucess, start deep sleep
  if(status == ESP_NOW_SEND_SUCCESS)
  {
    // Start deep sleep
    esp_deep_sleep_start();
  }
  else
  {
    // If message was sent two times, start deep sleep
    if (Send)
    {
      // Start deep sleep
      esp_deep_sleep_start();
    }

    // Send as true
    Send = true;

    // Send one last time
    esp_now_send(broadcastAddress,(uint8_t *) &Data, sizeof(Data)); 
  }
}


// Function to read battery voltage and convert to percentage
void Battery()
{
  // IO1 as OUTPUT
  pinMode(1, OUTPUT);

  // Turn on ADC_EN
  digitalWrite(1, HIGH);
  
  //IO3 as INPUT
  pinMode(3,INPUT);  

  // Variable to store 3 measurements
  uint32_t Vbatt = 0;

  // 3 measurements
  for(int i = 0; i < 3; i++) 
  {
    // Read
    // Attenuation 11db (Default), Voltage : 0,15V - 2,45V 
    Vbatt = Vbatt + analogReadMilliVolts(3);   
  }

  // Turn off ADC_EN
  digitalWrite(1, HIGH);
  
  // Attenuation 1/2, conversion from mV to V, average readings and round to 1 decimal place
  float Vbattf = roundf((2 * (Vbatt / 3) / 1000.0)*10)/10;          

  // Conversion to percentage, max voltage 4.2V(100%), 3V(0%)
  Data.Battery_Percentage = ((Vbattf - 3.0) * (100 - 0) / (4.2 - 3) + 0);
}


// ESP-NOW function 
void ESP_NOW()
{
  // Set frequency
  setCpuFrequencyMhz(160);

  // WiFi as station mode
  WiFi.mode(WIFI_STA);

  // Start protocol, if error, start deep sleep
  if (esp_now_init() != ESP_OK)   
  {
    // Start deep sleep
    esp_deep_sleep_start();
  }
  
  // Register callback function
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  
  // Add peer, if error, start deep sleep  
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    // Start deep sleep
    esp_deep_sleep_start();
  }

  // Send data
  esp_now_send(broadcastAddress,(uint8_t *) &Data, sizeof(Data)); 
}


// Setup
void setup()  
{ 
  // Set frequency
  setCpuFrequencyMhz(40);

  // Ativate deep sleep timer
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // SHT31 function
  Sensor();

  // Battery function
  Battery();

  // ESP-NOW function
  ESP_NOW();

  // Delete Loop
  vTaskDelete(NULL);   
}


// Loop
void loop() {}