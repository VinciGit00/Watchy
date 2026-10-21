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

void WatchyMacPaint::showWatchFaceSelector() {
    guiState = APP_STATE;
    pinMode(MENU_BTN_PIN, INPUT);
    pinMode(BACK_BTN_PIN, INPUT);
    pinMode(UP_BTN_PIN, INPUT);
    pinMode(DOWN_BTN_PIN, INPUT);
    uint8_t selected = getSelectedWatchFace();

    auto isPressed = [](int pin) { return digitalRead(pin) == HIGH; };
    auto waitForRelease = [&]() {
        while (isPressed(MENU_BTN_PIN) || isPressed(BACK_BTN_PIN) ||
               isPressed(UP_BTN_PIN) || isPressed(DOWN_BTN_PIN)) delay(20);
        delay(100);
    };
    auto drawSelector = [&]() {
        display.setFullWindow();
        display.fillScreen(GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(8, 24);
        display.print("SET WATCHFACE");
        display.drawFastHLine(0, 34, 200, GxEPD_WHITE);
        for (uint8_t i = 0; i < 3; i++) {
            int y = 67 + i * 34;
            if (i == selected) {
                display.fillRect(3, y - 20, 194, 27, GxEPD_WHITE);
                display.setTextColor(GxEPD_BLACK);
                display.setCursor(10, y);
                display.print("> ");
            } else {
                display.setTextColor(GxEPD_WHITE);
                display.setCursor(10, y);
                display.print("  ");
            }
            display.print(getWatchFaceName(i));
        }
        display.setFont();
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(8, 181);
        display.print("RIGHT BUTTONS: MOVE");
        display.setCursor(8, 194);
        display.print("BOTTOM LEFT: OK");
        display.display(false);
    };

    drawSelector();
    waitForRelease();
    unsigned long lastInput = millis();
    while (millis() - lastInput < 60000UL) {
        if (isPressed(MENU_BTN_PIN)) {
            waitForRelease();
            showMenu(menuIndex, false);
            return;
        }
        if (isPressed(BACK_BTN_PIN)) {
            selectWatchFace(selected);
            waitForRelease();
            RTC.read(currentTime);
            display.setFullWindow();
            drawWatchFacePreview(selected);
            display.display(false);
            guiState = WATCHFACE_STATE;
            return;
        }
        if (isPressed(UP_BTN_PIN)) {
            selected = selected == 0 ? 2 : selected - 1;
            lastInput = millis();
            drawSelector();
            waitForRelease();
        } else if (isPressed(DOWN_BTN_PIN)) {
            selected = (selected + 1) % 3;
            lastInput = millis();
            drawSelector();
            waitForRelease();
        }
        delay(20);
    }
    showMenu(menuIndex, false);
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

void WatchyMacPaint::drawMiniDigit(uint8_t digit, int16_t x, int16_t y) {
    const unsigned char *bitmap = numbers[digit % 10];
    const uint8_t sourceRowBytes = 5;
    for (uint8_t targetY = 0; targetY < 17; targetY++) {
        for (uint8_t targetX = 0; targetX < 13; targetX++) {
            bool ink = false;
            for (uint8_t offsetY = 0; offsetY < 3 && !ink; offsetY++) {
                uint8_t sourceY = targetY * 3 + offsetY;
                if (sourceY >= 50) continue;
                for (uint8_t offsetX = 0; offsetX < 3; offsetX++) {
                    uint8_t sourceX = targetX * 3 + offsetX;
                    if (sourceX >= 38) continue;
                    uint8_t value = pgm_read_byte(bitmap + sourceY * sourceRowBytes + sourceX / 8);
                    if (value & (0x80 >> (sourceX & 7))) {
                        ink = true;
                        break;
                    }
                }
            }
            if (ink) display.drawPixel(x + targetX, y + targetY, GxEPD_BLACK);
        }
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
    const uint16_t year = tmYearToCalendar(currentTime.Year);
    const uint8_t dateDigits[] = {
        (uint8_t)(currentTime.Day / 10), (uint8_t)(currentTime.Day % 10),
        (uint8_t)(currentTime.Month / 10), (uint8_t)(currentTime.Month % 10),
        (uint8_t)((year / 1000) % 10), (uint8_t)((year / 100) % 10),
        (uint8_t)((year / 10) % 10), (uint8_t)(year % 10)};
    int16_t dateX = 49;
    for (uint8_t i = 0; i < 8; i++) {
        drawMiniDigit(dateDigits[i], dateX, 128);
        dateX += 14;
        if (i == 1 || i == 3) {
            display.drawLine(dateX, 142, dateX + 5, 129, GxEPD_BLACK);
            dateX += 8;
        }
    }

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
