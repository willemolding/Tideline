#pragma once

Animation *create_anim_scroll_out(Layer *layer, int up);

Animation *create_anim_scoll_in(Layer *layer, GRect dest, int up);

void animation_started(Animation *animation, void *data);

void animation_stopped(Animation *animation, bool finished, void *data);

#ifdef PBL_PLATFORM_BASALT

static Animation *create_anim_scroll(int down);

static Animation *create_anim_load();

#elif PBL_PLATFORM_APLITE

static Animation *create_anim_scroll(int down);

static Animation *create_anim_load();

#endif