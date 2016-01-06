
#include "nightstand.h"

#define MAX_EFFECTS 4

typedef void effect_cb(GContext* ctx, GRect position, void* param);
// structure of effect layer
typedef struct {
  Layer*      layer;
  effect_cb*  effects[MAX_EFFECTS];
  void*       params[MAX_EFFECTS];
  uint8_t     next_effect;
} EffectLayer;

// used to pass bimap info to get/set pixel accurately
typedef struct {
  GBitmap *bitmap;  // actual bitmap for Chalk raw manipulation
  uint8_t *bitmap_data;
  int bytes_per_row;
  GBitmapFormat bitmap_format;
}  BitmapInfo;

static AccelData a_data;
static EffectLayer *effect_layer;
static char time_text[] = "00:00";
static TextLayer *night_time_layer;
static Window *window;
static bool loaded;

// set pixel color at given coordinates
static void prv_set_pixel(BitmapInfo bitmap_info, int y, int x, uint8_t color) {
  
#ifndef PBL_PLATFORM_APLITE
  if (bitmap_info.bitmap_format == GBitmapFormat1BitPalette) { // for 1bit palette bitmap on Basalt --- verify if it needs to be different
    bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] ^= (-color ^ bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8]) & (1 << (x % 8));
#else
    if (bitmap_info.bitmap_format == GBitmapFormat1Bit) { // for 1 bit bitmap on Aplite  --- verify if it needs to be different
      bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] ^= (-color ^ bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8]) & (1 << (x % 8));
#endif
    } else { // othersise (assuming GBitmapFormat8Bit) going byte-wise
      
#ifndef PBL_PLATFORM_CHALK
      bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x] = color;
#else
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(bitmap_info.bitmap, y);
      if ((x >= info.min_x) && (x <= info.max_x)) info.data[x] = color;
#endif
      
    }
    
  }


// get pixel color at given coordinates
static uint8_t prv_get_pixel(BitmapInfo bitmap_info, int y, int x) {
  
#ifndef PBL_PLATFORM_APLITE
  if (bitmap_info.bitmap_format == GBitmapFormat1BitPalette) { // for 1bit palette bitmap on Basalt shifting left to get correct bit
    return (bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] << (x % 8)) & 128;
#else
    if (bitmap_info.bitmap_format == GBitmapFormat1Bit) { // for 1 bit bitmap on Aplite - shifting right to get bit
      return (bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] >> (x % 8)) & 1;
#endif
    } else {  // othersise (assuming GBitmapFormat8Bit) going byte-wise
      
#ifndef PBL_PLATFORM_CHALK
      return bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x];
#else
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(bitmap_info.bitmap, y);
      if ((x >= info.min_x) && (x <= info.max_x))
        return info.data[x];
      else
        return -1;
#endif
    }
    
  }


// Rotate 90 degrees
// Added by Ron64
// Parameter:  true: rotate right/clockwise,  false: rotate left/counter_clockwise
static void prv_effect_rotate_90_degrees(GContext* ctx,  GRect position, void* param){
  
  //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  
  BitmapInfo bitmap_info;
  bitmap_info.bitmap = fb;
  bitmap_info.bitmap_data =  gbitmap_get_data(fb);
  bitmap_info.bytes_per_row = gbitmap_get_bytes_per_row(fb);
  bitmap_info.bitmap_format = gbitmap_get_format(fb);
  
  bool right = (bool)param;
  uint8_t qtr, xCn, yCn, temp_pixel;
  xCn= position.origin.x + position.size.w /2;
  yCn= position.origin.y + position.size.h /2;
  qtr=position.size.w;
  if (position.size.h < qtr)
    qtr= position.size.h;
  qtr= qtr/2;
  
  for (int c1 = 0; c1 < qtr; c1++)
    for (int c2 = 1; c2 < qtr; c2++){
      temp_pixel = prv_get_pixel(bitmap_info, yCn +c1, xCn +c2);
      if (right){
        prv_set_pixel(bitmap_info, yCn +c1, xCn +c2, prv_get_pixel(bitmap_info, yCn -c2, xCn +c1));
        prv_set_pixel(bitmap_info, yCn -c2, xCn +c1, prv_get_pixel(bitmap_info, yCn -c1, xCn -c2));
        prv_set_pixel(bitmap_info, yCn -c1, xCn -c2, prv_get_pixel(bitmap_info, yCn +c2, xCn -c1));
        prv_set_pixel(bitmap_info, yCn +c2, xCn -c1, temp_pixel);
      }
      else{
        prv_set_pixel(bitmap_info, yCn +c1, xCn +c2, prv_get_pixel(bitmap_info, yCn +c2, xCn -c1));
        prv_set_pixel(bitmap_info, yCn +c2, xCn -c1, prv_get_pixel(bitmap_info, yCn -c1, xCn -c2));
        prv_set_pixel(bitmap_info, yCn -c1, xCn -c2, prv_get_pixel(bitmap_info, yCn -c2, xCn +c1));
        prv_set_pixel(bitmap_info, yCn -c2, xCn +c1, temp_pixel);
      }
    }
  
  graphics_release_frame_buffer(ctx, fb);
}

