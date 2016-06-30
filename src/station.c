#include <pebble.h>
#include "station.h"
#include "list.h"
#include "compass.h"

#define DANGER_NUM 4

static int station_pointer;
static char distance_str[8];
static char heading_str[3];
static char name_initial[8];
static char n_str[4];

static Window *main_window;

static CompassLayer *compass;

static int16_t old_n;
static Animation* refresh_animation;
static Animation* button_animation;

typedef struct NumbersAnimationData {
	int16_t from;
	int16_t to;
	Layer *txt;
} NumbersAnimationData;

// Declarations

static StatusBarLayer *status;
static TextLayer *distance;
static TextLayer *heading;
static TextLayer *name;
static Layer *hr;
static TextLayer *number;
static TextLayer *bikes_spaces;
static TextLayer *address;

//Other

static void hr_update_proc(Layer *layer, GContext *ctx) {
	const GRect bounds = layer_get_bounds(layer);
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);

}

Animation *create_animation_numbers(NumbersAnimationData *adata);
Animation *create_animation_compass(NumbersAnimationData *adata);

static void refresh(bool new_stations) {
	bool animate = !new_stations;
	
	//Choose station
	
	if(station_pointer == -1) {
		animate = false;
		// Auto start with nearest OK station
		station_pointer = 0;
		for(int i=0; i < N_STATIONS; i++) {
			if (stations[i].n > DANGER_NUM) {
				station_pointer = i;
				break;
			}
		}
	}
	
	if(new_stations) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "We have new stations");
		station_pointer = 0;
		// If we update, try to find the same station
		for(int i=0; i < N_STATIONS; i++) {
			if(strncmp(name_initial, stations[i].name, 7) == 0) {
				station_pointer = i;
				animate = true;
				break;
			}
		}
	}
		
	//Station chosen
	
	Station *station = &stations[station_pointer];
	
	if(station->distance > 1000) {
		int km = (station->distance + 50) / 1000;
		int kmdp1 = (station->distance + 50 - 1000 * km) / 100;
		snprintf(distance_str, 8, "%d.%d km", km, kmdp1);
	} else {
		snprintf(distance_str, 8, "%d m", station->distance);
	}
	text_layer_set_text(distance, distance_str);
	compass_layer_set_colors(compass, station->distance > 300 ? GColorImperialPurple : GColorBlue,
							 station->distance > 100 ? GColorBlack : GColorWhite, GColorRed);
	
	text_layer_set_text(name, station->name);
	snprintf(name_initial, 8, "%s", station->name);
	
	text_layer_set_text(address, station->address);
	
	int h = station->heading + 22;
	if(h < 45) snprintf(heading_str, 3, " N");
	else if(h <= 90) snprintf(heading_str, 3, "NE");
	else if(h <= 135) snprintf(heading_str, 3, " E");
	else if(h <= 180) snprintf(heading_str, 3, "SE");
	else if(h <= 180+45) snprintf(heading_str, 3, " S");
	else if(h <= 270) snprintf(heading_str, 3, "SW");
	else if(h <= 270+45) snprintf(heading_str, 3, " W");
	else if(h <= 360) snprintf(heading_str, 3, "NW");
	else snprintf(heading_str, 3, " N");
	text_layer_set_text(heading, heading_str);
		
	GColor new_bg_color;
	if(station->n <= 1) {
		new_bg_color = GColorRed;
	} else if(station->n <= DANGER_NUM) {
		new_bg_color = GColorOrange;
	} else {
		new_bg_color = GColorJaegerGreen;
	}
	window_set_background_color(main_window, new_bg_color);
	status_bar_layer_set_colors(status, new_bg_color, GColorWhite);
	
	text_layer_set_text(bikes_spaces, looking_for_parking ? (station->n == 1 ? "space" : "spaces") : (station->n == 1 ? "bike" : "bikes"));
	
	if(animate) {
		animation_unschedule(refresh_animation);
		
		NumbersAnimationData *nadata = (NumbersAnimationData*)malloc(sizeof(NumbersAnimationData));
		nadata->from = atoi(text_layer_get_text(number));
		nadata->to = station->n;
		nadata->txt = (Layer *)number;
		int16_t diff = nadata->to > nadata->from ? nadata->to - nadata->from : - nadata->to + nadata->from;
		
		NumbersAnimationData *cadata = (NumbersAnimationData*)malloc(sizeof(NumbersAnimationData));
		cadata->from = (int16_t)compass_layer_get_target(compass);
		cadata->to = station->heading;
		cadata->txt = (Layer*)compass;
		refresh_animation = animation_spawn_create(create_animation_numbers(nadata), create_animation_compass(cadata), NULL);
		animation_set_duration(refresh_animation, diff > 6 ? 1000 : 300);
		animation_schedule(refresh_animation);
	} else {
		snprintf(n_str, 4, PBL_IF_RECT_ELSE("%d", station->n < 10 ? "0%d" : "%d"), station->n);
		text_layer_set_text(number, n_str);
		
		compass_layer_set_target(compass, station->heading);
	}
}

