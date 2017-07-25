#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_bt_layer, *s_batt_layer, *s_weather_layer;

static void update_battery(BatteryChargeState state) {
	static char s_battery_buffer[16];
	// Write the battert percentage to the buffer
	snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", state.charge_percent);
	// Write the buffer to the Battery TextLayer
	text_layer_set_text(s_batt_layer, s_battery_buffer);
}

static void battery_handler(BatteryChargeState state) {
  update_battery(state);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)temp_tuple->value->int32*2+32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);

    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


// show bt connected/disconnected
void display_bt_layer(bool connected) {
  vibes_short_pulse();
  if (connected) {
    text_layer_set_text(s_bt_layer, "BT");
  } else {
    text_layer_set_text(s_bt_layer, "X");
  }
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  // Copy date into buffer from tm structure
static char date_buffer[16];
strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

// Show the date
text_layer_set_text(s_date_layer, date_buffer);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  // Get weather update every 60 minutes
if(tick_time->tm_min == 0) {
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);

  // Send the message!
  app_message_outbox_send();
}

}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(10, 0), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create date TextLayer
s_date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(50, 0), bounds.size.w, 50));
text_layer_set_text_color(s_date_layer, GColorBlack);
text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_background_color(s_date_layer, GColorClear);
text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Add to Window
layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  // Bat Level
s_batt_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(75, 0), bounds.size.w, 50));
  text_layer_set_background_color(s_batt_layer, GColorClear);
  text_layer_set_text_color(s_batt_layer, GColorBlack);
  text_layer_set_text_alignment(s_batt_layer, GTextAlignmentCenter);
  
  // Add to Window
layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_batt_layer));

  // BT text
s_bt_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(90, 0), bounds.size.w, 50));
text_layer_set_text_color(s_bt_layer, GColorBlack);
text_layer_set_font(s_bt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
text_layer_set_background_color(s_bt_layer, GColorClear);
text_layer_set_text_alignment(s_bt_layer, GTextAlignmentCenter);
  
  // Add to Window
layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_bt_layer));
  
  // Initial BT Check
  if (bluetooth_connection_service_peek()) {
    text_layer_set_text(s_bt_layer, "BT");
  } else {
    text_layer_set_text(s_bt_layer, "X");
  }
  
  // Weather Layer
  s_weather_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(130, 0), bounds.size.w, 50));
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading");
  // Add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_bt_layer);
  text_layer_destroy(s_batt_layer);
  text_layer_destroy(s_weather_layer);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register for battery level updates
  
update_battery(battery_state_service_peek());
  
battery_state_service_subscribe(battery_handler);
  
bluetooth_connection_service_subscribe(display_bt_layer);

  // Register callbacks
app_message_register_inbox_received(inbox_received_callback);
app_message_register_inbox_dropped(inbox_dropped_callback);
app_message_register_outbox_failed(outbox_failed_callback);
app_message_register_outbox_sent(outbox_sent_callback);
app_message_open(512, 64);

}

static void deinit() {
  bluetooth_connection_service_unsubscribe();
  
  battery_state_service_unsubscribe();
  
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}