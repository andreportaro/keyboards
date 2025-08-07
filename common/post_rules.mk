ifeq ($(strip $(OLED_ENABLE)), yes)
	SRC += keyboards/handwired/skaterboyz/common/display_oled.c
endif
