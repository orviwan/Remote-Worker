#pragma once
#include <pebble.h>

#define SETTINGS_KEY 1337

enum {
  BACKGROUND_KEY = 0x0,
  LOCALTIME_KEY = 0x1,
  REMOTETIME_KEY = 0x2,
  DAY_KEY = 0x3,
  DATE_KEY = 0x4,
	OFFSET_KEY = 0x5,
  LABEL_KEY = 0x6
};

typedef struct AppSettings {
  GColor Background;
	GColor LocalTime;
	GColor RemoteTime;
	GColor Day;
	GColor Date;
	int Offset;
  char Label[6];
} __attribute__((__packed__)) AppSettings;


void settings_load();
void settings_save();
void settings_process_tuple(Tuple *new_tuple);
void inbox_received_handler(DictionaryIterator *iter, void *context);
void force_tick();
void handle_tick(struct tm *tick_time, TimeUnits units_changed);
void main_window_load(Window *window);
void main_window_unload(Window *window);
void handle_init(void);
void handle_deinit(void);
