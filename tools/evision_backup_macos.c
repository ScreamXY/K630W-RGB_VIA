// SPDX-License-Identifier: GPL-2.0-or-later
// macOS transport for the read-only Evision stock flash protocol documented by
// mdmower/qmk-8051, commit c0d819da636af990611e874d2342f3b56e480455,
// util/evision_dump.py. This program has no bootloader or flash-write commands.
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { REPORT_LEN = 64, CHUNK = 48, FLASH_SIZE = 65536, MARKER = 0x27c0 };
static const uint8_t marker[] = {0xaa, 0x55, 0x33, 0xcc};
#include "device_location.h"

typedef struct {
    uint8_t request[REPORT_LEN];
    uint8_t input[REPORT_LEN];
    uint8_t reply[CHUNK];
    bool waiting, received;
} Reader;

static int number(IOHIDDeviceRef d, CFStringRef key) {
    int v = 0;
    CFTypeRef p = IOHIDDeviceGetProperty(d, key);
    if (p && CFGetTypeID(p) == CFNumberGetTypeID()) {
        CFNumberGetValue(p, kCFNumberIntType, &v);
    }
    return v;
}

static bool vendor_collection(IOHIDDeviceRef d) {
    CFTypeRef p = IOHIDDeviceGetProperty(d, CFSTR(kIOHIDDeviceUsagePairsKey));
    if (!p || CFGetTypeID(p) != CFArrayGetTypeID()) {
        return false;
    }
    CFArrayRef pairs = p;
    for (CFIndex i = 0; i < CFArrayGetCount(pairs); i++) {
        CFDictionaryRef pair = CFArrayGetValueAtIndex(pairs, i);
        if (CFGetTypeID(pair) != CFDictionaryGetTypeID()) {
            continue;
        }
        CFTypeRef page = CFDictionaryGetValue(pair, CFSTR(kIOHIDDeviceUsagePageKey));
        CFTypeRef usage = CFDictionaryGetValue(pair, CFSTR(kIOHIDDeviceUsageKey));
        int a = 0, b = 0;
        if (page && CFGetTypeID(page) == CFNumberGetTypeID()) {
            CFNumberGetValue(page, kCFNumberIntType, &a);
        }
        if (usage && CFGetTypeID(usage) == CFNumberGetTypeID()) {
            CFNumberGetValue(usage, kCFNumberIntType, &b);
        }
        if (a == 0xff1c && b == 0x92) {
            return true;
        }
    }
    return false;
}

static bool make_request(uint32_t addr, unsigned n, uint8_t r[REPORT_LEN]) {
    if (!n || n > CHUNK || addr >= FLASH_SIZE || n > FLASH_SIZE - addr) {
        return false;
    }
    uint16_t offset = (uint16_t)(addr - 0xa700);
    memset(r, 0, REPORT_LEN);
    r[0] = 4; r[3] = 0x10; r[4] = (uint8_t)n;
    r[5] = (uint8_t)offset; r[6] = (uint8_t)(offset >> 8);
    unsigned sum = r[3] + r[4] + r[5] + r[6];
    r[1] = (uint8_t)sum; r[2] = (uint8_t)(sum >> 8);
    return true;
}

static bool accept_reply(Reader *r, const uint8_t *data, CFIndex length) {
    if (!r->waiting || length != REPORT_LEN || data[0] != 4 ||
        memcmp(data + 3, r->request + 3, 4) != 0) {
        return false;
    }
    memcpy(r->reply, data + 8, r->request[4]);
    r->received = true;
    r->waiting = false;
    return true;
}

static void input_report(void *ctx, IOReturn result, void *sender,
                         IOHIDReportType type, uint32_t id,
                         uint8_t *data, CFIndex length) {
    (void)sender;
    // Keyboard/media/mouse reports on this shared interface are ignored.
    if (result == kIOReturnSuccess && type == kIOHIDReportTypeInput && id == 4) {
        (void)accept_reply(ctx, data, length);
    }
}

