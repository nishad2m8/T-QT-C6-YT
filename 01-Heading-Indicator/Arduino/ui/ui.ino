#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include "lvgl.h"
#include "ui.h"
#include "mock_data.h"

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;

// N085-1212TBWIG06-C08
Arduino_DataBus *bus = new Arduino_HWSPI(
  LCD_DC /* DC */, LCD_CS /* CS */, LCD_SCLK /* SCK */, LCD_MOSI /* MOSI */, -1 /* MISO */);  // Software SPI

Arduino_GFX *gfx = new Arduino_GC9107(
  bus, LCD_RST /* RST */, 0 /* rotation */, true /* IPS */,
  LCD_WIDTH /* width */, LCD_HEIGHT /* height */,
  2 /* col offset 1 */, 1 /* row offset 1 */, 0 /* col_offset2 */, 0 /* row_offset2 */);

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
  std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

void Arduino_IIC_Touch_Interrupt(void);

std::unique_ptr<Arduino_IIC> CST816T(new Arduino_CST816x(IIC_Bus, CST816T_DEVICE_ADDRESS,
                                                         TP_RST, TP_INT, Arduino_IIC_Touch_Interrupt));
std::unique_ptr<Arduino_IIC> ETA4662(new Arduino_ETA4662(IIC_Bus, ETA4662_DEVICE_ADDRESS,
                                                         DRIVEBUS_DEFAULT_VALUE, DRIVEBUS_DEFAULT_VALUE));
std::unique_ptr<Arduino_IIC> LSM6DSL(new Arduino_LSM6DSL(IIC_Bus, LSM6DSL_DEVICE_ADDRESS,
                                                         DRIVEBUS_DEFAULT_VALUE, DRIVEBUS_DEFAULT_VALUE));

void Arduino_IIC_Touch_Interrupt(void) {
  CST816T->IIC_Interrupt_Flag = true;
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  if (CST816T->IIC_Interrupt_Flag == true) {
    CST816T->IIC_Interrupt_Flag = false;

    if ((uint32_t)CST816T->IIC_Read_Device_Value(CST816T->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X) == 60 && (uint32_t)CST816T->IIC_Read_Device_Value(CST816T->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y) == 150) {
    } else {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = (uint32_t)CST816T->IIC_Read_Device_Value(CST816T->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
      data->point.y = (uint32_t)CST816T->IIC_Read_Device_Value(CST816T->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);

      // Serial.printf("Fingers Number:%d\n",
      //             (uint32_t)CST816T->IIC_Read_Device_Value(CST816T->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER));
      // Serial.printf("Touch X:%d Y:%d\n",
      //             (uint32_t)CST816T->IIC_Read_Device_Value(CST816T->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X),
      //             (uint32_t)CST816T->IIC_Read_Device_Value(CST816T->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y));
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void lvgl_initialization(void) {
  lv_init();

  // Get the width and height from the gfx object
  uint16_t lcdWidth = gfx->width();
  uint16_t lcdHeight = gfx->height();

  // Allocate the display buffer
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * lcdWidth * 40, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  // Check for memory allocation success
  while (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
    delay(1000);
  }

  // Initialize the display buffer with the actual width
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, lcdWidth * 40);

  /* Initialize the display */
  lv_disp_drv_init(&disp_drv);
  /* Set the display resolution */
  disp_drv.hor_res = lcdWidth;
  disp_drv.ver_res = lcdHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* Initialize the (dummy) input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // Initialize the UI using ui.h from Squareline Studio
  ui_init();
}


void setup() {
  Serial.begin(115200);
  Serial.println("Initialization Started...");

  // Screen backlight
  pinMode(LCD_BL, OUTPUT);
  ledcAttach(LCD_BL, 20000, 8);
  ledcWrite(LCD_BL, 0);  // Turn on screen

  gfx->begin();
  gfx->fillScreen(BLACK);

  if (CST816T->begin() == false) {
    Serial.println("CST816T initialization fail");
    delay(2000);
  } else {
    Serial.println("CST816T initialization successfully");
  }

  // When a touch state change is detected, a low pulse is issued
  CST816T->IIC_Write_Device_State(CST816T->Arduino_IIC_Touch::Device::TOUCH_DEVICE_INTERRUPT_MODE,
                                  CST816T->Arduino_IIC_Touch::Device_Mode::TOUCH_DEVICE_INTERRUPT_PERIODIC);

  lvgl_initialization();
}

void loop() {
  lv_timer_handler();  // Let the GUI do its work
  updateMockData();  // Call mock data update to simulate compass and altitude data
  delay(100);  // Update every 100ms for smoother transitions
}