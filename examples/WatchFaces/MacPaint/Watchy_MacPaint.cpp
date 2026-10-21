#include "Watchy_MacPaint.h"

const unsigned char *numbers [10] = {numbers0, numbers1, numbers2, numbers3, numbers4, numbers5, numbers6, numbers7, numbers8, numbers9};
RTC_DATA_ATTR uint8_t selectedWatchFace = 255;
RTC_DATA_ATTR bool watchFaceInverted = false;
int8_t pipBoyAnimationFrame = -1;

void WatchyMacPaint::handleButtonPress() {
    uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
    if (guiState == WATCHFACE_STATE && (wakeupBit & BACK_BTN_MASK)) {
        pinMode(BACK_BTN_PIN, INPUT);
        while (digitalRead(BACK_BTN_PIN) == HIGH) delay(15);
        watchFaceInverted = !watchFaceInverted;
        RTC.read(currentTime);
        showWatchFace(false);
        return;
    }
    if (guiState == WATCHFACE_STATE && (wakeupBit & UP_BTN_MASK)) {
        pinMode(UP_BTN_PIN, INPUT);
        while (digitalRead(UP_BTN_PIN) == HIGH) delay(15);
        bool doubleClick = false;
        unsigned long deadline = millis() + 550;
        while ((long)(deadline - millis()) > 0) {
            if (digitalRead(UP_BTN_PIN) == HIGH) {
                doubleClick = true;
                while (digitalRead(UP_BTN_PIN) == HIGH) delay(15);
                break;
            }
            delay(15);
        }
        if (doubleClick && getSelectedWatchFace() == 1) {
            animatePipBoy();
            return;
        }
        return;
    }
    Watchy::handleButtonPress();
}

uint8_t WatchyMacPaint::getWatchFaceCount() { return 4; }

uint8_t WatchyMacPaint::getSelectedWatchFace() {
    if (selectedWatchFace > 3) {
        Preferences preferences;
        preferences.begin("watchface", true);
        selectedWatchFace = preferences.getUChar("selected", 0);
        preferences.end();
        if (selectedWatchFace > 3) selectedWatchFace = 0;
    }
    return selectedWatchFace;
}

const char *WatchyMacPaint::getWatchFaceName(uint8_t index) {
    static const char *names[] = {"MacPaint", "Pip-Boy", "DOS", "eatch"};
    return names[index < 4 ? index : 0];
}

void WatchyMacPaint::selectWatchFace(uint8_t index) {
    selectedWatchFace = index < 4 ? index : 0;
    Preferences preferences;
    preferences.begin("watchface", false);
    preferences.putUChar("selected", selectedWatchFace);
    preferences.end();
}

void WatchyMacPaint::drawWatchFacePreview(uint8_t index) {
    if (index == 1) drawPipBoy();
    else if (index == 2) drawDOS();
    else if (index == 3) drawEatch();
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
        for (uint8_t i = 0; i < 4; i++) {
            int y = 58 + i * 27;
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
            selectWatchFace(selected);
            waitForRelease();
            RTC.read(currentTime);
            display.setFullWindow();
            drawWatchFacePreview(selected);
            display.display(false);
            guiState = WATCHFACE_STATE;
            deepSleep();
            return;
        }
        if (isPressed(BACK_BTN_PIN)) {
            selectWatchFace(selected);
            waitForRelease();
            showMenu(menuIndex, false);
            return;
        }
        if (isPressed(UP_BTN_PIN)) {
            selected = selected == 0 ? 3 : selected - 1;
            lastInput = millis();
            drawSelector();
            waitForRelease();
        } else if (isPressed(DOWN_BTN_PIN)) {
            selected = (selected + 1) % 4;
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
    uint16_t foreground = watchFaceInverted ? GxEPD_WHITE : GxEPD_BLACK;
    uint16_t background = watchFaceInverted ? GxEPD_BLACK : GxEPD_WHITE;
    display.fillRect(x - 2, y - 2, 31, 17, background);
    display.drawRect(x, y, 25, 11, foreground);
    display.fillRect(x + 25, y + 3, 2, 5, foreground);

    const uint8_t fillWidth = (percent * 21) / 100;
    if (fillWidth > 0) {
        display.fillRect(x + 2, y + 2, fillWidth, 7, foreground);
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
            if (ink) display.drawPixel(x + targetX, y + targetY,
                                       watchFaceInverted ? GxEPD_WHITE : GxEPD_BLACK);
        }
    }
}

