#include <pebble.h>
#include <nightstand/nightstand.h>
#define BACK_ONE_KEY 0
#define BACK_TWO_KEY 1
#define NUMBER_ONE_KEY 2
#define NUMBER_TWO_KEY 3

#ifdef PBL_COLOR
GColor8 colors[4];
#endif

static Layer *background_layer;
static Window *window;
static GBitmap *number_bitmap;
static Animation *s_animation;
static int16_t s_hour, s_minute, s_last_hour;
static int16_t s_animation_percent;
static uint32_t resources[]={RESOURCE_ID_IMAGE_0,RESOURCE_ID_IMAGE_1,RESOURCE_ID_IMAGE_2,RESOURCE_ID_IMAGE_3,RESOURCE_ID_IMAGE_4,RESOURCE_ID_IMAGE_5,RESOURCE_ID_IMAGE_6,RESOURCE_ID_IMAGE_7,RESOURCE_ID_IMAGE_8,RESOURCE_ID_IMAGE_9,RESOURCE_ID_IMAGE_10,RESOURCE_ID_IMAGE_11,RESOURCE_ID_IMAGE_12,RESOURCE_ID_IMAGE_13,RESOURCE_ID_IMAGE_14,RESOURCE_ID_IMAGE_15,RESOURCE_ID_IMAGE_16,RESOURCE_ID_IMAGE_17,RESOURCE_ID_IMAGE_18,RESOURCE_ID_IMAGE_19,RESOURCE_ID_IMAGE_20,RESOURCE_ID_IMAGE_21,RESOURCE_ID_IMAGE_22,RESOURCE_ID_IMAGE_23};

// used to pass bimap info to get/set pixel accurately
typedef struct {
  uint8_t *bitmap_data;
  int bytes_per_row;
  GBitmapFormat bitmap_format;
}  BitmapInfo;

// set pixel color at given coordinates
void prv_set_pixel(BitmapInfo bitmap_info, int y, int x, uint8_t color) {

  if (bitmap_info.bitmap_format == GBitmapFormat1Bit) { // for 1 bit bitmap on Aplite  --- verify if it needs to be different
    bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] ^= (-color ^ bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8]) & (1 << (x % 8));
  } else { // othersise (assuming GBitmapFormat8Bit) going byte-wise
    bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x] = color;
  }
    
}
  
  // get pixel color at given coordinates
  uint8_t prv_get_pixel(BitmapInfo bitmap_info, int y, int x) {
  if (bitmap_info.bitmap_format == GBitmapFormat1Bit) { // for 1 bit bitmap on Aplite - shifting right to get bit
    return (bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] >> (x % 8)) & 1;
  } else {  // othersise (assuming GBitmapFormat8Bit) going byte-wise
    return bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x];
  }
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
#ifdef PBL_COLOR
  Tuple* tuple = dict_find(iter, BACK_ONE_KEY);
  if(tuple)
  {
    colors[0] = GColorFromHEX(tuple->value->int32);
    persist_write_int(BACK_ONE_KEY,tuple->value->int32);
  }
  tuple = dict_find(iter, BACK_TWO_KEY);
  if(tuple)
  {
    colors[1] = GColorFromHEX(tuple->value->int32);
    persist_write_int(BACK_TWO_KEY,tuple->value->int32);
  }
  tuple = dict_find(iter, NUMBER_ONE_KEY);
  if(tuple)
  {
    colors[2] = GColorFromHEX(tuple->value->int32);
    persist_write_int(NUMBER_ONE_KEY,tuple->value->int32);
  }
  tuple = dict_find(iter, NUMBER_TWO_KEY);
  if(tuple)
  {
    colors[3] = GColorFromHEX(tuple->value->int32);
    persist_write_int(NUMBER_TWO_KEY,tuple->value->int32);
  }
  layer_mark_dirty(background_layer);
#endif
}

static void loadcolors(void){
#ifdef PBL_COLOR
  colors[2] = persist_exists(NUMBER_ONE_KEY)? GColorFromHEX(persist_read_int(NUMBER_ONE_KEY)):GColorFromHEX(0x0000aa);
  colors[3] = persist_exists(NUMBER_TWO_KEY)? GColorFromHEX(persist_read_int(NUMBER_TWO_KEY)):GColorFromHEX(0xffff55);
  colors[0] = persist_exists(BACK_ONE_KEY)? GColorFromHEX(persist_read_int(BACK_ONE_KEY)):GColorFromHEX(0xffaaff);
  colors[1] = persist_exists(BACK_TWO_KEY)? GColorFromHEX(persist_read_int(BACK_TWO_KEY)):GColorFromHEX(0x005555);
#endif
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}


static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  // Init buffers
  app_message_open(100, 100);
}

