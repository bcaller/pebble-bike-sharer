#pragma once

#include <pebble.h>

#define DIALOG_MESSAGE_WINDOW_MARGIN   10

void dialog_message_window_push(uint8_t msg_id);
void dialog_message_window_pop();