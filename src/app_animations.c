
#include "app_animations.h"
#include <pebble.h>

Animation *create_anim_scroll_out(Layer *layer, int up) {
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

Animation *create_anim_scoll_in(Layer *layer, GRect dest, int up) {
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

#ifdef PBL_PLATFORM_BASALT

Animation *create_anim_scroll(int down, void (*animation_stopped)(Animation *animation, bool finished, void *data)) {
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
  level_height = ((current_height - min_height)*(MAX_LEVEL - MIN_LEVEL))/(max_height-min_height) + MIN_LEVEL;


  GRect from_frame_blue = layer_get_frame((Layer*) blue_layer);
  GRect to_frame_blue = GRect(from_frame_blue.origin.x, SCREEN_HEIGHT - level_height, from_frame_blue.size.w, from_frame_blue.size.h);
  PropertyAnimation *shift_blue_animation = property_animation_create_layer_frame((Layer*) blue_layer, &from_frame_blue, &to_frame_blue);
  animation_set_delay((Animation*) shift_blue_animation, 150);
  animation_set_duration((Animation*) shift_blue_animation, 1000);

  return animation_spawn_create(scroll_in_and_out, (Animation*) shift_blue_animation, NULL);
}

Animation *create_anim_load() {
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


Animation *create_anim_scroll(int down, void (*animation_stopped)(Animation *animation, bool finished, void *data)) {
  GRect frame = layer_get_frame((Layer*) tide_event_text_layer);
  Animation *scroll_out = (Animation*) property_animation_create_layer_frame((Layer*)tide_event_text_layer, &frame, &frame);
  animation_set_handlers(scroll_out, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL);
  return scroll_out;
}

Animation *create_anim_load() {
  GRect frame = layer_get_frame((Layer*) tide_event_text_layer);
  return (Animation*) property_animation_create_layer_frame((Layer*)tide_event_text_layer, &frame, &frame);
}

#endif