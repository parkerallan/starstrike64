#include "end.h"
#include "scenes.h"
#include <string.h>

void end_init(SceneEnd* scene, rdpq_font_t* font) {
    scene->last_update_time = 0.0f;
    scene->scene_time = 0.0f;
    scene->scroll_offset = 240.0f; // Start below the screen
    scene->scroll_speed = 30.0f;   // Pixels per second (smooth movement at 60fps)
    scene->scroll_complete = false;
    scene->line_count = 0;
    
    // Use pre-loaded font
    scene->font = font;
    rdpq_text_register_font(1, scene->font);
    
    // Load credits text file
    FILE* fp = fopen("rom:/credits.txt", "r");
    if (!fp) {
        debugf("WARNING: Failed to load credits.txt\n");
        // Add fallback credits
        strcpy(scene->credits_lines[0], "STAR STRIKE 64");
        strcpy(scene->credits_lines[1], "");
        strcpy(scene->credits_lines[2], "CREDITS");
        strcpy(scene->credits_lines[3], "");
        strcpy(scene->credits_lines[4], "Created with Libdragon");
        strcpy(scene->credits_lines[5], "and Tiny3D");
        scene->line_count = 6;
    } else {
        // Read credits file line by line
        while (scene->line_count < MAX_CREDITS_LINES && 
               fgets(scene->credits_lines[scene->line_count], MAX_LINE_LENGTH, fp) != NULL) {
            // Remove newline character if present
            size_t len = strlen(scene->credits_lines[scene->line_count]);
            if (len > 0 && scene->credits_lines[scene->line_count][len - 1] == '\n') {
                scene->credits_lines[scene->line_count][len - 1] = '\0';
            }
            scene->line_count++;
        }
        fclose(fp);
        debugf("Loaded %d lines from credits.txt\n", scene->line_count);
    }
    
    // Load and start music
    wav64_open(&scene->music, "rom:/Heartbeat_of_the_Earth.wav64");
    wav64_set_loop(&scene->music, true);
    mixer_ch_set_limits(0, 0, 48000, 0);
    wav64_play(&scene->music, 0);
    mixer_ch_set_vol(0, 0.5f, 0.5f);  // Standard volume

    debugf("End scene initialized\n");
}

int end_update(SceneEnd* scene) {
    // Calculate delta time
    float current_time = (float)((double)get_ticks_us() / 1000000.0);
    float delta_time;
    if (scene->last_update_time == 0.0f) {
        delta_time = 1.0f / 60.0f;
    } else {
        delta_time = current_time - scene->last_update_time;
    }
    scene->last_update_time = current_time;
    
    if (delta_time < 0.0f || delta_time > 0.5f) {
        delta_time = 1.0f / 60.0f;
    }
    
    scene->scene_time += delta_time;
    
    // Scroll credits upward if not complete
    if (!scene->scroll_complete) {
        scene->scroll_offset -= scene->scroll_speed * delta_time;
        
        // Calculate total height of credits (line height 16 pixels)
        float total_height = scene->line_count * 16.0f;
        
        // Check if scrolling is complete (all text has scrolled past top of screen)
        if (scene->scroll_offset + total_height < 0.0f) {
            scene->scroll_complete = true;
            debugf("Credits scroll complete\n");
        }
    }
    
    // Check for start button to return to intro
    if (scene->scroll_complete) {
        joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
        if (joypad.btn.start) {
            debugf("Returning to intro scene\n");
            return SCENE_INTRO;
        }
    }
    
    // No scene transition
    return -1;
}

void end_render(SceneEnd* scene) {
    surface_t* disp = display_get();
    
    // Clear screen to black
    rdpq_attach(disp, NULL);
    rdpq_clear(RGBA32(0, 0, 0, 255));
    
    // Render credits text (centered, scrolling)
    for (int i = 0; i < scene->line_count; i++) {
        float y_pos = scene->scroll_offset + (i * 16.0f);
        
        // Only render lines that are visible on screen
        if (y_pos > -16.0f && y_pos < 240.0f) {
            // Use rdpq_text_printf with center alignment
            rdpq_textparms_t parms = {
                .align = ALIGN_CENTER,
                .width = 320
            };
            rdpq_text_printf(&parms, 1, 0, (int)y_pos, "%s", scene->credits_lines[i]);
        }
    }
    
    // If scrolling is complete, show "press start to return" message
    if (scene->scroll_complete) {
        // Blink effect
        float blink = sinf(scene->scene_time * 3.0f) * 0.5f + 0.5f;
        if (blink > 0.5f) {
            rdpq_textparms_t parms = {
                .align = ALIGN_CENTER,
                .width = 320
            };
            rdpq_text_printf(&parms, 1, 0, 200, "PRESS START TO RETURN TO TITLE");
        }
    }
    
    rdpq_detach_show();
}

void end_cleanup(SceneEnd* scene) {
    debugf("End scene cleanup\n");
    // Unregister font (don't free it as it's not owned by this scene)
    rdpq_text_unregister_font(1);
    mixer_ch_stop(0);
    wav64_close(&scene->music);
}
