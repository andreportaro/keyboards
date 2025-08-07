
#include "quantum.h"

#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    if (is_keyboard_master()) {
        return OLED_ROTATION_270; // vertical for master
    }
    return rotation;
}

static void render_logo(void) {
    static const char PROGMEM raw_logo[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,128,192, 32, 48, 16, 56,184, 48, 48,112,224,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,128,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15, 28, 16, 17, 49, 48, 32,240,248, 15,  7,  0,  0,  0,128,128,192, 96,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,120, 68, 70, 65, 33, 32, 32, 32,224,  0,  0,112,128,192, 48, 12,  0,  0,  0,  0, 48,120,232, 72,232,184,  0,  0,  0,  0,  0,  0,  0,  0,  4,250, 36, 50, 50, 50, 42,174,160,224,240, 24,  8,  8,  8,  8,  8,240, 16, 16, 32,192,  0,  0,  0,  0,128, 96,152, 64, 64, 64, 64, 64,192, 64,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  4,  4,  6,  6,  7,255,255,225,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 48,  8,  8,  6,  1,  0,  0,  0, 63, 33,  2,  6,  6,  4,  8,  8,  0,  0,  7,  4,  4,  5,  7,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  2,  1,  1,  1,  0,  0,  0,  1,  1,  2,  2,  2,  2,  2,  1,  0,  0, 64, 96, 17, 15,  2,  1, 16, 26, 42, 36, 34, 35, 35, 35, 34, 34, 34,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 12,104,112, 16, 48, 32, 56, 56, 44, 38, 71, 67, 67, 66, 68, 44, 40, 48, 32, 48, 16,104, 44,  4,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    };
    oled_write_raw_P(raw_logo, sizeof(raw_logo));
}
#define MAX_RAINDROPS 15
#define SCREEN_WIDTH 32
#define SCREEN_HEIGHT 128
#define WPM_DISPLAY_HEIGHT 10
#define RAIN_AREA_START WPM_DISPLAY_HEIGHT
#define RAIN_AREA_HEIGHT (SCREEN_HEIGHT - WPM_DISPLAY_HEIGHT)

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t speed;
    bool active;
} raindrop_t;

typedef struct {
    uint16_t current_wpm;
    uint16_t recent_wpm;
    uint8_t active_drop_count;
    uint8_t target_drop_count;
    uint16_t keystrokes;
    uint32_t typing_start_time;
    uint32_t last_wpm_update;
    uint32_t last_keystroke_time;
} weather_system_t;

static raindrop_t raindrops[MAX_RAINDROPS];
static weather_system_t weather;
static uint32_t last_animation_time = 0;
static uint16_t animation_frame = 0;

// Calculate target number of raindrops based on WPM
uint8_t calculate_target_drops(uint16_t wpm) {
    if (wpm == 0) return 0;
    if (wpm <= 20) return 3;   // Light drizzle
    if (wpm <= 40) return 7;   // Light rain
    if (wpm <= 60) return 12;  // Moderate rain
    return 15;                 // Heavy downpour (60+ WPM)
}

// Calculate raindrop speed based on WPM
uint8_t calculate_drop_speed(uint16_t wpm, uint8_t drop_index) {
    uint8_t base_speed = 1 + (wpm / 20);  // 1-4 pixels per frame
    if (base_speed > 4) base_speed = 4;

    // Add slight variation between drops
    uint8_t variation = (drop_index % 3);
    return base_speed + (variation > 0 ? variation - 1 : 0);
}

// Initialize weather system
void init_weather_system(void) {
    weather.current_wpm = 0;
    weather.recent_wpm = 0;
    weather.active_drop_count = 0;
    weather.target_drop_count = 0;
    weather.keystrokes = 0;
    weather.typing_start_time = timer_read32();
    weather.last_wpm_update = timer_read32();
    weather.last_keystroke_time = 0;

    // Initialize all raindrops as inactive
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        raindrops[i].x = (i * 13 + 7) % SCREEN_WIDTH;
        raindrops[i].y = RAIN_AREA_START + ((i * 7) % (RAIN_AREA_HEIGHT - 5));
        raindrops[i].speed = 1;
        raindrops[i].active = false;
    }

    animation_frame = 0;
}

// Update WPM calculation
void weather_update_wpm(void) {
    uint32_t current_time = timer_read32();

    if (weather.keystrokes > 0) {
        uint32_t elapsed_ms = current_time - weather.typing_start_time;
        if (elapsed_ms > 1000) {  // Only calculate after 1 second
            weather.current_wpm = (weather.keystrokes * 60000) / (elapsed_ms * 5);
        }
    }

    // Calculate recent WPM (decay if no recent typing)
    if (current_time - weather.last_keystroke_time > 3000) {  // 3 seconds of no typing
        // Gradually decay WPM
        if (weather.recent_wpm > 0) {
            weather.recent_wpm = weather.recent_wpm * 95 / 100;  // 5% decay
        }
    } else {
        // Smooth transition to current WPM
        weather.recent_wpm = (weather.recent_wpm * 3 + weather.current_wpm) / 4;
    }

    weather.last_wpm_update = current_time;
}

