#pragma once

typedef struct CompassLayer CompassLayer;

CompassLayer *compass_layer_create(GRect frame);
void compass_layer_destroy(CompassLayer *layer);
Layer* compass_layer_get_layer(CompassLayer* layer);

int compass_layer_get_target(CompassLayer * layer);
void compass_layer_set_target(CompassLayer * layer, int trigangle);
void compass_layer_set_target_deg(CompassLayer * layer, int angle);
void compass_layer_set_colors(CompassLayer * layer, GColor foreground, GColor outline, GColor uncalib);