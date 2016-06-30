#include <pebble.h>
#include "bikeorparking.h"
#include "station.h"
#include "list.h"

// Declarations

static BitmapLayer *bike_img;
static GBitmap *bitmap_bike;
static BitmapLayer *parking_img;
static GBitmap *bitmap_park;
static TextLayer *updating;

static bool ready = false;
static Window *main_window;
static uint8_t pre_click = 0;

//Other


//Clicks

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(!ready) {
		if(pre_click == 0 || pre_click == BUTTON_ID_DOWN)
			pre_click = BUTTON_ID_DOWN;
		else
			pre_click = -1;
		return;
	}
	if(!looking_for_parking) {
		swap_looking();
	}
	station_window_push(true);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(!ready) {
		if(pre_click == 0 || pre_click == BUTTON_ID_UP)
			pre_click = BUTTON_ID_UP;
		else
			pre_click = -1;
		return;
	}
	if(looking_for_parking) {
		swap_looking();
	}
	station_window_push(true);
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}

static void loaded(bool new_stations) {
	layer_set_hidden(text_layer_get_layer(updating), true);
	if(!ready) { 
		ready = true;
		window_set_background_color(main_window, GColorJaegerGreen);
		bitmap_layer_set_background_color(bike_img, GColorWhite);
		bitmap_layer_set_background_color(parking_img, GColorBlack);
		vibes_short_pulse();
		if(pre_click == BUTTON_ID_UP) {
			up_click_handler(NULL, NULL);
		} else if(pre_click == BUTTON_ID_DOWN) {
			down_click_handler(NULL, NULL);
		}
	}
}

static void bikeorparking_window_load(Window *window) {
	ready = false;
	main_window = window;
	Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Load

	window_set_background_color(window, GColorJazzberryJam);
	
	GRect bounds_bike_img = GRect(0, 0, bounds.size.w, bounds.size.h/2 - 5);
	bike_img = bitmap_layer_create(bounds_bike_img);
	bitmap_layer_set_alignment(bike_img, GAlignBottom);
	bitmap_layer_set_background_color(bike_img, GColorJazzberryJam);
	bitmap_bike = gbitmap_create_with_resource(RESOURCE_ID_BIKE);
	bitmap_layer_set_bitmap(bike_img, bitmap_bike);
	bitmap_layer_set_compositing_mode(bike_img, GCompOpSet);
	
	GRect bounds_parking_img = GRect(0, bounds.size.h/2 + 5, bounds.size.w, bounds.size.h/2 - 5);
	parking_img = bitmap_layer_create(bounds_parking_img);
	bitmap_layer_set_alignment(parking_img, GAlignTop);
	bitmap_layer_set_background_color(parking_img, GColorJazzberryJam);
	bitmap_park = gbitmap_create_with_resource(RESOURCE_ID_PARK);
	bitmap_layer_set_bitmap(parking_img, bitmap_park);
	bitmap_layer_set_compositing_mode(parking_img, GCompOpSet);
	
	GRect bounds_updating = GRect(0, (bounds.size.h/2 - 12), bounds.size.w, 23);
	updating = text_layer_create(bounds_updating);
	text_layer_set_font(updating, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(updating, GTextAlignmentCenter);
	text_layer_set_background_color(updating, GColorBlack);
	text_layer_set_text_color(updating, GColorWhite);
	int msg_choice = rand() % 4;
	if(msg_choice == 0)
		text_layer_set_text(updating, "Pumping tyres...");
	else if(msg_choice == 1)
		text_layer_set_text(updating, "Running red light...");
	else if(msg_choice == 2)
		text_layer_set_text(updating, "Oiling chain...");
	else
		text_layer_set_text(updating, "Connecting...");

	// Add Layers

	layer_add_child(window_layer, bitmap_layer_get_layer(bike_img));
	layer_add_child(window_layer, bitmap_layer_get_layer(parking_img));
	layer_add_child(window_layer, text_layer_get_layer(updating));

    // Click
    window_set_click_config_provider(window, click_config_provider);
	
	set_update_handler(loaded);
}

static void bikeorparking_window_unload(Window *window) {

	bitmap_layer_destroy(bike_img);
	gbitmap_destroy(bitmap_bike);
	bitmap_layer_destroy(parking_img);
	gbitmap_destroy(bitmap_park);
	text_layer_destroy(updating);

}

Window *bikeorparking_window_get_window(BikeOrParkingWindow *window) {
    return (Window *)window;
}

BikeOrParkingWindow *bikeorparking_window_create() {
    Window *window = window_create();

    window_set_window_handlers(window, (WindowHandlers) {
            .load = bikeorparking_window_load,
            .unload = bikeorparking_window_unload
    });

    return (BikeOrParkingWindow *) window;
}

BikeOrParkingWindow *bikeorparking_window_push(bool animated) {
    BikeOrParkingWindow *window = bikeorparking_window_create();
    window_stack_push(bikeorparking_window_get_window(window), animated);
    return (BikeOrParkingWindow *) window;
}

void bikeorparking_window_destroy(BikeOrParkingWindow *window) {
    if(!window)return;
    window_destroy((Window *)window);
}