#include "v_interface.h"

HardwareSerial VSerial(1);

static const uint8_t HEAD[8] = {0xA5, 0x5A, 0xAA, 0x55, 0xAB, 0xCD, 0xEF, 0xFE};

vInterface::vInterface()
{

}

int vInterface::begin(uint8_t rx, uint8_t tx)
{
    VSerial.begin(1500000, SERIAL_8N1, rx, tx);
    VSerial.setRxBufferSize(96 * 1024);
    VSerial.flush();
}

int vInterface::syncFrame(uint32_t timeout)
{
    uint8_t state = 0;
    uint32_t start_time = millis();

    // M5LogTrace("Wait for processd = %d\n", VSerial.available());
    // VSerial.flush();
    while (1)
    {
        if (VSerial.available())
        {
            uint8_t b = VSerial.read();
            if (HEAD[state] == b)
            {
                state++;
                if (state == 8)
                {
                    return 0;
                }
            }
            else
            {
                state = 0;
            }
        }

        if (millis() - start_time > timeout)
        {
            VSerial.flush();
            return -1;
        }
    }
}

int vInterface::snapeshot(void)
{
    if(syncFrame(5000) == -1)
    {
        #ifdef M5LogWarn
        M5LogWarn("syncFrame time out.\n");
        #else
        log_e("syncFrame time out.\n");
        #endif
        return -1;
    }

    int32_t i;
    uint8_t buffer[8];
    VSerial.readBytes(buffer, 8);
    uint64_t frame = *((uint64_t*)buffer);
    VSerial.readBytes(buffer, 2);
    uint16_t boxnum = *((uint16_t*)buffer);
    bbox_buffer_t bboxes;

    if(boxnum < 255)
    {
        bboxes.resize(boxnum);
    }
    else
    {
        #ifdef M5LogWarn
        M5LogWarn("Too many bboxes.\n");
        #else
        log_e("Too many bboxes.\n");
        #endif
        return -2;
    }
    for(i = 0; i < boxnum; i++)
    {
        VSerial.readBytes(buffer, 2);
        bboxes[i].x1 = *((uint16_t*)buffer);
        VSerial.readBytes(buffer, 2);
        bboxes[i].y1 = *((uint16_t*)buffer);
        VSerial.readBytes(buffer, 2);
        bboxes[i].x2 = *((uint16_t*)buffer);
        VSerial.readBytes(buffer, 2);
        bboxes[i].y2 = *((uint16_t*)buffer);
        VSerial.readBytes(buffer, 2);
        bboxes[i].c = *((uint16_t*)buffer);
        VSerial.readBytes(buffer, 4);
        bboxes[i].prob = *((float*)buffer);
        // M5LogInfo("bbox (%d, %d, %d, %d) [%d] %.2f\n", bboxes[i].x1, bboxes[i].y1, bboxes[i].x2, bboxes[i].y2, bboxes[i].c, bboxes[i].prob);
    }

    this->_avaliable = false;
    VSerial.readBytes(buffer, 4);
    this->_jpeg_size = *((uint32_t*)buffer);

    // M5LogInfo("frame = %llu, bbox num = %u, jpeg size = %lu\n", frame, boxnum, this->_jpeg_size);
    if ((this->_jpeg_size) > (32 * 1024))
    {
        #ifdef M5LogWarn
        M5LogWarn("The image is larger than 32 Kib.\n");
        #else
        log_e("The image is larger than 32 Kib.\n");
        #endif
        return -3;
    }
    VSerial.readBytes(buffer, 2);
    if(buffer[0] != 0xFF || buffer[1] != 0xD8)
    {
        #ifdef M5LogWarn
        M5LogWarn("Invalid jpeg format.\n");
        #else
        log_e("Invalid jpeg format.\n");
        #endif
        return -4;
    }
    this->_jpeg_buffer = (uint8_t*)realloc(this->_jpeg_buffer, this->_jpeg_size);
    VSerial.readBytes(this->_jpeg_buffer + 2, (this->_jpeg_size) - 2);

    *(this->_jpeg_buffer) = 0xFF;
    *(this->_jpeg_buffer + 1) = 0xD8;

    this->_bboxes = bboxes;
    this->_avaliable = true;
    return 0;
}

bool vInterface::available(void)
{
    return this->_avaliable;
}

bbox_buffer_t vInterface::getBboxes(void)
{
    while(this->_avaliable == false);
    return this->_bboxes;
}

v_jpeg_t vInterface::getJpeg(void)
{
    v_jpeg_t ret;
    while(this->_avaliable == false);
    ret.data = this->_jpeg_buffer;
    ret.size = this->_jpeg_size;
    ret.w = 320;
    ret.h = 240;
    return ret;
}