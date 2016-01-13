#include <pebble.h>

static Window *s_window;

static Layer *s_window_layer;
TextLayer *local_time_layer, *remote_time_layer, *day_layer, *date_layer;
static time_t offset_seconds = 28800; //8 hours

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	
  if (units_changed & DAY_UNIT) {
		setlocale(LC_ALL, ""); 
		
		static char s_day_buffer[20];
		strftime(s_day_buffer, sizeof(s_day_buffer), "%d %a", tick_time);
    text_layer_set_text(day_layer, s_day_buffer);
		
    static char s_date_buffer[20];
		strftime(s_date_buffer, sizeof(s_date_buffer), "%b %Y", tick_time);
    text_layer_set_text(date_layer, s_date_buffer);
  }
	
  if (units_changed & MINUTE_UNIT) { 
    static char s_time_buffer[] = "00:00";
    strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(local_time_layer, s_time_buffer);
		
		static char s_time_buffer2[] = "PST: 00:00";
		time_t now_seconds = mktime(tick_time);
		time_t seconds = now_seconds - offset_seconds;
		struct tm *tm_remote = localtime(&seconds);
    strftime(s_time_buffer2, sizeof(s_time_buffer2), clock_is_24h_style() ? "PST: %H:%M" : "PST: %I:%M", tm_remote);
    text_layer_set_text(remote_time_layer, s_time_buffer2);		
  }
	
}

static void main_window_load(Window *window) {
	
	remote_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(42, 37), PBL_IF_ROUND_ELSE(181, 145), 40));
	text_layer_set_text_color(remote_time_layer, GColorLightGray);
	text_layer_set_background_color(remote_time_layer, GColorClear);
	text_layer_set_font(remote_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_18)));
	text_layer_set_text_alignment(remote_time_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(remote_time_layer));
	
  local_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(52, 47), PBL_IF_ROUND_ELSE(184, 147), 55));  
	text_layer_set_text_color(local_time_layer, GColorWhite); 
	text_layer_set_background_color(local_time_layer, GColorClear);
	text_layer_set_font(local_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_48)));
	text_layer_set_text_alignment(local_time_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(local_time_layer));
	
  day_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(101, 96), PBL_IF_ROUND_ELSE(183, 146), 55));
	text_layer_set_text_color(day_layer, GColorWhite);
	text_layer_set_background_color(day_layer, GColorClear);
	text_layer_set_font(day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_24)));
	text_layer_set_text_alignment(day_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(day_layer));
	
  date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(128, 123), PBL_IF_ROUND_ELSE(181, 145), 20));
	text_layer_set_text_color(date_layer, GColorWhite);
	text_layer_set_background_color(date_layer, GColorClear);
	text_layer_set_font(date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_IMAGINE_18)));
	text_layer_set_text_alignment(date_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
	layer_add_child(s_window_layer, text_layer_get_layer(date_layer));

	time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  

  handle_tick(tick_time, DAY_UNIT + MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	
}

static void main_window_unload(Window *window) {
	tick_timer_service_unsubscribe();
  text_layer_destroy(local_time_layer);
  text_layer_destroy(day_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(remote_time_layer);
}


void handle_init(void) {
  s_window = window_create();
	window_set_background_color(s_window, GColorBlack);
	s_window_layer = window_get_root_layer(s_window);
	
	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
	});
	
  window_stack_push(s_window, true);
}

void handle_deinit(void) {
	layer_destroy(s_window_layer);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
