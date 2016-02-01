#include <pebble.h>
#include "main.h"

static Window *s_window;
static Layer *s_window_layer;
TextLayer *local_time_layer, *remote_time_layer, *day_layer, *date_layer, *data_layer;
static uint8_t s_state = 0;
static uint8_t s_max_state = 6;
static uint8_t s_timer = 10;

AppSettings settings;
static char s_time_buffer[] = "00:00";
static char s_time_buffer2[] = "00:00";
static char s_time_buffer3[] = "XXXXX: 00:00";
static char s_data_buffer[20];

static bool s_battery_charging = false;
static uint8_t s_battery_percent = 0;

static HealthValue s_sleep, s_deep_sleep, s_steps, s_active, s_distance;

void settings_load() {
	settings.Background = GColorRed;
	settings.LocalTime = GColorWhite;
	settings.RemoteTime = GColorLightGray;
	settings.Day = GColorWhite;
	settings.Date = GColorWhite;
	settings.Offset = 0;
	settings.DisplayMode = DISPLAYMODE_NONE;

  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

void settings_save() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

void settings_process_tuple(Tuple *new_tuple) {
  switch (new_tuple->key) {
    case BACKGROUND_KEY:
      settings.Background.argb = GColorFromHEX(new_tuple->value->int32).argb;
			window_set_background_color(s_window, settings.Background);
      break;
		case LOCALTIME_KEY:
			settings.LocalTime.argb = GColorFromHEX(new_tuple->value->int32).argb;
			text_layer_set_text_color(local_time_layer, settings.LocalTime);
			break;
		case REMOTETIME_KEY:
			settings.RemoteTime.argb = GColorFromHEX(new_tuple->value->int32).argb;
			text_layer_set_text_color(remote_time_layer, settings.RemoteTime);
			break;
		case DAY_KEY:
			settings.Day.argb = GColorFromHEX(new_tuple->value->int32).argb;
			text_layer_set_text_color(day_layer, settings.Day);
			break;
		case DATE_KEY:
			settings.Date.argb = GColorFromHEX(new_tuple->value->int32).argb;
			text_layer_set_text_color(date_layer, settings.Date);
			break;
		case OFFSET_KEY:
			settings.Offset = new_tuple->value->int32;
			layer_set_hidden(text_layer_get_layer(remote_time_layer), false);
			force_tick();
			break;
		case LABEL_KEY:
			snprintf(settings.Label, sizeof(settings.Label), new_tuple->value->cstring);
			force_tick();
			break;
		case DISPLAYMODE_KEY:
			settings.DisplayMode = new_tuple->value->int32;
			force_tick();
			break;
  }
}

void inbox_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *t = dict_read_first(iter);
  while(t != NULL) {
    if(t) {
      settings_process_tuple(t);
    }
		t = dict_read_next(iter);
  }
	settings_save();
}

void battery_update(BatteryChargeState charge_state) {
  s_battery_percent = charge_state.charge_percent;
  s_battery_charging = charge_state.is_charging;
  if(s_battery_percent==0) {
    s_battery_percent=1;
  }
	s_state = 5;
	update_display();
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
		static char s_day_buffer[20];
		strftime(s_day_buffer, sizeof(s_day_buffer), "%a %d", tick_time);
    text_layer_set_text(day_layer, s_day_buffer);

    static char s_date_buffer[20];
		strftime(s_date_buffer, sizeof(s_date_buffer), "%b %Y", tick_time);
    text_layer_set_text(date_layer, s_date_buffer);
  }
  if (units_changed & MINUTE_UNIT) {
    strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(local_time_layer, s_time_buffer);

		if(settings.Label[0] != '\0') {

			time_t now_seconds = mktime(tick_time);

			//APP_LOG(APP_LOG_LEVEL_DEBUG, "offset seconds: %ld", settings.Offset);
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "now seconds: %d", (int)now_seconds);

			now_seconds += settings.Offset;
			struct tm *tm_remote = gmtime(&now_seconds);

	    strftime(s_time_buffer2, sizeof(s_time_buffer2), clock_is_24h_style() ? "%H:%M" : "%I:%M", tm_remote);
			snprintf(s_time_buffer3, sizeof(s_time_buffer3), "%s: %s", settings.Label, s_time_buffer2);
	    text_layer_set_text(remote_time_layer, s_time_buffer3);
		}
		else {
			text_layer_set_text(remote_time_layer, NULL);
		}

  }
	s_timer--;
	if (units_changed & SECOND_UNIT) {
		if(s_timer==0 && DISPLAYMODE_AUTO) {
			switch_state();
			s_timer = 10;
		}
	}
}

void force_tick() {
	time_t now = time(NULL);
	struct tm *tick_time = localtime(&now);
	handle_tick(tick_time, DAY_UNIT|MINUTE_UNIT|SECOND_UNIT);
}

void duration_to_time(int duration_s, int *hours, int *minutes) {
  *hours = (duration_s / 3600) ?: 0;
  *minutes = ((duration_s % 3600) / 60) ?: 0;
}

void number_to_fraction(int numerator, int* whole_part, int *decimal_part) {
  *whole_part = numerator / 1000;
  *decimal_part = (numerator - ((numerator / 1000) * 1000)) / 10;
}

