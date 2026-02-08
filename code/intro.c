#include "intro.h"
#include "scenes.h"

void intro_init(SceneIntro* scene, rdpq_font_t* font) {
    scene->last_update_time = 0.0f;
    scene->scene_time = 0.0f;
    
    // Set up camera viewport
    scene->viewport = t3d_viewport_create();

    // Load mecha model
    scene->mecha_model = t3d_model_load("rom:/mecha.t3dm");
    if (!scene->mecha_model) {
        debugf("WARNING: Failed to load mecha model\n");
    } else {
        debugf("Successfully loaded mecha model\n");
    }

    // Initialize skeleton for rigged model
    const T3DChunkSkeleton* skelChunk = t3d_model_get_skeleton(scene->mecha_model);
    if (skelChunk && scene->mecha_model) {
        scene->skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *scene->skeleton = t3d_skeleton_create(scene->mecha_model);
        debugf("Skeleton created successfully\n");

        // Initialize animation system
        animation_system_init(&scene->anim_system, scene->mecha_model, scene->skeleton);
        
        // Start with idle animation
        animation_system_play(&scene->anim_system, "Idle", true);
    } else {
        debugf("No skeleton found in model\n");
        scene->skeleton = NULL;
    }

    // Allocate model matrix
    scene->modelMat = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(scene->modelMat);

    // Load tunnel map
    scene->tunnel_model = t3d_model_load("rom:/tunnel.t3dm");
    if (!scene->tunnel_model) {
        debugf("WARNING: Failed to load tunnel model\n");
    } else {
        debugf("Successfully loaded tunnel model\n");
    }
    
    // Allocate tunnel matrix
    scene->tunnelMat = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(scene->tunnelMat);

    // Use pre-loaded font
    scene->font = font;
    rdpq_text_register_font(1, scene->font);

    // Load logo sprite
    scene->logo_sprite = sprite_load("rom:/starstrikelogo.sprite");
    
    // Load press start sprite
    scene->press_start_sprite = sprite_load("rom:/pressstart.sprite");

    // Set up lighting
    scene->colorAmbient[0] = 180;
    scene->colorAmbient[1] = 180;
    scene->colorAmbient[2] = 180;
    scene->colorAmbient[3] = 0xFF;

    scene->colorDir[0] = 255;
    scene->colorDir[1] = 255;
    scene->colorDir[2] = 255;
    scene->colorDir[3] = 0xFF;

    scene->lightDirVec = (T3DVec3){{0.3f, -0.8f, 0.5f}};
    t3d_vec3_norm(&scene->lightDirVec);
    
    // Load and start music
    wav64_open(&scene->music, "rom:/Brilliance_Days.wav64");
    wav64_set_loop(&scene->music, true);
    mixer_ch_set_limits(0, 0, 48000, 0);
    wav64_play(&scene->music, 0);
    mixer_ch_set_vol(0, 0.25f, 0.25f);  // Standard volume

    debugf("Intro scene initialized\n");
}

int intro_update(SceneIntro* scene) {
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

    // Update animation system
    if (scene->skeleton) {
        animation_system_update(&scene->anim_system, delta_time);
    }

    // Handle input
    joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // Start button - go to Level 1
    if (btn.start) {
        debugf("Starting game - going to Level 1\n");
        return LEVEL_1;
    }

    // Set up camera
    const T3DVec3 camPos = {{0, 125.0f, 100.0f}};
    // Set model matrix
    float scale[3] = {1.0f, 1.0f, 1.0f};
    float rotation[3] = {0.0f, T3D_DEG_TO_RAD(35.0f), 0.0f};
    float position[3] = {0.0f, 0.0f, 0.0f};

    t3d_mat4fp_from_srt_euler(scene->modelMat, scale, rotation, position);

    const T3DVec3 camTarget = {{-35.0f, 100.0f, 0}};

    t3d_viewport_set_projection(&scene->viewport, T3D_DEG_TO_RAD(60.0f), 20.0f, 1000.0f);
    t3d_viewport_look_at(&scene->viewport, &camPos, &camTarget, &(T3DVec3){{0,1,0}});

    return -1; // No transition
}

void intro_render(SceneIntro* scene) {
    rdpq_attach(display_get(), display_get_zbuf());
    t3d_frame_start();
    t3d_viewport_attach(&scene->viewport);

    // Clear screen - Dark blue background
    t3d_screen_clear_color(RGBA32(10, 10, 50, 0xFF));
    t3d_screen_clear_depth();

    // Set render flags
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);

    // Set up lighting
    t3d_light_set_ambient(scene->colorAmbient);
    t3d_light_set_directional(0, scene->colorDir, &scene->lightDirVec);
    t3d_light_set_count(1);

    // Draw tunnel map if loaded
    if (scene->tunnel_model) {
        t3d_matrix_push(scene->tunnelMat);
        t3d_model_draw(scene->tunnel_model);
        t3d_matrix_pop(1);
    }

    // Draw mecha model if loaded
    if (scene->mecha_model) {
        t3d_matrix_push(scene->modelMat);
        
        T3DModelDrawConf drawConf = {
            .userData = NULL,
            .tileCb = NULL,
            .filterCb = NULL,
            .dynTextureCb = NULL,
            .matrices = scene->skeleton ? scene->skeleton->boneMatricesFP : NULL
        };
        
        t3d_model_draw_custom(scene->mecha_model, drawConf);
        t3d_matrix_pop(1);
    }

    // Draw UI
    rdpq_sync_pipe();
    
    // Set up proper rendering mode for sprites
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_TEX);
    
    // Draw logo sprite (left-aligned)
    if (scene->logo_sprite) {
        rdpq_sprite_blit(scene->logo_sprite, 10, 20, NULL);
    }
    
    // Blinking "Press Start" sprite (left-aligned)
    if (scene->press_start_sprite && ((int)(scene->scene_time * 2.0f)) % 2 == 0) {
        rdpq_sprite_blit(scene->press_start_sprite, 10, 130, NULL);
    }
    
    // Credits (left-aligned)
    rdpq_text_printf(NULL, 1, 10, 230, "parkerdev 2026");

    rdpq_detach_show();
}

void intro_cleanup(SceneIntro* scene) {
    // Stop and close music
    mixer_ch_stop(0);
    wav64_close(&scene->music);

    // Cleanup animation system
    if (scene->skeleton) {
        animation_system_cleanup(&scene->anim_system);
        t3d_skeleton_destroy(scene->skeleton);
        free_uncached(scene->skeleton);
        scene->skeleton = NULL;
    }

    if (scene->mecha_model) {
        t3d_model_free(scene->mecha_model);
    }

    if (scene->tunnel_model) {
        t3d_model_free(scene->tunnel_model);
    }

    if (scene->modelMat) {
        free_uncached(scene->modelMat);
    }
    
    if (scene->tunnelMat) {
        free_uncached(scene->tunnelMat);
    }

    if (scene->logo_sprite) {
        sprite_free(scene->logo_sprite);
    }
    
    if (scene->press_start_sprite) {
        sprite_free(scene->press_start_sprite);
    }

    rdpq_text_unregister_font(1);
    debugf("Intro scene cleaned up\n");
}
