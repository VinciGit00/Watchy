#include "Watchy_MacPaint.h"

const unsigned char *numbers [10] = {numbers0, numbers1, numbers2, numbers3, numbers4, numbers5, numbers6, numbers7, numbers8, numbers9};
RTC_DATA_ATTR uint8_t selectedWatchFace = 255;

uint8_t WatchyMacPaint::getWatchFaceCount() { return 3; }

uint8_t WatchyMacPaint::getSelectedWatchFace() {
    if (selectedWatchFace > 2) {
        Preferences preferences;
        preferences.begin("watchface", true);
        selectedWatchFace = preferences.getUChar("selected", 0);
        preferences.end();
        if (selectedWatchFace > 2) selectedWatchFace = 0;
    }
    return selectedWatchFace;
}

const char *WatchyMacPaint::getWatchFaceName(uint8_t index) {
    static const char *names[] = {"MacPaint", "Pip-Boy", "DOS"};
    return names[index < 3 ? index : 0];
}

void WatchyMacPaint::selectWatchFace(uint8_t index) {
    selectedWatchFace = index < 3 ? index : 0;
    Preferences preferences;
    preferences.begin("watchface", false);
    preferences.putUChar("selected", selectedWatchFace);
    preferences.end();
}

void WatchyMacPaint::drawWatchFacePreview(uint8_t index) {
    if (index == 1) drawPipBoy();
    else if (index == 2) drawDOS();
    else drawMacPaint();
}

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
    drawWatchFacePreview(getSelectedWatchFace());
}

void WatchyMacPaint::drawMacPaint(){
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

    // Date below the clock.
    char date[16];
    String month = monthShortStr(currentTime.Month);
    month.toUpperCase();
    snprintf(date, sizeof(date), "%02d %s %04d", currentTime.Day,
             month.c_str(), tmYearToCalendar(currentTime.Year));
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int16_t x1, y1;
    uint16_t width, height;
    display.getTextBounds(date, 0, 148, &x1, &y1, &width, &height);
    display.setCursor((200 - width) / 2, 148);
    display.print(date);

    drawBattery();
}

void WatchyMacPaint::drawPipBoy() {
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    display.setFont(&monofonto8pt7b);
    display.setCursor(22, 14);
    display.print("STAT  INV  DATA  MAP");
    display.drawBitmap(0, 10, menubar, 200, 9, GxEPD_WHITE);
    display.setCursor(10, 195);
    display.print("PIP-BOY 3000 ROBCO IND.");

    display.drawBitmap(70, 50, vaultboy, 57, 100, GxEPD_WHITE);
    display.drawLine(137, 28, 200, 28, GxEPD_WHITE);
    display.drawLine(137, 28, 137, 132, GxEPD_WHITE);
    display.drawLine(137, 132, 157, 132, GxEPD_WHITE);
    display.drawLine(180, 132, 200, 132, GxEPD_WHITE);
    display.setFont(&monofonto28pt7b);
    display.setCursor(141, 75);
    if (currentTime.Hour < 10) display.print("0");
    display.print(currentTime.Hour);
    display.setCursor(141, 125);
    if (currentTime.Minute < 10) display.print("0");
    display.print(currentTime.Minute);

    display.setFont(&monofonto10pt7b);
    display.setCursor(7, 42);
    display.print(dayShortStr(currentTime.Wday));
    display.setCursor(7, 62);
    display.print(monthShortStr(currentTime.Month));
    display.print(" ");
    display.print(currentTime.Day);
    display.setCursor(7, 78);
    display.print(tmYearToCalendar(currentTime.Year));

    uint32_t steps = sensor.getCounter();
    uint8_t progress = min(100U, (uint32_t)(steps * 100.0 / 5000));
    display.drawBitmap(60, 155, gauge, 73, 10, GxEPD_WHITE);
    display.fillRect(73, 160, progress / 2 + 5, 4, GxEPD_WHITE);
    display.setFont(&monofonto8pt7b);
    display.setCursor(150, 160);
    display.print("STEPS");
    display.setCursor(150, 175);
    display.print(steps);

    display.drawBitmap(10, 150, battery, 37, 21, GxEPD_WHITE);
    display.fillRect(15, 155, 27, 11, GxEPD_BLACK);
    uint8_t segments = batteryPercentage(getBatteryVoltage()) / 34;
    for (uint8_t i = 0; i < segments; i++)
        display.fillRect(15 + i * 9, 155, 7, 11, GxEPD_WHITE);
}

void WatchyMacPaint::drawDOS() {
    char time[6];
    snprintf(time, sizeof(time), "%02d:%02d", currentTime.Hour,
             currentTime.Minute);
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);
    display.setFont(&Px437_IBM_BIOS5pt7b);
    display.setCursor(0, 24);
    display.println("WATCHY-DOS 1.1.8");
    display.println("Copyright (c) 2020");
    display.println(" ");
    display.print("AUTOEXEC BAT "); display.println(time);
    display.print("COMMAND  COM "); display.println(time);
    display.print("CONFIG   SYS "); display.println(time);
    display.print("ESPTOOL  PY  "); display.println(time);
    display.println(" ");
    display.println("  4 files 563 bytes");
    display.println("  2048 bytes free");
    display.println(" ");
    display.println("<C:\\>esptool");
}
