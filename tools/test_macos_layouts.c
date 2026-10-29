// SPDX-License-Identifier: GPL-2.0-or-later
// Validate U.S. firmware sequences and EurKEY passthrough with the actual layouts.
// Does not select a layout or inject keyboard input.
#include <Carbon/Carbon.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void check_layout(CFStringRef input_id, bool eurkey) {
    const void *keys[] = {kTISPropertyInputSourceID};
    const void *values[] = {input_id};
    CFDictionaryRef filter = CFDictionaryCreate(NULL, keys, values, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFArrayRef sources = TISCreateInputSourceList(filter, true);
    assert(sources && CFArrayGetCount(sources));
    TISInputSourceRef source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, 0);
    CFDataRef data = TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
    assert(data);
    const UCKeyboardLayout *layout = (const UCKeyboardLayout *)CFDataGetBytePtr(data);
    const UInt16 letters[] = {kVK_ANSI_A, kVK_ANSI_O, kVK_ANSI_U};
    const UniChar expected[2][3] = {{0xe4, 0xf6, 0xfc}, {0xc4, 0xd6, 0xdc}};
    for (unsigned upper = 0; upper < 2; upper++) {
        for (unsigned i = 0; i < 3; i++) {
            UInt32 state = 0;
            UniChar chars[8]; UniCharCount count;
            if (!eurkey) {
                assert(UCKeyTranslate(layout, kVK_ANSI_U, kUCKeyActionDown, optionKey >> 8,
                                      40, 0, &state, 8, &count, chars) == noErr);
                assert(state != 0 && count == 0);
            }
            assert(UCKeyTranslate(layout, letters[i], kUCKeyActionDown,
                                  ((upper ? shiftKey : 0) | (eurkey ? optionKey : 0)) >> 8,
                                  40, 0, &state, 8, &count, chars) == noErr);
            if (count != 1 || chars[0] != expected[upper][i]) {
                fprintf(stderr, "upper=%u letter=%u state=%u count=%u got=U+%04X expected=U+%04X\n",
                        upper, i, (unsigned)state, (unsigned)count, chars[0], expected[upper][i]);
                exit(1);
            }
            // UCKeyTranslate's opaque state can retain high bits after a
            // composition. Verify normal following text, not a zero sentinel.
            assert(UCKeyTranslate(layout, kVK_ANSI_B, kUCKeyActionDown, 0,
                                  40, 0, &state, 8, &count, chars) == noErr);
            assert(count == 1 && chars[0] == 'b');
        }
    }
    CFRelease(sources); CFRelease(filter);
}

int main(void) {
    check_layout(CFSTR("com.apple.keylayout.US"), false);
    check_layout(CFSTR("org.unknown.keylayout.EurKEYv12"), true);
    puts("macOS U.S. sequences and EurKEY v1.2 direct Option chords both yield ä ö ü Ä Ö Ü.");
}