// Find the offset of parent layer pointer
static uint8_t prv_find_parent_offset() {
  Layer* p = layer_create(GRect(0,0,32,32));
  Layer* l = layer_create(GRect(0,0,16,16));
  layer_add_child(p,l);
  
  uint8_t i=0;
  while(i<16 && *(((Layer**)(void*)l)+i)!=p) ++i;
  
  if(*(((Layer**)(void*)l)+i)!=p) {
    i=0xff;
    APP_LOG(APP_LOG_LEVEL_ERROR,"EffectLayer library was unable to find the parent layer offset! Your app will probably crash (sorry) :(");
  }
  
  layer_destroy(l);
  layer_destroy(p);
  return i;
}

// on layer update - apply effect
static void prv_effect_layer_update_proc(Layer *me, GContext* ctx) {
  static uint8_t parent_layer_offset = 0xff;
  if(parent_layer_offset == 0xff) {
    parent_layer_offset = prv_find_parent_offset();
  }
  
  // retrieving layer and its real coordinates
  EffectLayer* effect_layer = (EffectLayer*)(layer_get_data(me));
  GRect layer_frame = layer_get_frame(me);
  Layer* l = me;
  while((l=((Layer**)(void*)l)[parent_layer_offset])) {
    GRect parent_frame = layer_get_frame(l);
    layer_frame.origin.x += parent_frame.origin.x;
    layer_frame.origin.y += parent_frame.origin.y;
  }
  
  // Applying effects
  for(uint8_t i=0; effect_layer->effects[i] && i<MAX_EFFECTS;++i) effect_layer->effects[i](ctx, layer_frame, effect_layer->params[i]);
}

// create effect layer
static EffectLayer* prv_effect_layer_create(GRect frame) {
  
  //creating base layer
  Layer* layer =layer_create_with_data(frame, sizeof(EffectLayer));
  layer_set_update_proc(layer, prv_effect_layer_update_proc);
  EffectLayer* effect_layer = (EffectLayer*)layer_get_data(layer);
  memset(effect_layer,0,sizeof(EffectLayer));
  effect_layer->layer = layer;
  
  return effect_layer;
}

//destroy effect layer
static void prv_effect_layer_destroy(EffectLayer *effect_layer) {
  // precaution
  if (effect_layer != NULL && effect_layer->layer != NULL) {
    layer_destroy(effect_layer->layer);
    effect_layer->layer = NULL;
    effect_layer = NULL;
  }
  
}

// returns base layer
static Layer* prv_effect_layer_get_layer(EffectLayer *effect_layer){
  return effect_layer->layer;
}

//adds effect to the layer
static void prv_effect_layer_add_effect(EffectLayer *effect_layer, effect_cb* effect, void* param) {
  if(effect_layer->next_effect < MAX_EFFECTS) {
    effect_layer->effects[effect_layer->next_effect] = effect;
    effect_layer->params[effect_layer->next_effect] = param;
    ++effect_layer->next_effect;
  }
}

//removes last added effect
static void prv_effect_layer_remove_effect(EffectLayer *effect_layer) {
  if(effect_layer->next_effect > 0) {
    effect_layer->effects[effect_layer->next_effect - 1] = NULL;
    effect_layer->params[effect_layer->next_effect - 1] = NULL;
    --effect_layer->next_effect;
  }
}
    
static void loadWindow(Window *window) {
  loaded = true;
}
  
static void unloadWindow(Window *window) {
  loaded = false;
}
void nightstand_window_init(void){
  window = window_create();
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  window_set_window_handlers(window, (WindowHandlers) {
    .load = loadWindow,
    .unload = unloadWindow
  });
  loaded = false;
  window_set_background_color(window,GColorBlack);
  effect_layer = prv_effect_layer_create(GRect(0,(bounds.size.h-bounds.size.w)/2-1,bounds.size.w,bounds.size.w));
  night_time_layer = text_layer_create(GRect(0, bounds.size.h/2-25, bounds.size.w, 50));
  text_layer_set_background_color(night_time_layer, GColorBlack);
  text_layer_set_text_color(night_time_layer, GColorWhite);
  text_layer_set_font(night_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(night_time_layer, GTextAlignmentCenter);
  text_layer_set_text(night_time_layer,time_text);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(night_time_layer));
  layer_add_child(window_get_root_layer(window), prv_effect_layer_get_layer(effect_layer));
}

bool nightstand_window_update(void){
  char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  time_t now = time(NULL);
  struct tm* tick_time = localtime(&now);
  strftime(time_text, sizeof(time_text), time_format, tick_time);
  
  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }
  layer_mark_dirty(text_layer_get_layer(night_time_layer));
  accel_service_peek(&a_data);
  if(a_data.x<-900){ // rotate screen left
    prv_effect_layer_remove_effect(effect_layer);
    prv_effect_layer_add_effect(effect_layer,prv_effect_rotate_90_degrees,(void*)true);
    if(!loaded)
      window_stack_push(window,false);
    return true;
  }
  else if(a_data.x>900) { //rotate screen right
    prv_effect_layer_remove_effect(effect_layer);
    prv_effect_layer_add_effect(effect_layer,prv_effect_rotate_90_degrees,(void*)false);
    if(!loaded)
      window_stack_push(window,false);
    return true;
  }
  if(loaded)
    window_stack_pop(false);
  return false;
}

void nightstand_window_deinit(void){
  text_layer_destroy(night_time_layer);
  prv_effect_layer_destroy(effect_layer);
  window_destroy(window);
}