static bool read_chunk(IOHIDDeviceRef d, Reader *r, uint32_t addr, unsigned n, uint8_t *out) {
    if (!make_request(addr, n, r->request)) {
        return false;
    }
    for (unsigned retry = 0; retry < 3; retry++) {
        r->received = false; r->waiting = true;
        // The only device-output call in this program. Fixed read opcode 0x10.
        IOReturn s = IOHIDDeviceSetReport(d, kIOHIDReportTypeOutput, 4, r->request, REPORT_LEN);
        if (s != kIOReturnSuccess) {
            fprintf(stderr, "Read request failed: 0x%08x\n", s);
            r->waiting = false;
            return false;
        }
        CFAbsoluteTime until = CFAbsoluteTimeGetCurrent() + 1.0;
        while (!r->received && CFAbsoluteTimeGetCurrent() < until) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, true);
        }
        r->waiting = false;
        if (r->received) {
            memcpy(out, r->reply, n);
            return true;
        }
    }
    fprintf(stderr, "No matching vendor reply at 0x%04x; stopped.\n", addr);
    return false;
}

static int self_test(void) {
    Reader r = {0};
    for (uint32_t addr = 0; addr < FLASH_SIZE; addr++) {
        unsigned n = FLASH_SIZE - addr < CHUNK ? FLASH_SIZE - addr : CHUNK;
        if (!make_request(addr, n, r.request)) {
            return 1;
        }
        uint8_t *b = r.request;
        unsigned sum = 0;
        for (unsigned i = 3; i < REPORT_LEN; i++) {
            sum += b[i];
        }
        if (b[0] != 4 || b[3] != 0x10 || b[4] != n ||
            (uint16_t)(0xa700 + b[5] + (b[6] << 8)) != addr ||
            (unsigned)(b[1] | (b[2] << 8)) != sum ||
            (b[1] == 0xee && b[2] == 0xee && b[3] == 0)) {
            return 1;
        }
    }
    if (make_request(65536, 1, r.request) || make_request(65535, 2, r.request) ||
        make_request(0, 0, r.request) || make_request(0, CHUNK + 1, r.request)) {
        return 1;
    }
    if (!make_request(MARKER, 4, r.request)) {
        return 1;
    }
    uint8_t response[REPORT_LEN];
    memcpy(response, r.request, REPORT_LEN);
    memcpy(response + 8, marker, sizeof(marker));
    r.waiting = true;
    if (accept_reply(&r, response, 8)) {
        return 1;
    }
    response[0] = 1;
    if (accept_reply(&r, response, REPORT_LEN)) {
        return 1;
    }
    response[0] = 4; response[5] ^= 1;
    if (accept_reply(&r, response, REPORT_LEN)) {
        return 1;
    }
    response[5] ^= 1;
    if (!accept_reply(&r, response, REPORT_LEN) || memcmp(r.reply, marker, 4)) {
        return 1;
    }
    if (accept_reply(&r, response, REPORT_LEN)) {
        return 1;
    }
    puts("PASS: all 65536 addresses use read-only framing; invalid ranges and unrelated/truncated replies rejected.");
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        return self_test();
    }
    bool send = argc == 4 && !strcmp(argv[1], "--send") && !strcmp(argv[2], "--out");
    if (argc != 1 && !send) {
        fprintf(stderr, "Usage: %s [--self-test | --send --out NEW_BACKUP_FILE]\n", argv[0]);
        return 2;
    }
    uint32_t expected_location = k630_location();
    if (!expected_location) {
        fputs("Set K630_USB_LOCATION from list_usb_macos first.\n", stderr);
        return 2;
    }
    setbuf(stdout, NULL);
    printf("Input Monitoring granted: %s\n", CGPreflightListenEventAccess() ? "yes" : "no");
    IOHIDManagerRef m = IOHIDManagerCreate(NULL, 0);
    if (!m) {
        return 1;
    }
    int vid = 0x320f, pid = 0x5000;
    CFNumberRef v = CFNumberCreate(NULL, kCFNumberIntType, &vid);
    CFNumberRef p = CFNumberCreate(NULL, kCFNumberIntType, &pid);
    const void *keys[] = {CFSTR(kIOHIDVendorIDKey), CFSTR(kIOHIDProductIDKey)};
    const void *values[] = {v, p};
    CFDictionaryRef match = CFDictionaryCreate(NULL, keys, values, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    IOHIDManagerSetDeviceMatching(m, match);
    CFRelease(match); CFRelease(v); CFRelease(p);
    CFSetRef ds = IOHIDManagerCopyDevices(m);
    IOHIDDeviceRef target = NULL;
    unsigned candidates = 0;
    if (ds) {
        CFIndex count = CFSetGetCount(ds);
        const void **items = calloc((size_t)count, sizeof(*items));
        if (!items && count) {
            CFRelease(ds);
            CFRelease(m);
            return 1;
        }
        CFSetGetValues(ds, items);
        for (CFIndex i = 0; i < count; i++) {
            IOHIDDeviceRef d = (IOHIDDeviceRef)items[i];
            if (vendor_collection(d) && number(d, CFSTR(kIOHIDMaxOutputReportSizeKey)) == REPORT_LEN &&
                number(d, CFSTR(kIOHIDMaxInputReportSizeKey)) == REPORT_LEN) {
                candidates++;
                if ((uint32_t)number(d, CFSTR(kIOHIDLocationIDKey)) == expected_location) {
                    target = d;
                }
            }
        }
        free(items);
    }
    int result = 1;
    if (!target || candidates != 1) {
        fprintf(stderr, "Expected exactly one stock vendor interface at USB location 0x%08x; found %u candidates.\n", expected_location, candidates);
        goto done;
    }
    printf("Selected 320f:5000 at 0x%08x, vendor collection ff1c:0092, report 4.\n", expected_location);
    if (!send) {
        puts("DRY RUN. No device opened and nothing sent.");
        result = 0;
        goto done;
    }
    IOReturn status = IOHIDDeviceOpen(target, kIOHIDOptionsTypeNone);
    if (status != kIOReturnSuccess) {
        fprintf(stderr, "Open failed: 0x%08x\n", status);
        goto done;
    }
    Reader reader = {0};
    IOHIDDeviceRegisterInputReportCallback(target, reader.input, REPORT_LEN, input_report, &reader);
    IOHIDDeviceScheduleWithRunLoop(target, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    uint8_t check[4], *copies = NULL;
    if (!read_chunk(target, &reader, MARKER, 4, check)) {
        goto close_device;
    }
    printf("Boot marker at 0x27c0: %02x %02x %02x %02x\n", check[0], check[1], check[2], check[3]);
    if (memcmp(check, marker, sizeof(marker))) {
        fputs("Unexpected boot layout. No further commands sent.\n", stderr);
        goto close_device;
    }
    copies = malloc(FLASH_SIZE * 2);
    if (!copies) {
        goto close_device;
    }
    for (unsigned pass = 0; pass < 2; pass++) {
        printf("Read %u/2\n", pass + 1);
        for (uint32_t addr = 0; addr < FLASH_SIZE;) {
            unsigned n = FLASH_SIZE - addr < CHUNK ? FLASH_SIZE - addr : CHUNK;
            if (!read_chunk(target, &reader, addr, n, copies + pass * FLASH_SIZE + addr)) {
                goto close_device;
            }
            addr += n;
            if (addr % 0x3000 == 0 || addr == FLASH_SIZE) {
                printf("  %u/%u bytes\n", addr, FLASH_SIZE);
            }
        }
    }
    if (memcmp(copies, copies + FLASH_SIZE, FLASH_SIZE)) {
        fputs("Reads differ; no backup written.\n", stderr);
        goto close_device;
    }
    if (memcmp(copies + MARKER, marker, sizeof(marker))) {
        fputs("Marker changed; no backup written.\n", stderr);
        goto close_device;
    }
    FILE *file = fopen(argv[3], "wbx");
    if (!file) {
        perror("Create NEW backup file");
        goto close_device;
    }
    size_t written = fwrite(copies, 1, FLASH_SIZE, file);
    int closed = fclose(file);
    if (written != FLASH_SIZE || closed) {
        fputs("Backup file write failed; discard incomplete file.\n", stderr);
        goto close_device;
    }
    printf("VERIFIED: both 65536-byte reads match. Saved %s\n", argv[3]);
    result = 0;
close_device:
    free(copies);
    IOHIDDeviceUnscheduleFromRunLoop(target, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceRegisterInputReportCallback(target, reader.input, REPORT_LEN, NULL, NULL);
    IOHIDDeviceClose(target, kIOHIDOptionsTypeNone);
done:
    if (ds) {
        CFRelease(ds);
    }
    CFRelease(m);
    return result;
}
