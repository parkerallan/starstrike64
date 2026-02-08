#include "startup.h"
#include "scenes.h"

void startup_init(SceneStartup* scene, rdpq_font_t* font) {
    scene->scene_time = 0.0f;
    scene->last_update_time = 0.0f;
    scene->font = font;
    scene->current_logo = 0;
    scene->sound_played = false;
    
    // Load logo sprites
    scene->libdragon_sprite = sprite_load("rom:/libdragon.sprite");
    scene->tiny3d_sprite = sprite_load("rom:/tiny3d.sprite");
    
    // Load startup sound
    wav64_open(&scene->startup_sound, "rom:/gamestart.wav64");
    
    rdpq_text_register_font(1, scene->font);

    debugf("Startup scene initialized\n");
}

int startup_update(SceneStartup* scene) {
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

    // Update scene timer
    scene->scene_time += delta_time;

    // Play startup sound on first frame of libdragon logo
    if (!scene->sound_played && scene->current_logo == 0) {
        wav64_play(&scene->startup_sound, 1);  // Use channel 1 (0 is reserved for music)
        scene->sound_played = true;
        debugf("Playing startup sound\n");
    }

    // Check for skip input
    joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (btn.start || btn.a) {
        debugf("User skipped startup, transitioning to intro\n");
        return SCENE_INTRO;
    }

    // Transition between logos
    if (scene->current_logo == 0 && scene->scene_time >= 3.0f) {
        // Switch to tiny3d logo
        scene->current_logo = 1;
        scene->scene_time = 0.0f;
        debugf("Switching to tiny3d logo\n");
    } else if (scene->current_logo == 1 && scene->scene_time >= 3.0f) {
        // Transition to intro
        debugf("Transitioning to intro\n");
        return SCENE_INTRO;
    }

    return -1; // No transition
}

void startup_render(SceneStartup* scene) {
    rdpq_attach(display_get(), NULL);
    
    // Clear screen to black
    rdpq_set_mode_fill(RGBA32(0, 0, 0, 255));
    rdpq_fill_rectangle(0, 0, 320, 240);
    
    // Set up sprite rendering
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    
    // Calculate fade alpha based on time
    float alpha = 1.0f;
    if (scene->scene_time < 0.5f) {
        // Fade in
        alpha = scene->scene_time / 0.5f;
    } else if (scene->scene_time > 2.5f) {
        // Fade out
        alpha = 1.0f - ((scene->scene_time - 2.5f) / 0.5f);
    }
    
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    
    // Draw appropriate logo centered
    sprite_t* current_sprite = (scene->current_logo == 0) ? scene->libdragon_sprite : scene->tiny3d_sprite;
    
    if (current_sprite) {
        int sprite_width = current_sprite->width;
        int sprite_height = current_sprite->height;
        int x = (320 - sprite_width) / 2;
        int y = (240 - sprite_height) / 2;
        
        rdpq_blitparms_t params = {
            .cx = sprite_width / 2,
            .cy = sprite_height / 2,
        };
        rdpq_sprite_blit(current_sprite, x + sprite_width/2, y + sprite_height/2, &params);
    }

    rdpq_detach_show();
}

void startup_cleanup(SceneStartup* scene) {
    // Stop and close startup sound
    mixer_ch_stop(1);  // Stop channel 1 first
    wav64_close(&scene->startup_sound);
    
    if (scene->libdragon_sprite) {
        sprite_free(scene->libdragon_sprite);
        scene->libdragon_sprite = NULL;
    }
    
    if (scene->tiny3d_sprite) {
        sprite_free(scene->tiny3d_sprite);
        scene->tiny3d_sprite = NULL;
    }
    
    rdpq_text_unregister_font(1);
    debugf("Startup scene cleaned up\n");
}
