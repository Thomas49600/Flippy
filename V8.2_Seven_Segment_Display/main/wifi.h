#ifndef WIFI_H
#define WIFI_H
#include <Arduino.h>  // Include Arduino functions like delay(), pinMode(), etc.

#include <WebSocketsServer.h> // must be included here as we use WStype_t
#include "counter.h" // must be included here are we use Counter type

// Function prototypes (public)
void setupWifi();
void setupCounterObjs();
void wifiChecks();
void handleNotFound();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);

extern Counter** counterObjs; // Pointer to a dynamic array of Counter objects
extern int* counterObjGroupIDs; // Pointer to a dynamic array of integers
extern int sizeOfObjectArray; // Current size of the arrays

#endif