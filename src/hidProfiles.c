#include <hidProfiles.h>

void classifyButtons(HidRecord *dev) {
    // TODO: Map the Buttons
}

void buildHIDMap(HidRecord *dev) {
    // Clear the maps on startup
    for (int i = 0; i < MAX_USAGES; i++) {
        dev->buttonMap[i] = -1;
        dev->axisMap[i] = -1;
    }

    classifyButtons(dev);
}