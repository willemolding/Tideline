#include <pebble.h>

#define MAX_TIDE_EVENTS 4
#define MAX_NAME_LENGTH 48

#define LEFT_MARGIN 10

enum {
    NAME,
    UNIT,
    N_EVENTS,
    TIMES,
    HEIGHTS,
    EVENTS
  };

typedef union IntByteArray
{
  int values[MAX_TIDE_EVENTS];
  char bytes[MAX_TIDE_EVENTS*4];
} IntByteArray;

// the text layers to display the info
static Window *window;
static TextLayer *name_text_layer;
static TextLayer *tide_event_text_layer;
static TextLayer *at_text_layer;
static TextLayer *height_text_layer;
static TextLayer *counter_text_layer;

static IntByteArray times;
static IntByteArray heights;
static char events[MAX_TIDE_EVENTS];
static char name[MAX_NAME_LENGTH];
static char unit[3];
static int n_events = 0;

static char timestring[20];
static char counter_text[6];
static char height_text[10];

static int data_index = 0;

static void update_display_data() {
    text_layer_set_text(name_text_layer, name);
    if(events[data_index] == 1) {
          text_layer_set_text(tide_event_text_layer, "HIGH TIDE");
    }
    else {
          text_layer_set_text(tide_event_text_layer, "LOW TIDE");
    }

    time_t t = times.values[data_index];
    if(clock_is_24h_style()) {
      strftime(timestring + 3, 20, "%H:%M", localtime(&t));
    }
    else {
      strftime(timestring + 3, 20, "%I:%M %p", localtime(&t));
    }
    text_layer_set_text(at_text_layer, timestring);

    int x = heights.values[data_index];
    int d1 = x/100;
    int d2 = x/10 - 10*d1;
    int d3 = x - 10*d2 - 100*d1;

    snprintf(height_text,10,"%d.%d%d %s",d1,d2,d3, unit);    
    text_layer_set_text(height_text_layer, height_text);

    snprintf(counter_text,6,"%d/%d",data_index + 1, n_events);    
    text_layer_set_text(counter_text_layer,counter_text);

}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

   // incoming message received
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message was received");

  Tuple *tuple = dict_read_first(iterator);

  //read in the data from the message using the dictionary iterator
  while (tuple) 
  {
    switch (tuple->key) 
    {
      case NAME:
        strcpy(name,tuple->value->cstring);
        break;
      case UNIT:
        strcpy(unit,tuple->value->cstring);
        break;
      case N_EVENTS:
        n_events = tuple->value->int32;
        break;
      case TIMES:
        memcpy(times.bytes, tuple->value->data, sizeof(IntByteArray));
        break;
      case HEIGHTS:
        memcpy(heights.bytes, tuple->value->data, sizeof(IntByteArray));
        break;
      case EVENTS:
        memcpy(events, tuple->value->data, MAX_TIDE_EVENTS);
        break;
    }

    tuple = dict_read_next(iterator);
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Name: %s, n_events : %d, unit : %s", name, n_events, unit);
  for(int i=0; i < n_events; i++)
  {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "time: %d, height : %d, event : %d", times.values[i], heights.values[i], events[i]);
  }

  update_display_data();
  layer_mark_dirty(window_get_root_layer(window));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  //create the name text layer
  name_text_layer = text_layer_create((GRect) { .origin = { LEFT_MARGIN, 10 }, .size = { bounds.size.w - 40, 30 } });
  text_layer_set_font(name_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(name_text_layer));

  //create the event text layer
  tide_event_text_layer = text_layer_create((GRect) { .origin = { LEFT_MARGIN, 40 }, .size = { bounds.size.w - LEFT_MARGIN, 50 } });
  text_layer_set_text(tide_event_text_layer, "Getting Data");
  text_layer_set_font(tide_event_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(tide_event_text_layer));

  //create the at text layer
  at_text_layer = text_layer_create((GRect) { .origin = { LEFT_MARGIN, 80 }, .size = { bounds.size.w - LEFT_MARGIN, 50 } });
  text_layer_set_font(at_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(at_text_layer));

  //create the height text layer
  height_text_layer = text_layer_create((GRect) { .origin = { LEFT_MARGIN, 120 }, .size = { bounds.size.w - LEFT_MARGIN, 50 } });
  text_layer_set_font(height_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(height_text_layer));

  //create the counter text layer
  counter_text_layer = text_layer_create((GRect) { .origin = { bounds.size.w - 30, 10 }, .size = { bounds.size.w , 30 } });
  text_layer_set_font(name_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(counter_text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(name_text_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(data_index > 0) {
    data_index -= 1;
  }
  update_display_data();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(data_index < (n_events - 1)) {
    data_index += 1;
  }
  update_display_data();
}

static void click_config_provider() {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void init(void) {

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  //open app message
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  timestring[0] = 'A';
  timestring[1] = 'T';
  timestring[2] = ' ';

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Size of IntByteArray: %d", sizeof(IntByteArray));  

  app_event_loop();
  deinit();
}
