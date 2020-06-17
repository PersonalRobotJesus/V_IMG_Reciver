/*
This example demonstrates the transfer of image and data between V and your device.
This example uses dual core for acceleration.
*/
#include <m5stack.h>
#include "v_interface.h"

// parameters to core0 function.
uint8_t *jpeg;
uint32_t jpeg_size;
bbox_buffer_t bboxes;

// creat V interface class
vInterface V;

// function on core 0
typedef int (*parallel_process_t)(int);
volatile parallel_process_t parallel_process = 0;
void core0(void *para)
{
    while (1)
    {
        if (parallel_process)
        {
            (*parallel_process)(1);
            parallel_process = 0;
        }
    }
}

// update image and box to screen
int Display(int ps)
{
    char buf[20];
    M5.Lcd.drawJpg(jpeg, jpeg_size, 0, 0, 320, 240);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextSize(2);
    for (int i = 0; i < bboxes.size(); i++)
    {
        sprintf(buf, "%d %.2f", bboxes[i].c, bboxes[i].prob);
        M5.Lcd.drawRect(bboxes[i].x1, bboxes[i].y1, bboxes[i].x2 - bboxes[i].x1, bboxes[i].y2 - bboxes[i].y1, TFT_GREEN);
        M5.Lcd.drawString(buf, bboxes[i].x1, bboxes[i].y1);
    }
    return 0;
}

void setup()
{
    disableCore0WDT();
    M5.begin();
    V.begin(21, 22);

    M5.Lcd.fillScreen(TFT_ORANGE);
    xTaskCreatePinnedToCore(core0, "core0", 32 * 1024, NULL, 1, NULL, 0);
}

void loop()
{
    uint32_t start = micros();

    int ret = V.snapeshot(); // Get data from V
    if (ret == 0)            // No error occurs
    {
        v_jpeg_t v_jpeg = V.getJpeg(); // Get jpeg data.
                                       // If you calling this function when snapeshot() is working,
                                       // this function will be blocked.
        bboxes = V.getBboxes();        // Get bboxes, Same as above.
        while (parallel_process)
            ; // Wait for core0 to complete.

        jpeg = (uint8_t *)realloc(jpeg, v_jpeg.size); // Creat new buffer to store jpeg data.
                                                      // Avoid data errors between the two cores.
        memcpy(jpeg, v_jpeg.data, v_jpeg.size);
        jpeg_size = v_jpeg.size;
        parallel_process = &Display; // Pass the function pointer to core0.
    }
#ifdef M5LogTrace
    M5LogTrace("Total = %lu us\n", micros() - start);
#else
    Serial.printf("Total = %lu us\n", micros() - start);
#endif
}