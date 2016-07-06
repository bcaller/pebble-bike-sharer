#include <pebble.h>
#include "list.h"
#include "dialog_message_window.h"

#define PRINT_FORMAT "%s"

Station stations[N_STATIONS];
bool looking_for_parking;
static bool phone_has_spoken; // Have we ever received good data?
static UpdateHandler update_handler = NULL;

void set_update_handler(UpdateHandler uh) {
	update_handler = uh;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message received!");
	Tuple *t_bikes_slots = dict_find(iterator, (looking_for_parking ? MESSAGE_KEY_SLOTS : MESSAGE_KEY_BIKES));
	Tuple *t_err = dict_find(iterator, MESSAGE_KEY_ERR);
	if(t_bikes_slots) {
		dialog_message_window_pop();
		phone_has_spoken = true;
		
		bool new_stations = false;
		uint8_t *n = t_bikes_slots->value->data;
		uint8_t *other = dict_find(iterator, (looking_for_parking ? MESSAGE_KEY_BIKES : MESSAGE_KEY_SLOTS))->value->data;
		
		for(int i=0; i<N_STATIONS; i++) {
			Tuple *t_name = dict_find(iterator, MESSAGE_KEY_NAMES + i);
			if(t_name) {
				snprintf(stations[i].name, sizeof(stations[i].name), PRINT_FORMAT, t_name->value->cstring);
				Tuple *t_address = dict_find(iterator, MESSAGE_KEY_ADDRESSES + i);
				snprintf(stations[i].address, sizeof(stations[i].address), PRINT_FORMAT, t_address ? t_address->value->cstring : "");
				new_stations = true;
			}
			stations[i].n = n[i];
			stations[i].other = other[i];
			stations[i].heading = DEG_TO_TRIGANGLE(dict_find(iterator, MESSAGE_KEY_HEADINGS + i)->value->int32);
			stations[i].distance = dict_find(iterator, MESSAGE_KEY_DISTANCES + i)->value->int32;
			
			if(i==0)
				APP_LOG(APP_LOG_LEVEL_DEBUG, "%s with %d! (%d)", stations[i].name, stations[i].n, stations[i].other);
		}
		
		if(update_handler) {
			update_handler(new_stations);
		}
	} else if(t_err) {
		if(!phone_has_spoken) {
			dialog_message_window_push(t_err->value->uint8);
		}
	}
}

void swap_looking() {
	if(phone_has_spoken) {
		for(int i=0; i<N_STATIONS; i++) {
			uint8_t old_n = stations[i].n;
			stations[i].n = stations[i].other;
			stations[i].other = old_n;
		}
		looking_for_parking = !looking_for_parking;
	}
}

void request_update() {
	if(phone_has_spoken) {
		DictionaryIterator *iter;
		if(phone_has_spoken && app_message_outbox_begin(&iter) == APP_MSG_OK) {
			dict_write_int(iter, 0, 0, sizeof(int), true);
			app_message_outbox_send();
		}
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void register_app_message() {
	// Register callbacks
	phone_has_spoken = false;
	looking_for_parking = false;
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    app_message_open(app_message_inbox_size_maximum(), APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}
