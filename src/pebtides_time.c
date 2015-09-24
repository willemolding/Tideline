#include <pebble.h>
#include "tide_data.h"
#include "app_animations.h"

// the text layers to display the info
static Window *window;

// layers for the load screen animation
Layer *blue_layer;
Layer *line_layer;

// text layers to display the data
TextLayer *name_text_layer;
TextLayer *tide_event_text_layer;
TextLayer *at_text_layer;
TextLayer *height_text_layer;

TideData tide_data;

// string buffers
static char timestring[20];
static char counter_text[6];
static char height_text[10];
static char error_message[50];

// other random global vars
int level_height = SCREEN_HEIGHT / 2; // how many pixels above the bottom to draw the blue layer
int min_height = 10000;
int max_height = 0;
int data_index = 0;
int has_data = 0;

static void update_display_data() {
    text_layer_set_text(name_text_layer, tide_data.name);

    if(tide_data.events[data_index] == 1) {
          text_layer_set_text(tide_event_text_layer, "HIGH TIDE");
    }
    else {
          text_layer_set_text(tide_event_text_layer, "LOW TIDE");
    }

    time_t t = tide_data.times.values[data_index];

    if(clock_is_24h_style()) {
      strftime(timestring + 3, 20, "%H:%M\n%B %d", localtime(&t));
    }
    else {
      strftime(timestring + 3, 20, "%I:%M %p\n%B %d", localtime(&t));
    }

    text_layer_set_text(at_text_layer, timestring);

    int x = tide_data.heights.values[data_index];
    int d1 = abs(x/100);
    int d2 = abs(x) - d1*100;

    //make sure the sign is right even for d1=0
    if(x>=0)
      snprintf(height_text,10,"%d.%d%s",d1,d2, tide_data.unit);  
    else
      snprintf(height_text,10,"-%d.%d%s",d1,d2, tide_data.unit);  

    text_layer_set_text(height_text_layer, height_text);

}

/**
Function to be called when scroll animations end. This is passed as a parameter to the create_scroll_anim function
This makes the transitions much more visually pleasing than changing the data before the animation
*/
void animation_stopped(Animation *animation, bool finished, void *data) {
   update_display_data();
   layer_mark_dirty(window_get_root_layer(window));
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

   // incoming message received
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message was received");

  Tuple *tuple = dict_read_first(iterator);
  bool is_error = false;

  //read in the data from the message using the dictionary iterator
  while (tuple) 
  {
    switch (tuple->key) 
    {
      case NAME:
        strcpy(tide_data.name,tuple->value->cstring);
        break;
      case UNIT:
        strcpy(tide_data.unit,tuple->value->cstring);
        break;
      case N_EVENTS:
        tide_data.n_events = tuple->value->int32;
        break;
      case TIMES:
        memcpy(tide_data.times.bytes, tuple->value->data, sizeof(IntByteArray));
        break;
      case HEIGHTS:
        memcpy(tide_data.heights.bytes, tuple->value->data, sizeof(IntByteArray));
        break;
      case EVENTS:
        memcpy(tide_data.events, tuple->value->data, MAX_TIDE_EVENTS);
        break;
      case ERROR_MSG:
        strcpy(error_message,tuple->value->cstring);
        is_error = true;
        break;
    }

    tuple = dict_read_next(iterator);
  }

  if(is_error == false) {

  	min_height = find_min(tide_data.heights.values, tide_data.n_events);
  	max_height = find_max(tide_data.heights.values, tide_data.n_events);    

    has_data = 1;
    data_index = 0;
    animation_unschedule_all();
    animation_schedule(create_anim_scroll(1, animation_stopped));

  }
  else { // push an error message window to the stack
      Window *error_window = window_create();
      window_set_background_color(error_window, COLOR_FALLBACK(GColorOrange,GColorWhite));
      Layer *error_window_layer = window_get_root_layer(error_window);
      GRect bounds = layer_get_bounds(error_window_layer);

      TextLayer *error_text_layer = text_layer_create(bounds);

      text_layer_set_text(error_text_layer, error_message);
      text_layer_set_font(error_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
      text_layer_set_background_color(error_text_layer, GColorClear);
      layer_add_child(error_window_layer, text_layer_get_layer(error_text_layer));

      window_stack_push(error_window, true);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void blue_layer_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorBlueMoon,GColorClear));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

static void line_layer_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  if(has_data){
      graphics_context_set_stroke_color(ctx, GColorBlack);
      #ifdef PBL_PLATFORM_BASALT
      graphics_context_set_stroke_width(ctx, 1);
      #endif
      graphics_draw_line(ctx, GPoint(LEFT_MARGIN, 32), GPoint(SCREEN_WIDTH - 50, 32));
  }
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  window_set_background_color(window, COLOR_FALLBACK(GColorPictonBlue, GColorWhite));
  GRect bounds = layer_get_bounds(window_layer);

  //add the blue layer at the base
  blue_layer = layer_create(GRect(bounds.origin.x, SCREEN_HEIGHT - level_height, bounds.size.w, bounds.size.h));
  layer_set_update_proc(blue_layer, blue_layer_update_callback);
  layer_add_child(window_layer, blue_layer);

  //add the line layer
  line_layer = layer_create(bounds);
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);


  //create the name text layer
  name_text_layer = text_layer_create((GRect) { .origin = { LEFT_MARGIN, 0 }, .size = { bounds.size.w - 40, 30 } });
  text_layer_set_font(name_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(name_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(name_text_layer));

  //create the event text layer
  tide_event_text_layer = text_layer_create(tide_event_text_layer_bounds);
  text_layer_set_text(tide_event_text_layer, "Getting Data");
  text_layer_set_font(tide_event_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(tide_event_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(tide_event_text_layer));

  //create the at text layer
  at_text_layer = text_layer_create(at_text_layer_bounds);
  text_layer_set_font(at_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(at_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(at_text_layer));

  //create the height text layer
  height_text_layer = text_layer_create((GRect) { .origin = { bounds.size.w - 45, SCREEN_HEIGHT - level_height }, .size = { bounds.size.w - LEFT_MARGIN, 50 } });
  text_layer_set_font(height_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(height_text_layer, GColorClear);
  #ifdef PBL_PLATFORM_BASALT
  text_layer_set_text_color(height_text_layer, GColorWhite);
  #endif
  layer_add_child(window_layer, text_layer_get_layer(height_text_layer));

  //start the loading animation
  animation_schedule(create_anim_load());

}



static void destroy_layers(){
  layer_destroy(blue_layer);
  layer_destroy(line_layer);
  text_layer_destroy(name_text_layer);
  text_layer_destroy(tide_event_text_layer);
  text_layer_destroy(at_text_layer);
  text_layer_destroy(height_text_layer);
}


static void init(void) {

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
  });
  const bool animated = true;

  window_stack_push(window, animated);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // displays cached data before waiting for more. This makes data still available without phone connection.
  if(load_tide_data(&tide_data)) {
    has_data = 1;
    data_index = 0;
    animation_unschedule_all();
    animation_schedule(create_anim_scroll(1, animation_stopped));
  }

}

static void deinit(void) {
  window_destroy(window);
  destroy_layers();
  if(has_data == 1){
  	store_tide_data(&tide_data);
  }
}

int main(void) {
  init();
  timestring[0] = 'A';
  timestring[1] = 'T';
  timestring[2] = ' ';

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
