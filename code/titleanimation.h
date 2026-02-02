#ifndef TITLEANIMATION_H
#define TITLEANIMATION_H

#include <libdragon.h>
#include <stdbool.h>

typedef struct {
    const char* title_text;
    float typing_timer;
    int typing_char_index;
    bool show_title;
    float title_hide_timer;
    bool show_go;
    float go_timer;
    float char_delay;        // Delay between characters (default: 0.1s)
    float pause_duration;    // Pause between title and GO! (default: 1.0s)
    float go_duration;       // How long GO! stays visible (default: 1.0s)
} TitleAnimation;

// Initialize the title animation with default timing
void title_animation_init(TitleAnimation* anim, const char* title_text);

// Initialize with custom timing
void title_animation_init_custom(TitleAnimation* anim, const char* title_text, 
                                  float char_delay, float pause_duration, float go_duration);

// Update the animation state
void title_animation_update(TitleAnimation* anim, float delta_time);

// Render the animation at specified position
void title_animation_render(TitleAnimation* anim, rdpq_font_t* font, int font_id, int y_position);

// Check if animation is completely finished
bool title_animation_is_finished(TitleAnimation* anim);

// Reset the animation to play again
void title_animation_reset(TitleAnimation* anim);

#endif // TITLEANIMATION_H