//Clicks

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
	window_stack_pop_all(true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	station_pointer = (station_pointer + 1) % N_STATIONS;
	refresh(false);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	station_pointer = (station_pointer + N_STATIONS - 1) % N_STATIONS;
	refresh(false);
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
	window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void station_window_load(Window *window) {
	station_pointer = -1;
	
	main_window = window;
	
	Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Load

	window_set_background_color(window, GColorJaegerGreen);
	
#ifndef PBL_ROUND
	
	status = status_bar_layer_create();
	GRect bounds_status = GRect(0, 0, bounds.size.w, STATUS_BAR_LAYER_HEIGHT);
	status_bar_layer_set_colors(status, GColorJaegerGreen, GColorWhite);
	
	GRect bounds_distance = GRect(2, -2, bounds_status.size.w/2 - 5 - 2, 20);
	distance = text_layer_create(bounds_distance);
	text_layer_set_font(distance, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(distance, GTextAlignmentLeft);
	text_layer_set_background_color(distance, GColorClear);
	text_layer_set_text_color(distance, GColorWhite);
	
	GRect bounds_heading = GRect((bounds_status.size.w/2 + 5), -2, bounds_status.size.w - (bounds_status.size.w/2 + 5) - 2, 20);
	heading = text_layer_create(bounds_heading);
	text_layer_set_font(heading, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(heading, GTextAlignmentRight);
	text_layer_set_background_color(heading, GColorClear);
	text_layer_set_text_color(heading, GColorWhite);
	
	GRect bounds_content = GRect(3, (STATUS_BAR_LAYER_HEIGHT), bounds.size.w - 3 - 3, bounds.size.h - (STATUS_BAR_LAYER_HEIGHT));
	
	GRect bounds_name = GRect(bounds_content.origin.x, bounds_content.origin.y, bounds_content.size.w+20, 25);
	name = text_layer_create(bounds_name);
	text_layer_set_font(name, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(name, GTextAlignmentLeft);
	text_layer_set_overflow_mode(name, GTextOverflowModeFill);
	text_layer_set_background_color(name, GColorClear);
	text_layer_set_text_color(name, GColorWhite);
	
	GRect bounds_hr = GRect(bounds_content.origin.x, 26 + bounds_content.origin.y, bounds_content.size.w, 2);
	hr = layer_create(bounds_hr);
	layer_set_update_proc(hr, hr_update_proc);
	
	GRect bounds_number = GRect(5 + bounds_content.origin.x, 27 + bounds_content.origin.y, bounds_content.size.w - 10, 44);
	number = text_layer_create(bounds_number);
	text_layer_set_font(number, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
	text_layer_set_text_alignment(number, GTextAlignmentLeft);
	text_layer_set_background_color(number, GColorClear);
	text_layer_set_text_color(number, GColorWhite);
	
	GRect bounds_bikes_spaces = GRect(7 + bounds_content.origin.x, 70 + bounds_content.origin.y, 100, 16);
	bikes_spaces = text_layer_create(bounds_bikes_spaces);
	text_layer_set_font(bikes_spaces, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(bikes_spaces, GTextAlignmentLeft);
	text_layer_set_background_color(bikes_spaces, GColorClear);
	text_layer_set_text_color(bikes_spaces, GColorWhite);
	
	GRect bounds_compass = GRect(bounds_content.origin.x + 40, 28 + bounds_content.origin.y, bounds_content.size.w - 40, bounds_content.size.h - 28 - 25);
	compass = compass_layer_create(bounds_compass);
	compass_layer_set_colors(compass, GColorBlue, GColorBlack, GColorRed);
	
	GRect bounds_address = GRect(bounds_content.origin.x - 1, bounds_content.size.h - 25 + bounds_content.origin.y, bounds_content.size.w+20, 25);
	address = text_layer_create(bounds_address);
	text_layer_set_font(address, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(address, GTextAlignmentLeft);
	text_layer_set_overflow_mode(address, GTextOverflowModeFill);
	text_layer_set_background_color(address, GColorClear);
	text_layer_set_text_color(address, GColorWhite);

	// Add Layers

	layer_add_child(window_layer, status_bar_layer_get_layer(status));
	layer_add_child(status_bar_layer_get_layer(status), text_layer_get_layer(distance));
	layer_add_child(status_bar_layer_get_layer(status), text_layer_get_layer(heading));
	layer_add_child(window_layer, text_layer_get_layer(name));
	layer_add_child(window_layer, hr);
	layer_add_child(window_layer, compass_layer_get_layer(compass));
	layer_add_child(window_layer, text_layer_get_layer(number));
	layer_add_child(window_layer, text_layer_get_layer(bikes_spaces));
	layer_add_child(window_layer, text_layer_get_layer(address));
	
#else
	
	status = status_bar_layer_create();
	status_bar_layer_set_colors(status, GColorJaegerGreen, GColorWhite);
	
	GRect bounds_number = GRect(0, (STATUS_BAR_LAYER_HEIGHT-5), bounds.size.w, 39);
	number = text_layer_create(bounds_number);
	text_layer_set_font(number, fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
	text_layer_set_text_alignment(number, GTextAlignmentCenter);
	text_layer_set_background_color(number, GColorClear);
	text_layer_set_text_color(number, GColorWhite);
	text_layer_set_text(number, "12");
	
	GRect bounds_bikes_spaces = GRect((bounds.size.w/2 + 30), 40, bounds.size.w - (bounds.size.w/2 + 30) - 5, 16);
	bikes_spaces = text_layer_create(bounds_bikes_spaces);
	text_layer_set_font(bikes_spaces, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(bikes_spaces, GTextAlignmentLeft);
	text_layer_set_background_color(bikes_spaces, GColorClear);
	text_layer_set_text_color(bikes_spaces, GColorWhite);
	text_layer_set_text(bikes_spaces, "bikes");
	
	GRect bounds_name = GRect(5, (bounds.size.h/2 - 33), bounds.size.w - 5 - -20, 25);
	name = text_layer_create(bounds_name);
	text_layer_set_font(name, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(name, GTextAlignmentLeft);
	text_layer_set_overflow_mode(name, GTextOverflowModeFill);
	text_layer_set_background_color(name, GColorClear);
	text_layer_set_text_color(name, GColorWhite);
	text_layer_set_text(name, "Menilmontant");
	
	GRect bounds_hr = GRect(0, (bounds.size.h/2 - 7), bounds.size.w, 2);
	hr = layer_create(bounds_hr);
	layer_set_update_proc(hr, hr_update_proc);
	
	GRect bounds_address = GRect(3, (bounds.size.h/2 - 6), bounds.size.w - 3 - -20, 20);
	address = text_layer_create(bounds_address);
	text_layer_set_font(address, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(address, GTextAlignmentLeft);
	text_layer_set_overflow_mode(address, GTextOverflowModeFill);
	text_layer_set_background_color(address, GColorClear);
	text_layer_set_text_color(address, GColorWhite);
	text_layer_set_text(address, "25 Bd de Menilmontant");
	
	GRect bounds_compass = GRect(0, bounds.size.h/2 - 7, bounds.size.w, bounds.size.h/2 - -7);
	compass = compass_layer_create(bounds_compass);
	compass_layer_set_colors(compass, GColorBlue, GColorBlack, GColorRed);

	GRect bounds_distance = GRect(1, (bounds.size.h/2 + 20), bounds.size.w/2 - 35 - 1, 20);
	distance = text_layer_create(bounds_distance);
	text_layer_set_font(distance, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(distance, GTextAlignmentRight);
	text_layer_set_background_color(distance, GColorClear);
	text_layer_set_text_color(distance, GColorWhite);
	text_layer_set_text(distance, "250m");
	
	GRect bounds_heading = GRect((bounds.size.w/2 + 35), (bounds.size.h/2 + 20), bounds.size.w - (bounds.size.w/2 + 35) - 1, 20);
	heading = text_layer_create(bounds_heading);
	text_layer_set_font(heading, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(heading, GTextAlignmentLeft);
	text_layer_set_background_color(heading, GColorClear);
	text_layer_set_text_color(heading, GColorWhite);
	text_layer_set_text(heading, "NNE");
	
	// Add Layers

	layer_add_child(window_layer, status_bar_layer_get_layer(status));
	layer_add_child(window_layer, text_layer_get_layer(name));
	layer_add_child(window_layer, hr);
	layer_add_child(window_layer, compass_layer_get_layer(compass));
	layer_add_child(window_layer, text_layer_get_layer(distance));
	layer_add_child(window_layer, text_layer_get_layer(heading));
	layer_add_child(window_layer, text_layer_get_layer(number));
	layer_add_child(window_layer, text_layer_get_layer(address));
	layer_add_child(window_layer, text_layer_get_layer(bikes_spaces));
	
#endif

    // Click
    window_set_click_config_provider(window, click_config_provider);
	
	refresh(false);
	set_update_handler(refresh);
}


//Anim

static void set_animated_numbers(NumbersAnimationData *adata, int16_t n) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "set %d < %d < %d", adata->from, n, adata->to);
  	snprintf(n_str, 4, PBL_IF_RECT_ELSE("%d", n < 10 ? "0%d" : "%d"), (int)n);
	text_layer_set_text((TextLayer *)adata->txt, n_str);
}

static void set_animated_compass(NumbersAnimationData *adata, int16_t n) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "set %d < %d < %d", adata->from, n, adata->to);
	compass_layer_set_target((CompassLayer *)adata->txt, n);
}

static void teardown_animation(PropertyAnimation *property_animation) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "TEARDOWN");
	void *subject;
	if (property_animation_get_subject(property_animation, &subject)) {
		free(subject);
	}
}

static const PropertyAnimationImplementation s_animated_numbers_implementation = {
	.base = {
		.update = (AnimationUpdateImplementation) property_animation_update_int16,
		.teardown = (AnimationTeardownImplementation) teardown_animation,
	},
	.accessors = {
		.setter = { .int16 = (const Int16Setter) set_animated_numbers, },
	},
};

static const PropertyAnimationImplementation s_animated_compass_implementation = {
	.base = {
		.update = (AnimationUpdateImplementation) property_animation_update_int16,
		.teardown = (AnimationTeardownImplementation) teardown_animation,
	},
	.accessors = {
		.setter = { .int16 = (const Int16Setter) set_animated_compass, },
	},
};

Animation *create_animation_numbers(NumbersAnimationData *adata) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "create %d < < %d", adata->from, adata->to);
  PropertyAnimation *animation = property_animation_create(&s_animated_numbers_implementation, adata, NULL, NULL);
	property_animation_set_from_int16(animation, &adata->from);
	property_animation_set_to_int16(animation, &adata->to);
  return (Animation *) animation;
}

Animation *create_animation_compass(NumbersAnimationData *adata) {
	//APP_LOG(APP_LOG_LEVEL_INFO, "create %d < < %d", adata->from, adata->to);
  PropertyAnimation *animation = property_animation_create(&s_animated_compass_implementation, adata, NULL, NULL);
	int diff = adata->from - adata->to;
	if(diff > 180) {
		adata->from -= 360;
	} else if(diff < -180) {
		adata->from += 360;
	}
	property_animation_set_from_int16(animation, &adata->from);
	property_animation_set_to_int16(animation, &adata->to);
  return (Animation *) animation;
}

//Anim



static void station_window_unload(Window *window) {
	set_update_handler(NULL);
	status_bar_layer_destroy(status);
	text_layer_destroy(distance);
	text_layer_destroy(heading);
	text_layer_destroy(name);
	layer_destroy(hr);
	text_layer_destroy(number);
	text_layer_destroy(bikes_spaces);
	compass_layer_destroy(compass);
	text_layer_destroy(address);

}

Window *station_window_get_window(StationWindow *window) {
    return (Window *)window;
}

StationWindow *station_window_create() {
    Window *window = window_create();

    window_set_window_handlers(window, (WindowHandlers) {
            .load = station_window_load,
            .unload = station_window_unload
    });

    return (StationWindow *) window;
}

StationWindow *station_window_push(bool animated) {
    StationWindow *window = station_window_create();
    window_stack_push(station_window_get_window(window), animated);
    return (StationWindow *) window;
}

void station_window_destroy(StationWindow *window) {
    if(!window)return;
    window_destroy((Window *)window);
}