void update_display() {
	if(DISPLAYMODE_NONE) {
		return;
	}
	int hours = 0, minutes = 0;
	int who = 0, dec = 0;
	switch(s_state) {
			case 0:
				//steps
				snprintf(s_data_buffer, sizeof(s_data_buffer), "step %li", s_steps);
				break;
			case 1:
				//s_distance
	      number_to_fraction(s_distance, &who, &dec);
				snprintf(s_data_buffer, sizeof(s_data_buffer), "dist %d.%dkm", who, dec);
				break;
			case 2:
				//active
				duration_to_time(s_active, &hours, &minutes);
				if(hours>0) {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "act. %dH %dM", hours, minutes);
				}
				else {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "active %dM", minutes);
				}
				break;
			case 3:
				//sleep
				duration_to_time(s_sleep, &hours, &minutes);
				if(hours>0) {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "zzz %dH %dM", hours, minutes);
				}
				else {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "zzz %dM", minutes);
				}
				break;
			case 4:
				//deep
				duration_to_time(s_deep_sleep, &hours, &minutes);
				if(hours>0) {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "deep %dH %dM", hours, minutes);
				}
				else {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "deep %dM", minutes);
				}
				break;

			case 5:
				//battery
				if(s_battery_charging) {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "{+} %d%%", s_battery_percent);
				}
				else {
					snprintf(s_data_buffer, sizeof(s_data_buffer), "{=} %d%%", s_battery_percent);
				}
				break;
	}
	text_layer_set_text(data_layer, s_data_buffer);
}

void switch_state() {
	s_timer = 10;
	s_state++;
	if(s_state>=s_max_state) {
		s_state = 0;
	}
	update_display();
}

void tap_handler(AccelAxisType axis, int32_t direction) {
	switch (axis) {
		case ACCEL_AXIS_Z:
		case ACCEL_AXIS_X:
			break;
	  case ACCEL_AXIS_Y:
			switch_state();
	    break;
  }
}

void health_handler(HealthEventType event, void *context) {
	if(DISPLAYMODE_NONE) {
		return;
	}
  if (event == HealthEventMovementUpdate) {
    s_steps = health_service_sum_today(HealthMetricStepCount);
		s_distance = health_service_sum_today(HealthMetricWalkedDistanceMeters);
		s_active = health_service_sum_today(HealthMetricActiveSeconds);
  } else if (event == HealthEventSleepUpdate) {
    s_sleep = health_service_sum_today(HealthMetricSleepSeconds);
    s_deep_sleep = health_service_sum_today(HealthMetricSleepRestfulSeconds);
  }
	update_display();
}

void main_window_load(Window *window) {
	settings_load();

	//light_enable(true);

	window_set_background_color(s_window, settings.Background);

	setlocale(LC_ALL, "");

	remote_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(42, 37), PBL_IF_ROUND_ELSE(181, 145), 40));
	text_layer_set_text_color(remote_time_layer, settings.RemoteTime);
	text_layer_set_background_color(remote_time_layer, GColorClear);
	text_layer_set_font(remote_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_18)));
	text_layer_set_text_alignment(remote_time_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(remote_time_layer));

  local_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(52, 47), PBL_IF_ROUND_ELSE(184, 147), 55));
	text_layer_set_text_color(local_time_layer, settings.LocalTime);
	text_layer_set_background_color(local_time_layer, GColorClear);
	text_layer_set_font(local_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_48)));
	text_layer_set_text_alignment(local_time_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(local_time_layer));

  day_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(101, 96), PBL_IF_ROUND_ELSE(183, 146), 55));
	text_layer_set_text_color(day_layer, settings.Day);
	text_layer_set_background_color(day_layer, GColorClear);
	text_layer_set_font(day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_24)));
	text_layer_set_text_alignment(day_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(day_layer));

  date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(128, 123), PBL_IF_ROUND_ELSE(181, 145), 20));
	text_layer_set_text_color(date_layer, settings.Date);
	text_layer_set_background_color(date_layer, GColorClear);
	text_layer_set_font(date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_18)));
	text_layer_set_text_alignment(date_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(date_layer));

	data_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(148, 144), PBL_IF_ROUND_ELSE(181, 145), 40));
	text_layer_set_text_color(data_layer, GColorWhite);
	text_layer_set_background_color(data_layer, GColorClear);
	text_layer_set_font(data_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_18)));
	text_layer_set_text_alignment(data_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(data_layer));

	health_service_events_subscribe(health_handler, NULL);
	health_handler(HealthEventMovementUpdate, NULL);
	health_handler(HealthEventSleepUpdate, NULL);

	battery_update(battery_state_service_peek());
	battery_state_service_subscribe(&battery_update);

	force_tick();
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

	if(!DISPLAYMODE_NONE) {
		accel_tap_service_subscribe(tap_handler);
		update_display();
	}

}

void main_window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	health_service_events_unsubscribe();
	accel_tap_service_unsubscribe();
	battery_state_service_unsubscribe();

  text_layer_destroy(local_time_layer);
  text_layer_destroy(day_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(remote_time_layer);
  text_layer_destroy(data_layer);
}

void handle_init(void) {
  s_window = window_create();
	s_window_layer = window_get_root_layer(s_window);

	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
	});

  window_stack_push(s_window, true);

	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(128, 128);
}

void handle_deinit(void) {
	layer_destroy(s_window_layer);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
