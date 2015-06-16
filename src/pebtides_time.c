#include <pebble.h>

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#define MAX_TIDE_EVENTS 4
#define MAX_NAME_LENGTH 48

#define MIN_LEVEL 30
#define MAX_LEVEL 130

#define LEFT_MARGIN 5

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

static Layer *blue_layer;

static TextLayer *name_text_layer;
static TextLayer *tide_event_text_layer;
static TextLayer *at_text_layer;
static TextLayer *height_text_layer;
static TextLayer *counter_text_layer;

#define tide_event_text_layer_bounds (GRect) { .origin = { LEFT_MARGIN, 40 }, .size = { SCREEN_WIDTH - LEFT_MARGIN, 50 } }
#define at_text_layer_bounds (GRect) { .origin = { LEFT_MARGIN, 80 }, .size = { SCREEN_WIDTH - LEFT_MARGIN, 50 } }


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
static int has_data = 0;
static int level_height = SCREEN_HEIGHT/2; // how many pixels above the bottom to draw the blue layer
static int min_height = 10000;
static int max_height = 0;

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

    snprintf(height_text,10,"%d.%d%d%s",d1,d2,d3, unit);    
    text_layer_set_text(height_text_layer, height_text);

    snprintf(counter_text,6,"%d/%d",data_index + 1, n_events);    
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

static Animation *create_anim_scroll(int down) {
  //the combined animation for scrolling up all the fields

  //first create all the scroll out animations and combine to a spawn animation
  Animation *tide_event_text_layer_out_animation =  create_anim_scroll_out((Layer*) tide_event_text_layer, down);
  Animation *at_text_layer_out_animation =  create_anim_scroll_out((Layer*) at_text_layer, down);

  Animation *scroll_out = animation_spawn_create(tide_event_text_layer_out_animation,at_text_layer_out_animation, NULL);

  // add a callback to swap the data at the end of the scoll out animation
  // You may set handlers to listen for the start and stop events
  animation_set_handlers(scroll_out, (AnimationHandlers) {
    .started = (AnimationStartedHandler) animation_started,
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL);

  //same with the scoll in animations
  Animation *tide_event_text_layer_in_animation = create_anim_scoll_in((Layer*) tide_event_text_layer, tide_event_text_layer_bounds, down);
  Animation *at_text_layer_in_animation = create_anim_scoll_in((Layer*) at_text_layer, at_text_layer_bounds, down);

  Animation *scroll_in = animation_spawn_create(tide_event_text_layer_in_animation,at_text_layer_in_animation, NULL);

  //create a sequence animation from the scroll out and scroll in
  Animation *scroll_in_and_out = animation_sequence_create(scroll_out, scroll_in, NULL);

  //also shift the height to the correct level
  level_height = ((heights.values[data_index] - min_height)*(MAX_LEVEL - MIN_LEVEL))/(max_height-min_height) + MIN_LEVEL;
  GRect from_frame = layer_get_frame((Layer*) height_text_layer);
  GRect to_frame = GRect(from_frame.origin.x, SCREEN_HEIGHT - level_height, from_frame.size.w, from_frame.size.h);
  PropertyAnimation *shift_height_animation = property_animation_create_layer_frame((Layer*) height_text_layer, &from_frame, &to_frame);

  GRect from_frame_blue = layer_get_frame((Layer*) blue_layer);
  GRect to_frame_blue = GRect(from_frame_blue.origin.x, SCREEN_HEIGHT - level_height, from_frame_blue.size.w, from_frame_blue.size.h);
  PropertyAnimation *shift_blue_animation = property_animation_create_layer_frame((Layer*) blue_layer, &from_frame_blue, &to_frame_blue);

  return animation_spawn_create(scroll_in_and_out, (Animation*) shift_height_animation, (Animation*) shift_blue_animation, NULL);
  //return (Animation*) shift_height_animation;
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

  //find the minimum and maximum heights
  for(int i=0; i < n_events; i++)
  {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "time: %d, height : %d, event : %d", times.values[i], heights.values[i], events[i]);
      if(heights.values[i] < min_height) {
        min_height = heights.values[i];
      }
      if(heights.values[i] > max_height) {
        max_height = heights.values[i];
      }
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Min height: %d, max_height: %d", min_height, max_height);

  has_data = 1;
  update_display_data();
  layer_mark_dirty(window_get_root_layer(window));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void blue_layer_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorPictonBlue);
  graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y + SCREEN_HEIGHT - level_height, bounds.size.w, bounds.size.h), 0, GCornerNone);
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  window_set_background_color(window, GColorBrass);
  GRect bounds = layer_get_bounds(window_layer);

  //add the blue layer at the base
  blue_layer = layer_create(bounds);
  layer_set_update_proc(blue_layer, blue_layer_update_callback);
  layer_add_child(window_layer, blue_layer);


  //create the name text layer
  name_text_layer = text_layer_create((GRect) { .origin = { LEFT_MARGIN, 10 }, .size = { bounds.size.w - 40, 30 } });
  text_layer_set_font(name_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
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
  height_text_layer = text_layer_create((GRect) { .origin = { bounds.size.w - 45, bounds.origin.y + SCREEN_HEIGHT - level_height }, .size = { bounds.size.w - LEFT_MARGIN, 50 } });
  text_layer_set_font(height_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(height_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(height_text_layer));

  //create the counter text layer
  counter_text_layer = text_layer_create((GRect) { .origin = { bounds.size.w - 30, 10 }, .size = { bounds.size.w , 30 } });
  text_layer_set_font(name_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(counter_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(counter_text_layer));

}

static void window_unload(Window *window) {
  text_layer_destroy(name_text_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(data_index > 0 && has_data) {
    data_index -= 1;
    animation_schedule(create_anim_scroll(0));
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(data_index < (n_events - 1) && has_data) {
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
