#pragma once

#define BATTERY_EMPTY_MV 3300
#define BATTERY_FULL_MV 4200
// Above this the divider is reading the USB rail, not a LiPo cell.
#define BATTERY_USB_MV 4400

static inline int battery_percent_from_mv(int mv)
{
    if (mv <= BATTERY_EMPTY_MV) {
        return 0;
    }
    if (mv >= BATTERY_FULL_MV) {
        return 100;
    }
    return (mv - BATTERY_EMPTY_MV) * 100 / (BATTERY_FULL_MV - BATTERY_EMPTY_MV);
}
