#include <pebble.h>
#include "tide_data.h"

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#define MIN_LEVEL 30
#define MAX_LEVEL 130

#define LEFT_MARGIN 5


// the text layers to display the info
static Window *window;

static Layer *blue_layer;
static Layer *line_layer;

static TextLayer *name_text_layer;
static TextLayer *tide_event_text_layer;
static TextLayer *at_text_layer;
static TextLayer *height_text_layer;
static TextLayer *counter_text_layer;

#define tide_event_text_layer_bounds (GRect) { .origin = { LEFT_MARGIN, 40 }, .size = { SCREEN_WIDTH - LEFT_MARGIN, 50 } }
#define at_text_layer_bounds (GRect) { .origin = { LEFT_MARGIN, 80 }, .size = { SCREEN_WIDTH - LEFT_MARGIN, 53 } }

static TideData tide_data;

static char timestring[20];
static char counter_text[6];
static char height_text[10];

static char error_message[50];

static int data_index = 0;
static int has_data = 0;
static int level_height = SCREEN_HEIGHT / 2; // how many pixels above the bottom to draw the blue layer
static int min_height = 10000;
static int max_height = 0;

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

    snprintf(counter_text,6,"%d/%d",data_index + 1, tide_data.n_events);    
    text_layer_set_text(counter_text_layer,counter_text);

}

static Animation *create_anim_scroll_out(Layer *layer, int up) {
  //creates an animation that shrinks a layer into either its top or bottom edge
  // Set start and end
  GRect from_frame = layer_get_frame(layer);
  GRect to_frame;
  if(up) {
    to_frame = GRect(from_frame.origin.x, from_frame.origin.y, from_frame.size.w, 0);
  }
  else {
    to_frame = GRect(from_frame.origin.x, from_frame.origin.y + from_frame.size.h, from_frame.size.w, 0);
  }

  // Create the animation
  return (Animation*) property_animation_create_layer_frame(layer, &from_frame, &to_frame);
}

static Animation *create_anim_scoll_in(Layer *layer, GRect dest, int up) {
  GRect from_frame;
  GRect to_frame = dest;

  if(up) {
    from_frame = GRect(to_frame.origin.x, to_frame.origin.y + to_frame.size.h, to_frame.size.w, 0);
  }
  else {
    from_frame = GRect(to_frame.origin.x, to_frame.origin.y, to_frame.size.w, 0);
  }
  return (Animation*) property_animation_create_layer_frame(layer, &from_frame, &to_frame);
}

void animation_started(Animation *animation, void *data) {
  // Animation started!

}

void animation_stopped(Animation *animation, bool finished, void *data) {
   update_display_data();
}

#ifdef PBL_PLATFORM_BASALT

