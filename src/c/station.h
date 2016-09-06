#pragma once
#include "pebble.h"

typedef struct StationWindow StationWindow;
Window *station_window_get_window(StationWindow *window);
StationWindow *station_window_create();
StationWindow *station_window_push(bool animated);
void station_window_destroy(StationWindow *window);