include keyboards/redragon/common/redragon_ch555.mk

F_CPU = 32000000
EEPROM_DRIVER = transient
SRC += os_mode.c umlaut.c
# Stock key scan at 0x9237 and key table at 0xA100 identify Esc as P4.1/P0.0.
EVISION_ESC_ROW = 1

GRAVE_ESC_ENABLE = no
MAGIC_ENABLE = no
SPACE_CADET_ENABLE = no
SEND_STRING_ENABLE = no
