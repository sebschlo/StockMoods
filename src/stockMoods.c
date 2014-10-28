#include <pebble.h>

#define KEY_SYMBOL 0
#define KEY_CHANGE 1
#define KEY_SIGN   2

static Window *window;

static TextLayer *time_layer;
static TextLayer *stock_layer;

static BitmapLayer *background_layer;
static GBitmap *background_bitmap;

/*
AUXILIARY FUNCTIONS
*/
static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // create a long-lived buffer
    static char buffer[] = "00:00";

    // write the current hours and minutes into the buffer
    if(clock_is_24h_style() == true) {
        strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
        strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    }

    // display this time on the textlayer
    text_layer_set_text(time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Update time every minute
  update_time();
  
  // Get stocks update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
 
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
 
    // Send the message!
    app_message_outbox_send();
  }
}

static void update_image(int change) {
  // If the percent change is positive show either indifferent or happy
  if (change == 0)
  {
    // If it is larger than 0.5, show a happy face
    background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HAPPY);
  } else if(change == 1) {
    // otherwise show an indifferent face
    background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_INDIFFERENT);
  } else {
    // if the change is negative, show a sad face
    background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SAD);
  }
  // Update the UI element at the end
  bitmap_layer_set_bitmap(background_layer, background_bitmap);
}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char change_buffer[20];
  static char symbol_buffer[20];
  static char stock_layer_buffer[40];
  static int change;

  // Read first item
  Tuple *t = dict_read_first(iterator);
 
  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      case KEY_SYMBOL:
        snprintf(symbol_buffer, sizeof(symbol_buffer), "%s", t->value->cstring);
        break;
      case KEY_CHANGE:
        snprintf(change_buffer, sizeof(change_buffer), "%s%%", t->value->cstring);
        break;
      case KEY_SIGN:
        change = (int)t->value->int32;
        APP_LOG(APP_LOG_LEVEL_INFO, "change thing %d", change);
        update_image(change);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }
 
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  // Assemble full string and display
  snprintf(stock_layer_buffer, sizeof(stock_layer_buffer), "%s, %s", symbol_buffer, change_buffer);
  text_layer_set_text(stock_layer, stock_layer_buffer);
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

/*
INITIALIZATION FUNCTIONS
*/
static void window_load(Window *window) {
  // get reference to the window and bounds 
  Layer *window_layer = window_get_root_layer(window);

  // Create GBitmap and then BitmapLayer
  background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NO_CONNECTION);
  background_layer = bitmap_layer_create(GRect(22,38,100,100));
  bitmap_layer_set_bitmap(background_layer, background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));

  // Create time layer
  time_layer = text_layer_create(GRect(0, 0, 144, 38));
  text_layer_set_background_color(time_layer, GColorWhite);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_text(time_layer, "00:00");
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));

  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  // create a stocks layer
  stock_layer = text_layer_create(GRect(0, 138, 144, 30));
  text_layer_set_background_color(stock_layer, GColorBlack);
  text_layer_set_text_color(stock_layer, GColorWhite);
  text_layer_set_text_alignment(stock_layer, GTextAlignmentCenter);
  text_layer_set_text(stock_layer, "Loading...");
  text_layer_set_font(stock_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));

  layer_add_child(window_layer, text_layer_get_layer(stock_layer));

  // update time from the start
  update_time();
}

static void window_unload(Window *window) {
  // Destroy GBitmap and bitmap layer
  gbitmap_destroy(background_bitmap);
  bitmap_layer_destroy(background_layer);

  // Destroy text layers
  text_layer_destroy(time_layer);
  text_layer_destroy(stock_layer);
}

static void init(void) {
  // create main window and assign to pointer
  window = window_create();

  // set handlers to manage window elements
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  // show the window on the watch
  const bool animated = true;
  window_stack_push(window, animated);

  // register with tickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
  window_destroy(window);
}

/*
MAIN
*/
int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();

  return 0;
}
