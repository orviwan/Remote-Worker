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
  LABEL_KEY = 0x6,
  DISPLAYMODE_KEY = 0x7,
  DISPLAYDATA_KEY = 0x8,
  DATA_KEY = 0x9
};

enum {
  DISPLAYMODE_NONE = 0,
  DISPLAYMODE_AUTO = 1,
  DISPLAYMODE_MANUAL = 2
};

typedef struct AppSettings {
  GColor Background;
	GColor LocalTime;
	GColor RemoteTime;
	GColor Day;
	GColor Date;
  GColor Data;
	int32_t Offset;
  char Label[6];
  uint8_t DisplayMode;
} __attribute__((__packed__)) AppSettings;


void settings_load();
void settings_save();
void settings_process_tuple(Tuple *new_tuple);
void inbox_received_handler(DictionaryIterator *iter, void *context);
void force_tick();
void handle_tick(struct tm *tick_time, TimeUnits units_changed);
void duration_to_time(int duration_s, int *hours, int *minutes);
void update_display();
void switch_state();
void tap_handler(AccelAxisType axis, int32_t direction);
void health_handler(HealthEventType event, void *context);
void main_window_load(Window *window);
void main_window_unload(Window *window);
void handle_init(void);
void handle_deinit(void);
