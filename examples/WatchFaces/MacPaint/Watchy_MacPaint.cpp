#include "Watchy_MacPaint.h"

const unsigned char *numbers [10] = {numbers0, numbers1, numbers2, numbers3, numbers4, numbers5, numbers6, numbers7, numbers8, numbers9};

uint8_t WatchyMacPaint::batteryPercentage(float voltage) {
    static const float volts[] = {3.40, 3.55, 3.65, 3.75, 3.85, 3.95, 4.05, 4.20};
    static const uint8_t percentages[] = {0, 5, 10, 25, 50, 70, 85, 100};

    if (voltage <= volts[0]) return percentages[0];
    if (voltage >= volts[7]) return percentages[7];

    for (uint8_t i = 1; i < 8; i++) {
        if (voltage <= volts[i]) {
            float position = (voltage - volts[i - 1]) / (volts[i] - volts[i - 1]);
            return percentages[i - 1] +
                   position * (percentages[i] - percentages[i - 1]);
        }
    }
    return 0;
}

void WatchyMacPaint::drawBattery() {
    const uint8_t percent = batteryPercentage(getBatteryVoltage());
    const int16_t x = 169;
    const int16_t y = 170;

    // Clear a section of MacPaint's status bar for the battery gauge.
    display.fillRect(x - 2, y - 2, 31, 17, GxEPD_WHITE);
    display.drawRect(x, y, 25, 11, GxEPD_BLACK);
    display.fillRect(x + 25, y + 3, 2, 5, GxEPD_BLACK);

    const uint8_t fillWidth = (percent * 21) / 100;
    if (fillWidth > 0) {
        display.fillRect(x + 2, y + 2, fillWidth, 7, GxEPD_BLACK);
    }
}

void WatchyMacPaint::drawWatchFace(){
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, window, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK);

    //Hour
    display.drawBitmap(35, 70, numbers[currentTime.Hour/10], 38, 50, GxEPD_BLACK); //first digit
    display.drawBitmap(70, 70, numbers[currentTime.Hour%10], 38, 50, GxEPD_BLACK); //second digit

    //Colon
    display.drawBitmap(100, 80, colon, 11, 31, GxEPD_BLACK); //second digit

    //Minute
    display.drawBitmap(115, 70, numbers[currentTime.Minute/10], 38, 50, GxEPD_BLACK); //first digit
    display.drawBitmap(153, 70, numbers[currentTime.Minute%10], 38, 50, GxEPD_BLACK); //second digit

    drawBattery();
}
