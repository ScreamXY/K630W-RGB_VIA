/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/* Deliberately no default USB port. Enumerate first, then bind explicitly. */
static uint32_t k630_location(void) {
    const char *value = getenv("K630_USB_LOCATION");
    if (!value || !*value || *value == '-' || *value == '+') {
        return 0;
    }
    char *end;
    errno = 0;
    unsigned long location = strtoul(value, &end, 0);
    if (errno || *end || !location || location > UINT32_MAX) {
        return 0;
    }
    return (uint32_t)location;
}