static Animation *create_anim_scroll(int down) {
  //the combined animation for scrolling up all the fields

  //first create all the scroll out animations and combine to a spawn animation
  Animation *tide_event_text_layer_out_animation =  create_anim_scroll_out((Layer*) tide_event_text_layer, down);
  animation_set_duration((Animation*) tide_event_text_layer_out_animation, 150);
  Animation *at_text_layer_out_animation =  create_anim_scroll_out((Layer*) at_text_layer, down);
  animation_set_duration((Animation*) at_text_layer_out_animation, 150);

  Animation *scroll_out = animation_spawn_create(tide_event_text_layer_out_animation,at_text_layer_out_animation, NULL);

  // add a callback to swap the data at the end of the scoll out animation
  // You may set handlers to listen for the start and stop events
  animation_set_handlers(scroll_out, (AnimationHandlers) {
    .started = (AnimationStartedHandler) animation_started,
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL);

  //same with the scoll in animations
  Animation *tide_event_text_layer_in_animation = create_anim_scoll_in((Layer*) tide_event_text_layer, tide_event_text_layer_bounds, down);
  animation_set_duration((Animation*) tide_event_text_layer_in_animation, 150);
  Animation *at_text_layer_in_animation = create_anim_scoll_in((Layer*) at_text_layer, at_text_layer_bounds, down);
  animation_set_duration((Animation*) at_text_layer_in_animation, 150);

  Animation *scroll_in = animation_spawn_create(tide_event_text_layer_in_animation,at_text_layer_in_animation, NULL);

  //create a sequence animation from the scroll out and scroll in
  Animation *scroll_in_and_out = animation_sequence_create(scroll_out, scroll_in, NULL);

  //also shift the height to the correct level
  level_height = ((tide_data.heights.values[data_index] - min_height)*(MAX_LEVEL - MIN_LEVEL))/(max_height-min_height) + MIN_LEVEL;

  GRect from_frame = layer_get_frame((Layer*) height_text_layer);
  GRect to_frame = GRect(from_frame.origin.x, SCREEN_HEIGHT - level_height, from_frame.size.w, from_frame.size.h);
  PropertyAnimation *shift_height_animation = property_animation_create_layer_frame((Layer*) height_text_layer, &from_frame, &to_frame);
  animation_set_delay((Animation*) shift_height_animation, 150);
  animation_set_duration((Animation*) shift_height_animation, 1000);


  GRect from_frame_blue = layer_get_frame((Layer*) blue_layer);
  GRect to_frame_blue = GRect(from_frame_blue.origin.x, SCREEN_HEIGHT - level_height, from_frame_blue.size.w, from_frame_blue.size.h);
  PropertyAnimation *shift_blue_animation = property_animation_create_layer_frame((Layer*) blue_layer, &from_frame_blue, &to_frame_blue);
  animation_set_delay((Animation*) shift_blue_animation, 150);
  animation_set_duration((Animation*) shift_blue_animation, 1000);

  return animation_spawn_create(scroll_in_and_out, (Animation*) shift_height_animation, (Animation*) shift_blue_animation, NULL);
}

static Animation *create_anim_load() {
  //the loading animation for the blue_layer
  GRect from_frame = layer_get_frame((Layer*) blue_layer);
  GRect to_frame = GRect(from_frame.origin.x, from_frame.origin.y + 20, from_frame.size.w, from_frame.size.h);

  PropertyAnimation *tide_rise = property_animation_create_layer_frame((Layer*) blue_layer, &from_frame, &to_frame);
  animation_set_duration((Animation*) tide_rise, 2000);

  PropertyAnimation *tide_fall = property_animation_create_layer_frame((Layer*) blue_layer, &to_frame, &from_frame);
  animation_set_duration((Animation*) tide_fall, 2000);

  Animation *load_sequence = animation_sequence_create((Animation*) tide_rise, (Animation*) tide_fall, NULL);
  animation_set_play_count(load_sequence, 20);
  return load_sequence;

}

#elif PBL_PLATFORM_APLITE


static Animation *create_anim_scroll(int down) {
  GRect frame = layer_get_frame((Layer*) tide_event_text_layer);
  Animation *scroll_out = (Animation*) property_animation_create_layer_frame((Layer*)tide_event_text_layer, &frame, &frame);
  animation_set_handlers(scroll_out, (AnimationHandlers) {
    .started = (AnimationStartedHandler) animation_started,
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL);
  return scroll_out;
}

static Animation *create_anim_load() {
  GRect frame = layer_get_frame((Layer*) tide_event_text_layer);
  return (Animation*) property_animation_create_layer_frame((Layer*)tide_event_text_layer, &frame, &frame);
}

#endif

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
    animation_schedule(create_anim_scroll(1));
    layer_mark_dirty(window_get_root_layer(window));

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

  //create the counter text layer
  counter_text_layer = text_layer_create((GRect) { .origin = { bounds.size.w - 35, 0}, .size = { bounds.size.w , 30 } });
  text_layer_set_font(counter_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(counter_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(counter_text_layer));

  //start the loading animation
  animation_schedule(create_anim_load());

}

static void window_appear(Window *window) {
}

static void window_unload(Window *window) {
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(data_index > 0 && has_data) {
    data_index -= 1;
    animation_schedule(create_anim_scroll(0));
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(data_index < (tide_data.n_events - 1) && has_data) {
    data_index += 1;
    animation_schedule(create_anim_scroll(1));
  }
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
    .appear = window_appear,
    .unload = window_unload,
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
    animation_schedule(create_anim_scroll(1));
    layer_mark_dirty(window_get_root_layer(window));
  }

}

static void deinit(void) {
  window_destroy(window);
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
