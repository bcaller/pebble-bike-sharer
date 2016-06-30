#include <pebble.h>
#include "bikeorparking.h"
#include "list.h"
#include "disco.h"

static void deinit() {
    app_message_deregister_callbacks();
}

static void init() {
	register_app_message();
	bikeorparking_window_push(true);
	disco_window_listen();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}