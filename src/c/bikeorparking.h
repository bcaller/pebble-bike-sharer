#pragma once
#include "pebble.h"

typedef struct BikeOrParkingWindow BikeOrParkingWindow;
Window *bikeorparking_window_get_window(BikeOrParkingWindow *window);
BikeOrParkingWindow *bikeorparking_window_create();
BikeOrParkingWindow *bikeorparking_window_push(bool animated);
void bikeorparking_window_destroy(BikeOrParkingWindow *window);