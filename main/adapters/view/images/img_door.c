
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#elif defined(LV_BUILD_TEST)
#include "../lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_DUST
#define LV_ATTRIBUTE_IMG_DUST
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_DUST
uint8_t img_door_map[] = {

    0x4d,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfa,0x30,0x00,0x00,0x00,0x00,
    0xdf,0xff,0xfe,0x98,0x88,0x88,0x88,0x88,0x8b,0xff,0xd1,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xe8,0x31,0x00,0x00,0x00,0x00,0x1c,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xec,0x50,0x00,0x00,0x00,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xfe,0x73,0x00,0x00,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xa1,0x00,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,0x10,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x02,0x20,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x3f,0xe8,0x10,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x02,0x63,0x4f,0xff,0xa0,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x11,0x11,0x11,0x3d,0xff,0xfa,0x10,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x19,0xdd,0xdd,0xde,0xff,0xff,0x70,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x3f,0xff,0xff,0xff,0xff,0xff,0xf0,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x29,0xff,0xff,0xff,0xff,0xff,0x70,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x00,0x00,0x3e,0xff,0xf7,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x02,0x63,0x4f,0xff,0x60,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x2c,0xd7,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x01,0x20,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x09,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x10,0x1b,0xf7,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xa9,0xcf,0xf3,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0x80,0x00,0x00,0x00,0x00,
    0x4c,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x76,0x62,0x00,0x00,0x00,0x00,0x00,
    0x00,0x6a,0xef,0xff,0xff,0xff,0xff,0xfd,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x26,0xcf,0xff,0xff,0xff,0xfc,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x06,0xbe,0xff,0xff,0xf7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x12,0x7c,0xff,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

};

const lv_image_dsc_t img_door = {
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.cf = LV_COLOR_FORMAT_A4,
  .header.flags = 0,
  .header.w = 29,
  .header.h = 32,
  .header.stride = 15,
  .data_size = sizeof(img_door_map),
  .data = img_door_map,
};