void WatchyMacPaint::drawWatchFace(){
    drawWatchFacePreview(getSelectedWatchFace());
}

void WatchyMacPaint::drawMacPaint(){
    uint16_t foreground = watchFaceInverted ? GxEPD_WHITE : GxEPD_BLACK;
    uint16_t background = watchFaceInverted ? GxEPD_BLACK : GxEPD_WHITE;
    display.fillScreen(background);
    display.drawBitmap(0, 0, window, DISPLAY_WIDTH, DISPLAY_HEIGHT, foreground);

    //Hour
    display.drawBitmap(35, 68, numbers[currentTime.Hour/10], 38, 50, foreground);
    display.drawBitmap(70, 68, numbers[currentTime.Hour%10], 38, 50, foreground);

    //Colon
    display.drawBitmap(100, 78, colon, 11, 31, foreground);

    //Minute
    display.drawBitmap(115, 68, numbers[currentTime.Minute/10], 38, 50, foreground);
    display.drawBitmap(153, 68, numbers[currentTime.Minute%10], 38, 50, foreground);

    // Date below the clock.
    const uint16_t year = tmYearToCalendar(currentTime.Year);
    const uint8_t dateDigits[] = {
        (uint8_t)(currentTime.Day / 10), (uint8_t)(currentTime.Day % 10),
        (uint8_t)(currentTime.Month / 10), (uint8_t)(currentTime.Month % 10),
        (uint8_t)((year / 1000) % 10), (uint8_t)((year / 100) % 10),
        (uint8_t)((year / 10) % 10), (uint8_t)(year % 10)};
    // The handwritten digit bitmaps have asymmetric transparent margins.
    // This offset centers their visible ink under the clock, not their boxes.
    int16_t dateX = 60;
    for (uint8_t i = 0; i < 8; i++) {
        drawMiniDigit(dateDigits[i], dateX, 126);
        dateX += 14;
        if (i == 1 || i == 3) {
            display.drawLine(dateX, 140, dateX + 5, 127, foreground);
            dateX += 8;
        }
    }

    drawBattery();
}

void WatchyMacPaint::drawPipBoy() {
    uint16_t background = watchFaceInverted ? GxEPD_WHITE : GxEPD_BLACK;
    uint16_t foreground = watchFaceInverted ? GxEPD_BLACK : GxEPD_WHITE;
    display.fillScreen(background);
    display.setTextColor(foreground);
    display.setFont(&monofonto8pt7b);
    display.setCursor(22, 14);
    display.print("STAT  INV  DATA  MAP");
    display.drawBitmap(0, 10, menubar, 200, 9, foreground);
    display.setCursor(10, 195);
    display.print("PIP-BOY 3000 ROBCO IND.");

    uint8_t pose = pipBoyAnimationFrame >= 0 ? pipBoyAnimationFrame
                                             : (currentTime.Minute / 5) % 3;
    switch (pose) {
    case 1: display.drawBitmap(70, 50, vaultboypoint, 57, 100, foreground); break;
    case 2: display.drawBitmap(60, 50, vaultboysmile, 67, 100, foreground); break;
    default: display.drawBitmap(70, 50, vaultboy, 57, 100, foreground); break;
    }
    display.drawLine(137, 28, 200, 28, foreground);
    display.drawLine(137, 28, 137, 132, foreground);
    display.drawLine(137, 132, 157, 132, foreground);
    display.drawLine(180, 132, 200, 132, foreground);
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
    display.drawBitmap(60, 155, gauge, 73, 10, foreground);
    display.fillRect(73, 160, progress / 2 + 5, 4, foreground);
    display.setFont(&monofonto8pt7b);
    display.setCursor(150, 160);
    display.print("STEPS");
    display.setCursor(150, 175);
    display.print(steps);

    display.drawBitmap(10, 150, battery, 37, 21, foreground);
    display.fillRect(15, 155, 27, 11, background);
    uint8_t percent = batteryPercentage(getBatteryVoltage());
    uint8_t segments = percent == 0 ? 0 : min((uint8_t)3, (uint8_t)((percent + 32) / 33));
    for (uint8_t i = 0; i < segments; i++)
        display.fillRect(15 + i * 9, 155, 7, 11, foreground);
}

