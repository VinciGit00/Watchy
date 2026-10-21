#ifndef WATCHY_MACPAINT_H
#define WATCHY_MACPAINT_H

#include <Watchy.h>
#include <Preferences.h>
#include "macpaint.h"
#include "pipboy_icons.h"
#include "pipboy_img.h"
#include "monofonto8pt7b.h"
#include "monofonto10pt7b.h"
#include "monofonto28pt7b.h"
#include "Px437_IBM_BIOS5pt7b.h"

class WatchyMacPaint : public Watchy{
    using Watchy::Watchy;
    public:
        void drawWatchFace();
        void handleButtonPress();
        void showWatchFaceSelector();
        uint8_t getWatchFaceCount();
        uint8_t getSelectedWatchFace();
        const char *getWatchFaceName(uint8_t index);
        void drawWatchFacePreview(uint8_t index);
        void selectWatchFace(uint8_t index);
    private:
        void drawMacPaint();
        void drawPipBoy();
        void drawDOS();
        void drawEatch();
        void animatePipBoy();
        void drawMiniDigit(uint8_t digit, int16_t x, int16_t y);
        void drawBattery();
        uint8_t batteryPercentage(float voltage);
};

#endif
