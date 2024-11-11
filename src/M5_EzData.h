/***************************************************************************
Title: M5_EzData
date: 2021/10/15
***************************************************************************/

#ifndef M5_EzData_h
#define M5_EzData_h

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi connection
int setupWifi(const char *ssid, const char *password);

// Store data to specified topic
int setData(const char *token, const char *topic, int val);

// Get data from specified topic 
int getData(const char *token, const char *topic, int &result);

// Remove specified data
int removeData(const char *token, const char *topic);

#endif
