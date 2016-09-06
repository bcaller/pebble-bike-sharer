#include <pebble.h>
#include "compass.h"

#define NUM_POINTS_COMPASS_PATH 4
typedef struct {
	int target;
    GColor fg;
	GColor outline;
	GColor uncalib;
	GPathInfo path_info;
	GPath *arrow;
	GPoint points[NUM_POINTS_COMPASS_PATH];
} CompassData;

static Layer* compass_singleton;

CompassHeadingData heading;

CompassData* compass_layer_get_data(CompassLayer* layer) {
    return layer_get_data((Layer*)layer);
}

static int point_compass_towards_trigangle(int heading_to_target_trig, int compass_heading) {
	return (TRIG_MAX_ANGLE + heading_to_target_trig + compass_heading) % TRIG_MAX_ANGLE;
}

static void draw(Layer *layer, GContext *ctx) {
    CompassData* data = compass_layer_get_data((CompassLayer*)layer);
    GRect bounds = layer_get_bounds((Layer*)layer);
	gpath_rotate_to(data->arrow, point_compass_towards_trigangle(data->target, heading.true_heading));
	GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    gpath_move_to(data->arrow, center);
	graphics_context_set_fill_color(ctx, data->fg);
	gpath_draw_filled(ctx, data->arrow);
	graphics_context_set_stroke_color(ctx, heading.compass_status == CompassStatusCalibrated ? data->outline : data->uncalib);
	graphics_context_set_stroke_width(ctx, 3);
	gpath_draw_outline(ctx, data->arrow);
}

static void compass_heading_handler(CompassHeadingData heading_data) {
	// Is the compass calibrated?
	heading = heading_data;
	switch(heading_data.compass_status) {
		case CompassStatusDataInvalid:
		case CompassStatusUnavailable:
			APP_LOG(APP_LOG_LEVEL_INFO, "Not yet calibrated.");
			break;
		case CompassStatusCalibrating:
			APP_LOG(APP_LOG_LEVEL_INFO, "Calibration in progress. Heading is %ld",
				TRIGANGLE_TO_DEG(heading_data.magnetic_heading));
			layer_mark_dirty(compass_singleton);
			break;
		case CompassStatusCalibrated:
			/*APP_LOG(APP_LOG_LEVEL_INFO, "Calibrated! Heading is %ld",
				TRIGANGLE_TO_DEG(heading_data.magnetic_heading));*/
			layer_mark_dirty(compass_singleton);
			break;
	}
}

CompassLayer *compass_layer_create(GRect frame) {
    Layer *layer = layer_create_with_data(frame, sizeof(CompassData));
    CompassData* data = layer_get_data(layer);
	
	#define MIN(x,y) (x>y?y:x)
	int16_t dim = MIN(frame.size.w, frame.size.h) / 2 - 1;
	data->points[0] = (GPoint){-dim/2, dim/3+dim/4};
	data->points[1] = (GPoint){0, -dim+dim/4};
	data->points[2] = (GPoint){dim/2, dim/3+dim/4};
	data->points[3] = (GPoint){0, -dim/5+dim/4};
	data->path_info = (GPathInfo){
		NUM_POINTS_COMPASS_PATH,
		data->points
	};
	data->arrow = gpath_create(&data->path_info);
	
    layer_set_update_proc(layer, draw);
	
	compass_service_subscribe(compass_heading_handler);
	compass_service_set_heading_filter(DEG_TO_TRIGANGLE(3));
    
	compass_singleton = layer;
	
    return (CompassLayer*) layer;
}

void compass_layer_destroy(CompassLayer *layer) {
	compass_service_unsubscribe();
	CompassData* data = compass_layer_get_data((CompassLayer*)layer);
	gpath_destroy(data->arrow);
    layer_destroy((Layer*)layer);
}

Layer* compass_layer_get_layer(CompassLayer* layer) {
    return (Layer*)layer;
}

void compass_layer_set_target(CompassLayer * layer, int trigangle) {
    CompassData* data = compass_layer_get_data((CompassLayer*)layer);
    data->target = trigangle;
	layer_mark_dirty((Layer*)layer);
}

void compass_layer_set_target_deg(CompassLayer * layer, int angle) {
    CompassData* data = compass_layer_get_data((CompassLayer*)layer);
    data->target = DEG_TO_TRIGANGLE(angle);
	layer_mark_dirty((Layer*)layer);
}

int compass_layer_get_target(CompassLayer * layer) {
    CompassData* data = compass_layer_get_data((CompassLayer*)layer);
    return data->target;
}

void compass_layer_set_colors(CompassLayer * layer, GColor foreground, GColor outline, GColor uncalib) {
    CompassData* data = compass_layer_get_data((CompassLayer*)layer);
    data->fg = foreground;
	data->outline = outline;
	data->uncalib = uncalib;
	layer_mark_dirty((Layer*)layer);
}