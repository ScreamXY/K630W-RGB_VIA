/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Enumerate candidate interfaces only. No device opens, reports or serials. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int number(IOHIDDeviceRef d, CFStringRef key) {
    int value = 0;
    CFTypeRef p = IOHIDDeviceGetProperty(d, key);
    if (p && CFGetTypeID(p) == CFNumberGetTypeID()) {
        CFNumberGetValue(p, kCFNumberIntType, &value);
    }
    return value;
}

int main(void) {
    IOHIDManagerRef manager = IOHIDManagerCreate(NULL, 0);
    if (!manager) {
        return 1;
    }
    IOHIDManagerSetDeviceMatching(manager, NULL);
    CFSetRef devices = IOHIDManagerCopyDevices(manager);
    unsigned found = 0;
    if (devices) {
        CFIndex count = CFSetGetCount(devices);
        const void **items = calloc((size_t)count, sizeof(*items));
        if (!items && count) {
            CFRelease(devices);
            CFRelease(manager);
            return 1;
        }
        CFSetGetValues(devices, items);
        for (CFIndex i = 0; i < count; i++) {
            IOHIDDeviceRef d = (IOHIDDeviceRef)items[i];
            int vid = number(d, CFSTR(kIOHIDVendorIDKey));
            int pid = number(d, CFSTR(kIOHIDProductIDKey));
            const char *mode = vid == 0x320f && pid == 0x5000 ? "stock candidate" :
                vid == 0x0c45 && pid == 0x7687 ? "Evision bootloader candidate" :
                vid == 0x5244 && pid == 0x0630 ? "K630 QMK/VIA" : NULL;
            if (!mode) {
                continue;
            }
            printf("%04x:%04x  location=0x%08x  %s\n", vid, pid,
                   (uint32_t)number(d, CFSTR(kIOHIDLocationIDKey)), mode);
            found++;
        }
        free(items);
        CFRelease(devices);
    }
    CFRelease(manager);
    puts("Multiple rows at one location can be interfaces of the same keyboard. USB IDs alone do not identify its MCU.");
    return found ? 0 : 1;
}
