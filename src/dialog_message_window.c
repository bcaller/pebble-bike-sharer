/**
 * Example implementation of the dialog message UI pattern.
 */
#include "dialog_message_window.h"

static Window *s_main_window;
static BitmapLayer *disco_img;
static GBitmap *bitmap_disco;
static TextLayer *disco_text;
static uint8_t msgid;

static void show_msgid() {
	switch(msgid) {
		case 5:
			text_layer_set_text(disco_text, "Connect phone");
			bitmap_disco = gbitmap_create_with_resource(RESOURCE_ID_DISCO);
			break;
		case 4:
			text_layer_set_text(disco_text, "No bikes nearby");
			bitmap_disco = gbitmap_create_with_resource(RESOURCE_ID_NO_BIKES);
			break;
		case 3:
			text_layer_set_text(disco_text, "No internet");
			bitmap_disco = gbitmap_create_with_resource(RESOURCE_ID_NO_INTERNET);
			break;
		default:
			text_layer_set_text(disco_text, "An error occurred");
	}
	bitmap_layer_set_bitmap(disco_img, bitmap_disco);
	bitmap_layer_set_compositing_mode(disco_img, GCompOpSet);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Load

	window_set_background_color(window, GColorChromeYellow);
	
	GRect bounds_disco_img = GRect((bounds.size.w/2 - 39), (bounds.size.h/2 - 40), 78, 67);
	disco_img = bitmap_layer_create(bounds_disco_img);
	bitmap_layer_set_alignment(disco_img, GAlignCenter);
	bitmap_layer_set_background_color(disco_img, GColorClear);
	
	GRect bounds_disco_text = GRect(bounds.size.w / 4, (bounds.size.h/2 + 28), bounds.size.w / 2, 60);
	disco_text = text_layer_create(bounds_disco_text);
	text_layer_set_font(disco_text, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(disco_text, GTextAlignmentCenter);
	text_layer_set_background_color(disco_text, GColorClear);
	text_layer_set_text_color(disco_text, GColorBlack);

	show_msgid();
	// Add Layers

	layer_add_child(window_layer, bitmap_layer_get_layer(disco_img));
	layer_add_child(window_layer, text_layer_get_layer(disco_text));

}

static void window_unload(Window *window) {
  bitmap_layer_destroy(disco_img);
  gbitmap_destroy(bitmap_disco);
  text_layer_destroy(disco_text);
  s_main_window = NULL;
}

void dialog_message_window_push(uint8_t msg_id) {
	if(msg_id % 2 == 0) {
		window_stack_pop_all(false);
	}
	if(!s_main_window) {
		msgid = msg_id;
		s_main_window = window_create();
		window_set_background_color(s_main_window, GColorJazzberryJam);
		window_set_window_handlers(s_main_window, (WindowHandlers) {
			.load = window_load,
			.unload = window_unload
		});
	}
  window_stack_push(s_main_window, false);
}

void dialog_message_window_pop() {
	if(msgid % 2 == 1)
		window_stack_remove(s_main_window, true);
}