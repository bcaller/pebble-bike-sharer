#pragma once
#include "consts.h"

typedef struct {
	uint8_t n;
	uint8_t other;
	int heading;
	int distance;
	char name[64];
	char address[64];
} Station;

typedef void(* UpdateHandler)(bool new_stations);

extern Station stations[N_STATIONS];
extern bool looking_for_parking;

void swap_looking();
void register_app_message();
void request_update();
void set_update_handler(UpdateHandler);