// Handle keystrokes for WPM tracking
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;  // Only on key press

    // Count all keystrokes for WPM
    weather.keystrokes++;
    weather.last_keystroke_time = timer_read32();
    weather_update_wpm();

    return true;  // Process all keys normally
}

// Manage raindrop activation/deactivation
void manage_raindrops(void) {
    weather.target_drop_count = calculate_target_drops(weather.recent_wpm);

    // Gradually adjust active drop count
    if (weather.active_drop_count < weather.target_drop_count) {
        // Activate more drops (1-2 per update cycle)
        for (int i = 0; i < MAX_RAINDROPS && weather.active_drop_count < weather.target_drop_count; i++) {
            if (!raindrops[i].active) {
                raindrops[i].active = true;
                raindrops[i].y = RAIN_AREA_START;
                raindrops[i].x = (animation_frame * 17 + i * 23) % SCREEN_WIDTH;
                raindrops[i].speed = calculate_drop_speed(weather.recent_wpm, i);
                weather.active_drop_count++;
                break;  // Only activate one per cycle for smooth transition
            }
        }
    } else if (weather.active_drop_count > weather.target_drop_count) {
        // Deactivate some drops
        for (int i = MAX_RAINDROPS - 1; i >= 0 && weather.active_drop_count > weather.target_drop_count; i--) {
            if (raindrops[i].active) {
                raindrops[i].active = false;
                weather.active_drop_count--;
                break;  // Only deactivate one per cycle
            }
        }
    }
}

// Update weather system
void update_weather(void) {
    uint32_t current_time = timer_read32();

    // Update WPM every 500ms
    if (current_time - weather.last_wpm_update > 500) {
        weather_update_wpm();
    }

    // Manage raindrop count based on typing speed
    manage_raindrops();

    // Update active raindrops
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        if (raindrops[i].active) {
            raindrops[i].y += raindrops[i].speed;

            // Reset raindrop when it hits bottom
            if (raindrops[i].y >= SCREEN_HEIGHT) {
                raindrops[i].y = RAIN_AREA_START;
                raindrops[i].x = (animation_frame * 19 + i * 31) % SCREEN_WIDTH;
                raindrops[i].speed = calculate_drop_speed(weather.recent_wpm, i);
            }
        }
    }

    animation_frame++;
}

// Draw WPM display at top
void draw_wpm_display(void) {
    // Display WPM
    oled_set_cursor(0, 0);
    oled_write_P(PSTR("WPM: "), false);

    char wpm_str[8];
    sprintf(wpm_str, "%d", weather.recent_wpm);
    oled_write_ln(wpm_str, false);

    oled_write_ln(PSTR("Drops: "), false);
    char drops_str[4];
    sprintf(drops_str, "%d", weather.active_drop_count);
    oled_write_ln(drops_str, false);
}

// Draw rain animation
void draw_rain(void) {
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        if (raindrops[i].active &&
            raindrops[i].y >= RAIN_AREA_START &&
            raindrops[i].y < SCREEN_HEIGHT) {

            // Draw main raindrop
            oled_write_pixel(raindrops[i].x, raindrops[i].y, true);

            // For heavy rain (high WPM), make drops wider
            if (weather.recent_wpm > 60) {
                if (raindrops[i].x > 0) {
                    oled_write_pixel(raindrops[i].x - 1, raindrops[i].y, true);
                }
                if (raindrops[i].x < SCREEN_WIDTH - 1) {
                    oled_write_pixel(raindrops[i].x + 1, raindrops[i].y, true);
                }
            }

            // Draw trail for motion effect
            if (raindrops[i].y > RAIN_AREA_START) {
                oled_write_pixel(raindrops[i].x, raindrops[i].y - 1, true);
            }
            if (raindrops[i].y > RAIN_AREA_START + 1 && raindrops[i].speed > 2) {
                oled_write_pixel(raindrops[i].x, raindrops[i].y - 2, true);
            }
        }
    }
}

void keyboard_post_init_kb(void) {
    if (!is_keyboard_master()) {
        render_logo();
    } else {
        oled_set_cursor(0, 0);
        oled_write("Layer", false);
    }
    keyboard_post_init_user();
}
bool oled_task_user(void) {

    if (is_keyboard_master()) {
        uint32_t current_time = timer_read32();

        // Initialize on first run
        static bool initialized = false;
        if (!initialized) {
            init_weather_system();
            initialized = true;
        }

        // Update weather system at ~20fps (50ms intervals)
        if (current_time - last_animation_time > 50) {
            update_weather();

            oled_clear();
            draw_wpm_display();
            draw_rain();

            last_animation_time = current_time;
        }

        return false;
    } else {
        render_logo();
        oled_scroll_left();
    }
    return false;
}
#endif
