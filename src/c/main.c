/*
    Bike Sharer for Pebble: shows the location of nearby rental bikes
    Copyright (C) 2016  Ben Caller
    Data provided by citybik.es

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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