void WatchyMacPaint::animatePipBoy() {
    unsigned long started = millis();
    uint8_t frame = 0;
    while (millis() - started < 20000UL) {
        pipBoyAnimationFrame = frame;
        display.setFullWindow();
        drawPipBoy();
        display.display(frame != 0);
        frame = (frame + 1) % 3;
        delay(150);
    }
    pipBoyAnimationFrame = -1;
    RTC.read(currentTime);
    display.setFullWindow();
    drawPipBoy();
    display.display(false);
    guiState = WATCHFACE_STATE;
    deepSleep();
}

void WatchyMacPaint::drawDOS() {
    char time[6];
    snprintf(time, sizeof(time), "%02d:%02d", currentTime.Hour,
             currentTime.Minute);
    display.fillScreen(watchFaceInverted ? GxEPD_WHITE : GxEPD_BLACK);
    display.setTextColor(watchFaceInverted ? GxEPD_BLACK : GxEPD_WHITE);
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

void WatchyMacPaint::drawEatch() {
    uint16_t background = watchFaceInverted ? GxEPD_BLACK : GxEPD_WHITE;
    uint16_t foreground = watchFaceInverted ? GxEPD_WHITE : GxEPD_BLACK;
    display.fillScreen(background);
    display.setTextColor(foreground);

    display.setFont(&DSEG7_Classic_Bold_53);
    display.setCursor(4, 58);
    if (currentTime.Hour < 10) display.print("0");
    display.print(currentTime.Hour);
    display.print(":");
    if (currentTime.Minute < 10) display.print("0");
    display.print(currentTime.Minute);

    weatherData weather = getWeatherData();
    display.setFont();
    display.setTextSize(1);
    display.setCursor(7, 72);
    display.print("SUN ");
    if (weather.sunrise.Hour < 24) {
        if (weather.sunrise.Hour < 10) display.print("0");
        display.print(weather.sunrise.Hour);
        display.print(":");
        if (weather.sunrise.Minute < 10) display.print("0");
        display.print(weather.sunrise.Minute);
    } else display.print("--:--");

    display.setFont(&FreeMonoBold9pt7b);
    String day = dayShortStr(currentTime.Wday);
    day.toUpperCase();
    display.setCursor(64, 91);
    display.print(day);
    String month = monthShortStr(currentTime.Month);
    month.toUpperCase();
    display.setCursor(6, 116);
    if (currentTime.Day < 10) display.print("0");
    display.print(currentTime.Day);
    display.print(" ");
    display.print(month);
    display.setCursor(6, 142);
    display.print(tmYearToCalendar(currentTime.Year));

    display.drawRect(111, 73, 84, 78, foreground);
    display.setFont();
    display.setCursor(116, 83);
    display.print("IDENTITY");
    for (int radius = 9; radius <= 29; radius += 5)
        display.drawCircle(153, 116, radius, foreground);
    display.fillRect(151, 87, 5, 59, background);
    for (int y = 91; y < 143; y += 7)
        display.drawLine(151, y, 179, y + 10, foreground);

    uint32_t steps = sensor.getCounter();
    display.setFont();
    display.setTextSize(2);
    display.setCursor(5, 181);
    char stepText[7];
    uint32_t shownSteps = steps > 99999U ? 99999U : steps;
    snprintf(stepText, sizeof(stepText), "%05lu", (unsigned long)shownSteps);
    display.print(stepText);

    display.setFont();
    display.setTextSize(1);
    display.setCursor(6, 195);
    display.print("STEPS");
    display.setCursor(115, 164);
    display.print(WIFI_CONFIGURED ? "WIFI" : "OFFLINE");

    uint8_t percent = batteryPercentage(getBatteryVoltage());
    display.drawRect(151, 173, 38, 17, foreground);
    display.fillRect(189, 178, 3, 7, foreground);
    display.fillRect(154, 176, (percent * 32) / 100, 11, foreground);
    display.setCursor(113, 194);
    display.print("BATTERY");
}
