// SPDX-License-Identifier: GPL-2.0-or-later
// Read-only VIA diagnostics bound to K630_USB_LOCATION.
#include "k630_hid_macos.c"
#include <stdio.h>

static uint8_t input[32], response[32], expected;
static int received;

static void reply(void *context, IOReturn result, void *sender,
                  IOHIDReportType type, uint32_t id, uint8_t *data, CFIndex n) {
    (void)context; (void)sender;
    if (result || type != kIOHIDReportTypeInput || id || n != 32 || data[0] != expected) {
        return;
    }
    memcpy(response, data, 32);
    received = 1;
}

static void exchange(const uint8_t request[32]) {
    expected = request[0];
    received = 0;
    IOReturn s = IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 0, request, 32);
    if (s) {
        fprintf(stderr, "VIA write: 0x%08x\n", s);
        exit(1);
    }
    double until = CFAbsoluteTimeGetCurrent() + 2;
    while (!received && CFAbsoluteTimeGetCurrent() < until) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);
    }
    if (!received) {
        fprintf(stderr, "No VIA reply for 0x%02x\n", expected);
        exit(1);
    }
}

static unsigned test_key(void) {
    const uint8_t request[32] = {4, 2, 1, 0};
    exchange(request);
    if (memcmp(request, response, 4)) {
        fputs("Unexpected key response\n", stderr);
        exit(1);
    }
    return (unsigned)response[4] * 256 + response[5];
}

static unsigned read_u32(const uint8_t *p) {
    return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | p[3];
}

static void read_buffer(uint8_t command, uint8_t *buffer, unsigned size) {
    for (unsigned offset = 0; offset < size; offset += 28) {
        unsigned n = size - offset < 28 ? size - offset : 28;
        uint8_t request[32] = {command, (uint8_t)(offset >> 8), (uint8_t)offset, (uint8_t)n};
        exchange(request);
        if (memcmp(request, response, 4)) {
            fputs("Unexpected buffer response\n", stderr);
            exit(1);
        }
        memcpy(buffer + offset, response + 4, n);
    }
}

int main(int argc, char **argv) {
    bool dump = argc == 2 && !strcmp(argv[1], "--dump");
    if (argc != 1 && !dump) {
        fputs("usage: via_probe_macos [--dump]\n", stderr);
        return 2;
    }
    if (!k630_location()) {
        fputs("Set K630_USB_LOCATION from list_usb_macos first.\n", stderr);
        return 2;
    }
    int s = k630_open(3);
    if (s != 1) {
        fprintf(stderr, "K630 open: 0x%08x\n", (unsigned)s);
        return 1;
    }
    IOHIDDeviceRegisterInputReportCallback(device, input, sizeof(input), reply, NULL);
    IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    exchange((uint8_t[32]){1});
    unsigned protocol = (unsigned)response[1] * 256 + response[2];
    exchange((uint8_t[32]){0x11});
    unsigned layers = response[1];
    if (layers < 1 || layers > 8) {
        fputs("Unexpected layer count\n", stderr);
        return 1;
    }
    exchange((uint8_t[32]){2, 4});
    unsigned firmware = read_u32(response + 2);
    exchange((uint8_t[32]){2, 1});
    unsigned uptime = read_u32(response + 2);
    unsigned mode = 255;
    if (firmware >= 0x200) {
        exchange((uint8_t[32]){8, 0x4b, 1});
        mode = response[3];
        if (response[1] != 0x4b || response[2] != 1 || (mode != 0 && mode != 2)) {
            fputs("Unexpected OS-mode response\n", stderr);
            return 1;
        }
    }
    unsigned profile = 255;
    if (firmware >= 0x400) {
        exchange((uint8_t[32]){8, 0x4b, 2});
        profile = response[3];
        if (response[1] != 0x4b || response[2] != 2 || profile > 1) {
            fputs("Unexpected input-profile response\n", stderr);
            return 1;
        }
    }
    if (dump) {
        exchange((uint8_t[32]){0x0c});
        unsigned macros = response[1];
        exchange((uint8_t[32]){0x0d});
        unsigned macro_size = (unsigned)response[1] * 256 + response[2];
        if (macro_size > 2048) {
            fputs("Unexpected macro size\n", stderr);
            return 1;
        }
        uint8_t keys[8 * 192], macro_data[2048], check[8 * 192 + 2048];
        unsigned key_size = layers * 192;
        read_buffer(0x12, keys, key_size);
        read_buffer(0x0e, macro_data, macro_size);
        read_buffer(0x12, check, key_size);
        read_buffer(0x0e, check + key_size, macro_size);
        if (memcmp(keys, check, key_size) || memcmp(macro_data, check + key_size, macro_size)) {
            fputs("VIA settings changed during the two reads; retry\n", stderr);
            return 1;
        }
        printf("{\"protocol\":%u,\"firmware\":%u,\"uptime_ms\":%u,\"os_base_layer\":%u,\"mac_input_profile\":%u,\"layers\":[",
               protocol, firmware, uptime, mode, profile);
        for (unsigned layer = 0; layer < layers; layer++) {
            printf("%s[", layer ? "," : "");
            for (unsigned key = 0; key < 96; key++) {
                unsigned offset = (layer * 96 + key) * 2;
                printf("%s%u", key ? "," : "", (unsigned)keys[offset] * 256 + keys[offset + 1]);
            }
            printf("]");
        }
        printf("],\"macro_count\":%u,\"macros_hex\":\"", macros);
        for (unsigned i = 0; i < macro_size; i++) {
            printf("%02x", macro_data[i]);
        }
        printf("\"}\n");
    } else {
        printf("VIA protocol: %u\nLayers: %u\nFirmware: 0x%08x\nUptime ms: %u\n", protocol, layers, firmware, uptime);
        if (mode != 255) {
            printf("OS mode: %s (base layer %u)\n", mode == 2 ? "Mac" : "Windows", mode);
        }
        if (profile != 255) {
            printf("Mac input profile: %s (%u)\n", profile ? "EurKEY" : "U.S.", profile);
        }
        unsigned key = test_key();
        printf("Layer 2 Esc: 0x%04x\n", key);
        // Cross-check the same key through VIA's bulk read, used by usevia.app.
        exchange((uint8_t[32]){0x12, 1, 0xa0, 2});
        unsigned bulk = (unsigned)response[4] * 256 + response[5];
        printf("Layer 2 Esc bulk: 0x%04x\n", bulk);
        if (memcmp(response, (uint8_t[]){0x12, 1, 0xa0, 2}, 4) || bulk != key) {
            fputs("VIA bulk readback mismatch\n", stderr);
            return 1;
        }
    }
    IOHIDDeviceUnscheduleFromRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    k630_close();
    return 0;
}
