#include <pebble.h>
#include "main.h"

static Window *s_window;
static Layer *s_window_layer;
TextLayer *local_time_layer, *remote_time_layer, *day_layer, *date_layer, *steps_layer, *sleep_layer;
AppSettings settings;
static char s_time_buffer[] = "00:00";
static char s_time_buffer2[] = "00:00";
static char s_time_buffer3[] = "XXXXX: 00:00";
static char s_steps_buffer[] = "Steps: 1234567890";
static char s_sleep_buffer[] = "Sleep: 1234567890";

void settings_load() {
	settings.Background = GColorRed;
	settings.LocalTime = GColorWhite;
	settings.RemoteTime = GColorLightGray;
	settings.Day = GColorWhite;
	settings.Date = GColorWhite;
	settings.Offset = 0;

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

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
		static char s_day_buffer[20];
		strftime(s_day_buffer, sizeof(s_day_buffer), "%d %a", tick_time);
    text_layer_set_text(day_layer, s_day_buffer);

    static char s_date_buffer[20];
		strftime(s_date_buffer, sizeof(s_date_buffer), "%b %Y", tick_time);
    text_layer_set_text(date_layer, s_date_buffer);
  }
  if (units_changed & MINUTE_UNIT) {
    strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(local_time_layer, s_time_buffer);


			time_t now_seconds = mktime(tick_time);
			now_seconds += settings.Offset;
			struct tm *tm_remote = gmtime(&now_seconds);

	    strftime(s_time_buffer2, sizeof(s_time_buffer2), clock_is_24h_style() ? "%H:%M" : "%I:%M", tm_remote);
			snprintf(s_time_buffer3, sizeof(s_time_buffer3), "%s: %s", settings.Label, s_time_buffer2);
	    text_layer_set_text(remote_time_layer, s_time_buffer3);

  }


	time_t now = time(NULL);
	time_t start = clock_to_timestamp(TODAY, tick_time->tm_hour - 5,
																		tick_time->tm_min);

	// Check data is available
	HealthServiceAccessibilityMask result = health_service_metric_accessible(HealthMetricStepCount, start, now);
	if(result == HealthServiceAccessibilityMaskAvailable) {
		APP_LOG(APP_LOG_LEVEL_INFO, "Available");
	}
	else {
		APP_LOG(APP_LOG_LEVEL_ERROR, "No data available!");
	}

	APP_LOG(APP_LOG_LEVEL_INFO, "HealthServiceAccessibilityMaskAvailable: %d", result);
}

void force_tick() {
	time_t now = time(NULL);
	struct tm *tick_time = localtime(&now);
	handle_tick(tick_time, DAY_UNIT|MINUTE_UNIT);
}


static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) {
    const uint32_t steps_today = health_service_sum_today(HealthMetricStepCount);
		snprintf(s_steps_buffer, sizeof(s_steps_buffer), "Steps: %li", steps_today);
		text_layer_set_text(steps_layer, s_steps_buffer);
  } else if (event == HealthEventSleepUpdate) {
    const uint32_t seconds_sleep_today = health_service_sum_today(HealthMetricSleepSeconds);
    const uint32_t seconds_restful_sleep_today = health_service_sum_today(HealthMetricSleepRestfulSeconds);
		snprintf(s_sleep_buffer, sizeof(s_sleep_buffer), "Sleep: %li", seconds_sleep_today);
		text_layer_set_text(sleep_layer, s_sleep_buffer);
  }
}

void main_window_load(Window *window) {
	settings_load();

	//light_enable(true);

	window_set_background_color(s_window, settings.Background);

	setlocale(LC_ALL, "");

	steps_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(12, 7), PBL_IF_ROUND_ELSE(181, 145), 40));
	text_layer_set_text_color(steps_layer, GColorWhite);
	text_layer_set_background_color(steps_layer, GColorClear);
	text_layer_set_font(steps_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_18)));
	text_layer_set_text_alignment(steps_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(steps_layer));

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

	sleep_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(147, 143), PBL_IF_ROUND_ELSE(181, 145), 40));
	text_layer_set_text_color(sleep_layer, GColorWhite);
	text_layer_set_background_color(sleep_layer, GColorClear);
	text_layer_set_font(sleep_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_18)));
	text_layer_set_text_alignment(sleep_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(sleep_layer));

	health_service_events_subscribe(health_handler, NULL);
	health_service_peek_current_activities();
	health_handler(HealthEventMovementUpdate, NULL);
	health_handler(HealthEventSleepUpdate, NULL);

	force_tick();

  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);

}

void main_window_unload(Window *window) {
	tick_timer_service_unsubscribe();

	health_service_events_unsubscribe();

  text_layer_destroy(local_time_layer);
  text_layer_destroy(day_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(remote_time_layer);
  text_layer_destroy(steps_layer);
  text_layer_destroy(sleep_layer);
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
