#include <pebble.h>

#define MAX_TIDE_EVENTS 10
#define MAX_NAME_LENGTH 20

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


static Window *window;
static TextLayer *text_layer;

static IntByteArray times;
static IntByteArray heights;
static char events[MAX_TIDE_EVENTS];
static char name[MAX_NAME_LENGTH];
static char unit[3];
static int n_events = 0;


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
        memcpy(&times, tuple->value->data, sizeof(IntByteArray));
        break;
      case HEIGHTS:
        memcpy(&heights, tuple->value->data, sizeof(IntByteArray));
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
  layer_mark_dirty(window_get_root_layer(window));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "Press a button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {

  window = window_create();
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

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
