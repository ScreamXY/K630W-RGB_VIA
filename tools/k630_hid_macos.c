// SPDX-License-Identifier: GPL-2.0-or-later
// Narrow macOS transport for the reviewed Evision flasher. No CLI entry point.
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "device_location.h"

static IOHIDManagerRef manager;
static IOHIDDeviceRef device;
static int opened_mode;

static int number(IOHIDDeviceRef d, CFStringRef key) {
    int v = 0;
    CFTypeRef p = IOHIDDeviceGetProperty(d, key);
    if (p && CFGetTypeID(p) == CFNumberGetTypeID()) {
        CFNumberGetValue(p, kCFNumberIntType, &v);
    }
    return v;
}

static int has_usage(IOHIDDeviceRef d, int page, int usage) {
    CFTypeRef p = IOHIDDeviceGetProperty(d, CFSTR(kIOHIDDeviceUsagePairsKey));
    if (!p || CFGetTypeID(p) != CFArrayGetTypeID()) {
        return 0;
    }
    CFArrayRef pairs = p;
    for (CFIndex i = 0; i < CFArrayGetCount(pairs); i++) {
        CFDictionaryRef pair = CFArrayGetValueAtIndex(pairs, i);
        if (CFGetTypeID(pair) != CFDictionaryGetTypeID()) {
            continue;
        }
        CFTypeRef a = CFDictionaryGetValue(pair, CFSTR(kIOHIDDeviceUsagePageKey));
        CFTypeRef b = CFDictionaryGetValue(pair, CFSTR(kIOHIDDeviceUsageKey));
        int av = 0, bv = 0;
        if (a && CFGetTypeID(a) == CFNumberGetTypeID()) {
            CFNumberGetValue(a, kCFNumberIntType, &av);
        }
        if (b && CFGetTypeID(b) == CFNumberGetTypeID()) {
            CFNumberGetValue(b, kCFNumberIntType, &bv);
        }
        if (av == page && (usage < 0 || bv == usage)) {
            return 1;
        }
    }
    return 0;
}

static int matches(IOHIDDeviceRef d, int mode) {
    if (!k630_location() || (uint32_t)number(d, CFSTR(kIOHIDLocationIDKey)) != k630_location()) {
        return 0;
    }
    if (mode == 1) {
        return has_usage(d, 0xff1c, 0x92) && number(d, CFSTR(kIOHIDMaxOutputReportSizeKey)) == 64;
    }
    if (mode == 2) {
        return has_usage(d, 0x0c, -1) && number(d, CFSTR(kIOHIDMaxFeatureReportSizeKey)) == 64;
    }
    if (mode == 3) {
        return has_usage(d, 0xff60, 0x61) && number(d, CFSTR(kIOHIDMaxInputReportSizeKey)) == 32;
    }
    return 0;
}

// Return 1 for exactly one matching interface at the original physical port.
static int find(int mode, int keep) {
    if (mode < 1 || mode > 3 || (keep && device)) {
        return -1;
    }
    int vids[] = {0, 0x320f, 0x0c45, 0x5244};
    int pids[] = {0, 0x5000, 0x7687, 0x0630};
    IOHIDManagerRef m = IOHIDManagerCreate(NULL, 0);
    if (!m) {
        return -1;
    }
    CFNumberRef v = CFNumberCreate(NULL, kCFNumberIntType, &vids[mode]);
    CFNumberRef p = CFNumberCreate(NULL, kCFNumberIntType, &pids[mode]);
    const void *keys[] = {CFSTR(kIOHIDVendorIDKey), CFSTR(kIOHIDProductIDKey)};
    const void *vals[] = {v, p};
    CFDictionaryRef match = CFDictionaryCreate(NULL, keys, vals, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    IOHIDManagerSetDeviceMatching(m, match);
    CFRelease(match); CFRelease(v); CFRelease(p);
    CFSetRef ds = IOHIDManagerCopyDevices(m);
    IOHIDDeviceRef found = NULL;
    int count = 0;
    if (ds) {
        CFIndex n = CFSetGetCount(ds);
        const void **items = calloc((size_t)n, sizeof(*items));
        if (!items && n) {
            CFRelease(ds);
            CFRelease(m);
            return -1;
        }
        CFSetGetValues(ds, items);
        for (CFIndex i = 0; i < n; i++) {
            IOHIDDeviceRef d = (IOHIDDeviceRef)items[i];
            if (matches(d, mode)) {
                count++;
                found = d;
            }
        }
        free(items);
    }
    int result = count > 1 ? -1 : count;
    if (keep && count == 1) {
        IOReturn s = IOHIDDeviceOpen(found, kIOHIDOptionsTypeNone);
        if (s == kIOReturnSuccess) {
            device = (IOHIDDeviceRef)CFRetain(found);
            manager = m;
            opened_mode = mode;
        } else {
            result = (int)s;
        }
    }
    if (ds) {
        CFRelease(ds);
    }
    if (m != manager) {
        CFRelease(m);
    }
    return result;
}

int k630_present(int mode) { return find(mode, 0); }
int k630_open(int mode) { return find(mode, 1); }

void k630_close(void) {
    if (device) {
        IOHIDDeviceClose(device, kIOHIDOptionsTypeNone);
        CFRelease(device);
        device = NULL;
    }
    if (manager) {
        CFRelease(manager);
        manager = NULL;
    }
    opened_mode = 0;
}

int k630_feature_write(const uint8_t *data, int length) {
    if (!device || opened_mode != 2 || length != 64) {
        return -1;
    }
    return (int)IOHIDDeviceSetReport(device, kIOHIDReportTypeFeature, 0, data, length);
}

int k630_feature_read(uint8_t *data, int length) {
    if (!device || opened_mode != 2 || length != 64) {
        return -1;
    }
    CFIndex n = length;
    IOReturn s = IOHIDDeviceGetReport(device, kIOHIDReportTypeFeature, 0, data, &n);
    return s == kIOReturnSuccess ? (n == 64 ? 0 : -2) : (int)s;
}

int k630_enter(const uint8_t *data, int length) {
    static const uint8_t command[64] = {4, 0xee, 0xee};
    if (!device || opened_mode != 1 || length != 64 || memcmp(data, command, 64)) {
        return -1;
    }
    return (int)IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 4, data, length);
}
