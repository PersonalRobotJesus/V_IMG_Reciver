#ifndef _V_INTERFACE_H_
#define _V_INTERFACE_H_

#include <m5stack.h>
#include <vector>

typedef struct
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
    uint16_t c;
    float prob;
}bbox_t;

typedef struct
{
    uint8_t* data;
    uint32_t size;
    uint16_t w;
    uint16_t h;
}v_jpeg_t;

typedef std::vector<bbox_t> bbox_buffer_t;

class vInterface
{
    public:
        vInterface();
        int begin(uint8_t rx = 21, uint8_t tx = 22);
        int snapeshot(void);
        bool available(void);
        bbox_buffer_t getBboxes(void);
        v_jpeg_t getJpeg(void);

    private:
        bbox_buffer_t _bboxes;
        uint8_t* _jpeg_buffer = NULL;
        uint32_t _jpeg_size = 0;
        bool _avaliable = true;
        
    private:
        int syncFrame(uint32_t timeout);
};

#endif
