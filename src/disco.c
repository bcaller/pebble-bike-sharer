#include <pebble.h>
#include "disco.h"
#include "dialog_message_window.h"

static void app_connection_handler(bool connected) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Pebble app %sconnected", connected ? "" : "dis");
	if(!connected) {
		dialog_message_window_push(5);
	} else if(connected) {
		dialog_message_window_pop();
	}
}

static void kit_connection_handler(bool connected) {
  APP_LOG(APP_LOG_LEVEL_INFO, "PebbleKit %sconnected", connected ? "" : "dis");
	if(!connected) {
		dialog_message_window_push(5);
	} else if(connected) {
		dialog_message_window_pop();
	}
}

void disco_window_listen() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "checking connection to phone");
	kit_connection_handler(connection_service_peek_pebble_app_connection());
	connection_service_subscribe((ConnectionHandlers) {
	  .pebble_app_connection_handler = app_connection_handler,
	  .pebblekit_connection_handler = kit_connection_handler
	});
}