static void implementation_update(Animation *animation,
                                  const AnimationProgress progress) {
  // Animate some completion variable
  s_animation_percent = ((int)progress * 100) / ANIMATION_NORMALIZED_MAX;
  layer_mark_dirty(background_layer);
}

static void background_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx,GColorBlack);
  graphics_fill_rect(ctx,bounds,0,GCornerNone);
  
  GRect frame = grect_inset(bounds, GEdgeInsets(-40));
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFillCircle, 130,
                       DEG_TO_TRIGANGLE(0), TRIG_MAX_ANGLE * s_minute * s_animation_percent / 6000);
  
  if(s_hour != s_last_hour) {
    // draw custom bitmap on top
    uint8_t hour = s_hour;
    if(!clock_is_24h_style())
    {
      hour = hour%12;
      if(hour==0)
        hour=12;
    }
    if(!number_bitmap)
        gbitmap_destroy(number_bitmap);
    number_bitmap = gbitmap_create_with_resource(resources[hour]);
    s_last_hour = s_hour;
  }
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  
  BitmapInfo bg_bitmap_info;
  bg_bitmap_info.bytes_per_row = gbitmap_get_bytes_per_row(number_bitmap);
  bg_bitmap_info.bitmap_data =  gbitmap_get_data(number_bitmap);
  bg_bitmap_info.bitmap_format = gbitmap_get_format(number_bitmap);
  
#ifdef PBL_PLATFORM_CHALK
  // Write a value to all visible pixels
  for(int y = 0; y < bounds.size.h; y++) {
    // Get the min and max x values for this row
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
    
    // Iterate over visible pixels in that row
    for(int x = info.min_x; x < info.max_x; x++) {
      //memset(&info.data[x], GColorBlack.argb, 1);
      uint8_t bmp_pixel = prv_get_pixel(bg_bitmap_info, y, x);
      uint8_t fb_pixel = info.data[x];
      if(bmp_pixel==0)
        memset(&info.data[x],gcolor_equal((GColor8)fb_pixel,GColorWhite)?colors[3].argb:colors[2].argb,1);
      else
        memset(&info.data[x],gcolor_equal((GColor8)fb_pixel,GColorWhite)?colors[1].argb:colors[0].argb,1);
    }
  }
#else
  BitmapInfo bitmap_info;
  bitmap_info.bitmap_data =  gbitmap_get_data(fb);
  bitmap_info.bytes_per_row = gbitmap_get_bytes_per_row(fb);
  bitmap_info.bitmap_format = gbitmap_get_format(fb);
  
  for (int y=0; y<168; y++) {
    for (int x=0; x<144; x++) {
      // we only want to set black pixels in the bitmap
      uint8_t bmp_pixel = prv_get_pixel(bg_bitmap_info, y, x);
      if(bmp_pixel==0)
      {
        uint8_t fb_pixel = prv_get_pixel(bitmap_info, y, x);
#ifdef PBL_COLOR
        prv_set_pixel(bitmap_info, y, x, gcolor_equal((GColor8)fb_pixel,GColorWhite)?colors[3].argb:colors[2].argb);
#else
        prv_set_pixel(bitmap_info, y, x, fb_pixel? 0:1);
#endif
      }
#ifdef PBL_COLOR
      else
      {
        uint8_t fb_pixel = prv_get_pixel(bitmap_info, y, x);
        prv_set_pixel(bitmap_info, y, x, gcolor_equal((GColor8)fb_pixel,GColorWhite)?colors[1].argb:colors[0].argb);
      }
#endif
    }
  }
#endif
  
  // release things
  graphics_release_frame_buffer(ctx, fb);

}


static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  s_hour = tick_time->tm_hour;
  s_minute = tick_time->tm_min;
  if(!nightstand_window_update())
    layer_mark_dirty(background_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // init background
  background_layer = layer_create(bounds);
  layer_set_update_proc(background_layer, background_update_proc);
  layer_add_child(window_layer, background_layer);
  
  // Create a new Animation
  s_animation = animation_create();
  animation_set_delay(s_animation, 100);
  animation_set_duration(s_animation, 500);
  
  // Create the AnimationImplementation
  static const AnimationImplementation implementation = {
    .update = implementation_update
  };
  animation_set_implementation(s_animation, &implementation);
  
  // force update
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hour = t->tm_hour;
  s_minute = t->tm_min;
  s_animation_percent=0;
  number_bitmap = NULL;
  s_last_hour = -1;
//  handle_tick(t, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  animation_schedule(s_animation);
}

static void window_unload(Window *window) {
  layer_destroy(background_layer);
  tick_timer_service_unsubscribe();
}

static void init(void) {
  app_message_init();
  loadcolors();
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  nightstand_window_init();
  
  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);
  
}

static void deinit(void) {
  window_destroy(window);
  nightstand_window_deinit();
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
