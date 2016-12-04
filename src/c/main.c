#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static BitmapLayer *s_background_layer, *s_bt_icon_layer;
static GBitmap *s_background_bitmap, *s_bt_icon_bitmap;
static GFont s_small_font, s_large_font;
static int s_battery_level;
static Layer *s_battery_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

	// Copy date into buffer from tm structure
	static char date_buffer[32];
	strftime(date_buffer, sizeof(date_buffer), "%a, %b %d, %Y", tick_time);
	
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
	
	// Show the date
	text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar (total width = 114px)
  int width = (s_battery_level * 114) / 100;

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the Background GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_GG_BACKGROUND);

  // Create the Background BitmapLayer to display the Background GBitmap
  s_background_layer = bitmap_layer_create(bounds);

  // Set the Background bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
	
	// Create the Bluetooth icon GBitmap
	s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);
	
	// Create the BitmapLayer to display the Bluetooth icon GBitmap
	#if defined(PBL_PLATFORM_EMERY)
		s_bt_icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - 10, 175, 15, 30));
	#else
		s_bt_icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - 10, PBL_IF_ROUND_ELSE(150, 120), 15, 30));
	#endif
	bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
	#if defined(PBL_COLOR)
	bitmap_layer_set_compositing_mode(s_bt_icon_layer, PBL_IF_BW_ELSE(GCompOpAssign, GCompOpSet));
	#endif
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
	
	// Create battery meter Layer
	#if defined(PBL_PLATFORM_EMERY)
		s_battery_layer = layer_create(GRect((bounds.size.w / 2) - 60, 215, 120, 2));
	#else
		s_battery_layer = layer_create(GRect(PBL_IF_BW_ELSE(0, (bounds.size.w / 2) - 60), PBL_IF_ROUND_ELSE(145, 160), 
																				 PBL_IF_BW_ELSE(bounds.size.w, 120), PBL_IF_BW_ELSE(10 ,2)));
	#endif
	layer_set_update_proc(s_battery_layer, battery_update_proc);
	
	// Add to Window
	layer_add_child(window_get_root_layer(window), s_battery_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect((bounds.size.w / 2) - 50, PBL_IF_ROUND_ELSE(15, 0), 100, 35));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, PBL_IF_BW_ELSE(GColorBlack, GColorClear));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

	// Create date TextLayer
	#if defined(PBL_PLATFORM_EMERY)
		s_date_layer = text_layer_create(GRect(0, 200, bounds.size.w, 50));
	#else
		s_date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(130, 145), bounds.size.w, 15));
	#endif
	text_layer_set_background_color(s_date_layer, PBL_IF_BW_ELSE(GColorBlack, GColorClear));
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_text(s_date_layer, "Xxx, Xxx 00, 0000");
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	
  // Create GFont
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GGFONT_12));
	s_large_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_GGFONT_32));
	
  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_large_font);
	text_layer_set_font(s_date_layer, s_small_font);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	
	// Show the correct state of the BT connection from the start
	bluetooth_callback(connection_service_peek_pebble_app_connection());
	
	// Update meter
	layer_mark_dirty(s_battery_layer);
}

static void main_window_unload(Window *window) {
  // Destroy Layers
  text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	layer_destroy(s_battery_layer);

  // Unload GFonts
  fonts_unload_custom_font(s_small_font);
	fonts_unload_custom_font(s_large_font);

  // Destroy GBitmaps
  gbitmap_destroy(s_background_bitmap);
	gbitmap_destroy(s_bt_icon_bitmap);

  // Destroy BitmapLayers
  bitmap_layer_destroy(s_background_layer);
	bitmap_layer_destroy(s_bt_icon_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

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
	battery_state_service_subscribe(battery_callback);
	
	// Ensure battery level is displayed from the start
	battery_callback(battery_state_service_peek());
	
	// Register for Bluetooth connection updates
  bluetooth_connection_service_subscribe(bluetooth_callback);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}