#include "titleanimation.h"
#include <string.h>

void title_animation_init(TitleAnimation* anim, const char* title_text) {
    title_animation_init_custom(anim, title_text, 0.1f, 1.0f, 1.0f);
}

void title_animation_init_custom(TitleAnimation* anim, const char* title_text, 
                                  float char_delay, float pause_duration, float go_duration) {
    anim->title_text = title_text;
    anim->typing_timer = 0.0f;
    anim->typing_char_index = 0;
    anim->show_title = true;
    anim->title_hide_timer = 0.0f;
    anim->show_go = false;
    anim->go_timer = 0.0f;
    anim->char_delay = char_delay;
    anim->pause_duration = pause_duration;
    anim->go_duration = go_duration;
}

void title_animation_update(TitleAnimation* anim, float delta_time) {
    // Update title typing animation
    if (anim->show_title) {
        int title_length = strlen(anim->title_text);
        if (anim->typing_char_index < title_length) {
            // Typing phase - reveal characters
            anim->typing_timer += delta_time;
            if (anim->typing_timer >= anim->char_delay) {
                anim->typing_timer = 0.0f;
                anim->typing_char_index++;
            }
        } else {
            // All characters typed - wait before showing GO!
            anim->title_hide_timer += delta_time;
            if (anim->title_hide_timer >= anim->pause_duration) {
                anim->show_title = false;
                anim->show_go = true;
                anim->go_timer = 0.0f;
            }
        }
    }
    
    // Update GO! timer
    if (anim->show_go) {
        anim->go_timer += delta_time;
        if (anim->go_timer >= anim->go_duration) {
            anim->show_go = false;
        }
    }
}

void title_animation_render(TitleAnimation* anim, rdpq_font_t* font, int font_id, int y_position) {
    rdpq_sync_pipe();
    
    // Draw title typing animation in center of screen
    if (anim->show_title) {
        // Create a temporary buffer to hold the visible characters
        char visible_text[128];
        int chars_to_show = anim->typing_char_index;
        if (chars_to_show > 0) {
            strncpy(visible_text, anim->title_text, chars_to_show);
            visible_text[chars_to_show] = '\0';
            
            // Calculate center position (320x240 screen, estimate ~8 pixels per char)
            int text_width = chars_to_show * 8;
            int x = (320 - text_width) / 2;
            
            rdpq_text_printf(NULL, font_id, x, y_position, "%s", visible_text);
        }
    }
    
    // Draw GO! message
    if (anim->show_go) {
        // Center "GO!" text (3 chars * 8 pixels = 24 pixels)
        int x = (320 - 24) / 2;
        rdpq_text_printf(NULL, font_id, x, y_position, "GO!");
    }
}

bool title_animation_is_finished(TitleAnimation* anim) {
    return !anim->show_title && !anim->show_go;
}

void title_animation_reset(TitleAnimation* anim) {
    anim->typing_timer = 0.0f;
    anim->typing_char_index = 0;
    anim->show_title = true;
    anim->title_hide_timer = 0.0f;
    anim->show_go = false;
    anim->go_timer = 0.0f;
}
