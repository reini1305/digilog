#include <pebble.h>

static Layer *background_layer;
static Window *window;
static GBitmap *number_bitmap;

static const GPathInfo QUINT1_PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{72, 84}, {72, 0}, {144, 0}}
};
static const GPathInfo QUINT2_PATH_INFO = {
  .num_points = 4,
  .points = (GPoint []) {{72, 84}, {72, 0}, {144, 0}, {144, 168}}
};
static const GPathInfo QUINT3_PATH_INFO = {
  .num_points = 5,
  .points = (GPoint []) {{72, 84}, {72, 0}, {144, 0}, {144, 168}, {0, 168}}
};
static const GPathInfo QUINT4_PATH_INFO = {
  .num_points = 6,
  .points = (GPoint []) {{72, 84}, {72, 0}, {144, 0}, {144, 168}, {-1, 168}, {-1, -1}}
};
static const GPathInfo QUINT5_PATH_INFO = {
  .num_points = 7,
  .points = (GPoint []) {{72, 84}, {72, 0}, {144, 0}, {144, 168}, {-1, 168}, {-1, -1}, {70, 0}}
};

static GPath* time_path;

// used to pass bimap info to get/set pixel accurately
typedef struct {
  uint8_t *bitmap_data;
  int bytes_per_row;
  GBitmapFormat bitmap_format;
}  BitmapInfo;

// set pixel color at given coordinates
void set_pixel(BitmapInfo bitmap_info, int y, int x, uint8_t color) {
  
#ifdef PBL_PLATFORM_BASALT
  if (bitmap_info.bitmap_format == GBitmapFormat1BitPalette) { // for 1bit palette bitmap on Basalt --- verify if it needs to be different
    bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] ^= (-color ^ bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8]) & (1 << (x % 8));
#else
    if (bitmap_info.bitmap_format == GBitmapFormat1Bit) { // for 1 bit bitmap on Aplite  --- verify if it needs to be different
      bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] ^= (-color ^ bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8]) & (1 << (x % 8));
#endif
    } else { // othersise (assuming GBitmapFormat8Bit) going byte-wise
      bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x] = color;
    }
    
  }
  
  // get pixel color at given coordinates
  uint8_t get_pixel(BitmapInfo bitmap_info, int y, int x) {
    
#ifdef PBL_PLATFORM_BASALT
    if (bitmap_info.bitmap_format == GBitmapFormat1BitPalette) { // for 1bit palette bitmap on Basalt shifting left to get correct bit
      return (bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] << (x % 8)) & 128;
#else
      if (bitmap_info.bitmap_format == GBitmapFormat1Bit) { // for 1 bit bitmap on Aplite - shifting right to get bit
        return (bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x / 8] >> (x % 8)) & 1;
#endif
      } else {  // othersise (assuming GBitmapFormat8Bit) going byte-wise
        return bitmap_info.bitmap_data[y*bitmap_info.bytes_per_row + x];
      }
      
    }


static void background_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx,GColorBlack);
  graphics_fill_rect(ctx,bounds,0,GCornerNone);
  
  // get current time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  GPoint minute_point = {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)150 / TRIG_MAX_RATIO) + 72,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)150 / TRIG_MAX_RATIO) + 84,
  };

  // select proper path
  if(t->tm_min<6)
  {
    time_path = gpath_create(&QUINT1_PATH_INFO);
  }
  else if(t->tm_min<23)
  {
    time_path = gpath_create(&QUINT2_PATH_INFO);
  }
  else if(t->tm_min<36)
  {
    time_path = gpath_create(&QUINT3_PATH_INFO);
  }
  else if(t->tm_min<54)
  {
    time_path = gpath_create(&QUINT4_PATH_INFO);
  }
  else
  {
    time_path = gpath_create(&QUINT5_PATH_INFO);
  }
  time_path->points[time_path->num_points-1] = minute_point;
  
  // draw path
  graphics_context_set_fill_color(ctx,GColorWhite);
  gpath_draw_filled(ctx,time_path);
  
  // draw custom bitmap on top
  uint8_t hour = t->tm_hour;
  if(!clock_is_24h_style())
  {
    hour = hour%12;
    if(hour==0)
      hour=12;
  }
  switch (hour) {
    case 0:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_0);
      break;
    case 1:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_1);
      break;
    case 2:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_2);
      break;
    case 3:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_3);
      break;
    case 4:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_4);
      break;
    case 5:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_5);
      break;
    case 6:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_6);
      break;
    case 7:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_7);
      break;
    case 8:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_8);
      break;
    case 9:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_9);
      break;
    case 10:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_10);
      break;
    case 11:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_11);
      break;
    case 12:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_12);
      break;
    case 13:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_13);
      break;
    case 14:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_14);
      break;
    case 15:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_15);
      break;
    case 16:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_16);
      break;
    case 17:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_17);
      break;
    case 18:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_18);
      break;
    case 19:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_19);
      break;
    case 20:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_20);
      break;
    case 21:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_21);
      break;
    case 22:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_22);
      break;
    case 23:
      number_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_23);
      break;
    default:
      break;
  }
  
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  BitmapInfo bitmap_info;
  bitmap_info.bitmap_data =  gbitmap_get_data(fb);
  bitmap_info.bytes_per_row = gbitmap_get_bytes_per_row(fb);
  bitmap_info.bitmap_format = gbitmap_get_format(fb);
  
  BitmapInfo bg_bitmap_info;
  bg_bitmap_info.bytes_per_row = gbitmap_get_bytes_per_row(number_bitmap);
  bg_bitmap_info.bitmap_data =  gbitmap_get_data(number_bitmap);
  bg_bitmap_info.bitmap_format = gbitmap_get_format(number_bitmap);
  
  for (int y=0; y<168; y++) {
    for (int x=0; x<144; x++) {
      // we only want to set black pixels in the bitmap
      uint8_t bmp_pixel = get_pixel(bg_bitmap_info, y, x);
      if(bmp_pixel==0)
      {
        uint8_t fb_pixel = get_pixel(bitmap_info, y, x);
#ifdef PBL_COLOR
        set_pixel(bitmap_info, y, x, gcolor_equal((GColor8)fb_pixel,GColorWhite)?GColorYellow.argb:GColorDarkGreen.argb);
#else
        set_pixel(bitmap_info, y, x, fb_pixel? 0:1);
#endif
      }
#ifdef PBL_COLOR
      else
      {
        uint8_t fb_pixel = get_pixel(bitmap_info, y, x);
        set_pixel(bitmap_info, y, x, gcolor_equal((GColor8)fb_pixel,GColorWhite)?GColorElectricBlue.argb:GColorBrilliantRose.argb);
      }
#endif
    }
  }
  
  // release things
  graphics_release_frame_buffer(ctx, fb);
  gbitmap_destroy(number_bitmap);
  gpath_destroy(time_path);
}


static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(background_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // init background
  background_layer = layer_create(bounds);
  layer_set_update_proc(background_layer, background_update_proc);
  layer_add_child(window_layer, background_layer);
  
  // force update
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_tick(t, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

static void window_unload(Window *window) {
  layer_destroy(background_layer);
  tick_timer_service_unsubscribe();
}

static void init(void) {
  
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
#ifdef PBL_SDK_2
  window_set_fullscreen(window,true);
#endif
  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);
  
}

static void deinit(void) {
  window_destroy(window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
