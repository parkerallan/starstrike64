#include "level1.h"
#include "scenes.h"

void level1_init(Level1* level, rdpq_font_t* font) {
    level->last_update_time = 0.0f;
    
    // Set up camera viewport
    level->viewport = t3d_viewport_create();

    // Load mecha model (you'll need to add a model file to assets/)
    level->mecha_model = t3d_model_load("rom:/mecha.t3dm");
    if (!level->mecha_model) {
        debugf("WARNING: Failed to load mecha model\n");
    } else {
        debugf("Successfully loaded mecha model\n");
    }

    // Initialize skeleton for rigged model
    const T3DChunkSkeleton* skelChunk = t3d_model_get_skeleton(level->mecha_model);
    if (skelChunk && level->mecha_model) {
        level->skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *level->skeleton = t3d_skeleton_create(level->mecha_model);
        debugf("Skeleton created successfully\n");

        // Initialize animation system
        animation_system_init(&level->anim_system, level->mecha_model, level->skeleton);
        
        // Start with idle animation
        animation_system_play(&level->anim_system, "Idle", true);
    } else {
        debugf("No skeleton found in model\n");
        level->skeleton = NULL;
    }

    // Allocate model matrix
    level->modelMat = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(level->modelMat);

    // Load stars map
    level->stars_model = t3d_model_load("rom:/stars.t3dm");
    if (!level->stars_model) {
        debugf("WARNING: Failed to load stars model\n");
    } else {
        debugf("Successfully loaded stars model\n");
    }
    
    // Initialize skeleton for stars animation
    const T3DChunkSkeleton* starsSkelChunk = t3d_model_get_skeleton(level->stars_model);
    if (starsSkelChunk && level->stars_model) {
        level->stars_skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *level->stars_skeleton = t3d_skeleton_create(level->stars_model);
        debugf("Stars skeleton created successfully\n");

        // Initialize stars animation system
        animation_system_init(&level->stars_anim_system, level->stars_model, level->stars_skeleton);
        
        // Start with Rotate animation
        animation_system_play(&level->stars_anim_system, "Rotate", true);
    } else {
        debugf("No skeleton found in stars model\n");
        level->stars_skeleton = NULL;
    }
    
    // Allocate stars matrix
    level->starsMat = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(level->starsMat);

    // Use pre-loaded font
    level->font = font;
    rdpq_text_register_font(1, level->font);

    // Set up lighting
    level->colorAmbient[0] = 180;
    level->colorAmbient[1] = 180;
    level->colorAmbient[2] = 180;
    level->colorAmbient[3] = 0xFF;

    level->colorDir[0] = 255;
    level->colorDir[1] = 255;
    level->colorDir[2] = 255;
    level->colorDir[3] = 0xFF;

    level->lightDirVec = (T3DVec3){{0.3f, -0.8f, 0.5f}};
    t3d_vec3_norm(&level->lightDirVec);
}

int level1_update(Level1* level) {
    // Calculate delta time
    float current_time = (float)((double)get_ticks_us() / 1000000.0);
    float delta_time;
    if (level->last_update_time == 0.0f) {
        delta_time = 1.0f / 60.0f;
    } else {
        delta_time = current_time - level->last_update_time;
    }
    level->last_update_time = current_time;
    
    if (delta_time < 0.0f || delta_time > 0.5f) {
        delta_time = 1.0f / 60.0f;
    }

    // Update animation system
    if (level->skeleton) {
        animation_system_update(&level->anim_system, delta_time);
    }
    
    // Update stars animation
    if (level->stars_skeleton) {
        animation_system_update(&level->stars_anim_system, delta_time);
    }

    // Handle input
    joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);

    // Start button - go to next level
    if (btn.start) {
        debugf("Going to Level 2\n");
        return LEVEL_2;
    }

    // Set up camera
    const T3DVec3 camPos = {{0, 0.0f, 200.0f}};
    const T3DVec3 camTarget = {{0, -50.0f, 0}};

    t3d_viewport_set_projection(&level->viewport, T3D_DEG_TO_RAD(60.0f), 20.0f, 1000.0f);
    t3d_viewport_look_at(&level->viewport, &camPos, &camTarget, &(T3DVec3){{0,1,0}});

    // Set model matrix
    float scale[3] = {1.0f, 1.0f, 1.0f};
    float rotation[3] = {0.0f, T3D_DEG_TO_RAD(180.0f), 0.0f};
    float position[3] = {0.0f, -150.0f, 0.0f};

    t3d_mat4fp_from_srt_euler(level->modelMat, scale, rotation, position);

    return -1; // No transition
}

void level1_render(Level1* level) {
    rdpq_attach(display_get(), display_get_zbuf());
    t3d_frame_start();
    t3d_viewport_attach(&level->viewport);

    // Clear screen - Blue background
    t3d_screen_clear_color(RGBA32(50, 50, 200, 0xFF));
    t3d_screen_clear_depth();

    // Set render flags
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);

    // Set up lighting
    t3d_light_set_ambient(level->colorAmbient);
    t3d_light_set_directional(0, level->colorDir, &level->lightDirVec);
    t3d_light_set_count(1);

    // Draw stars map if loaded
    if (level->stars_model) {
        t3d_matrix_push(level->starsMat);
        
        T3DModelDrawConf starsDrawConf = {
            .userData = NULL,
            .tileCb = NULL,
            .filterCb = NULL,
            .dynTextureCb = NULL,
            .matrices = level->stars_skeleton ? level->stars_skeleton->boneMatricesFP : NULL
        };
        
        t3d_model_draw_custom(level->stars_model, starsDrawConf);
        t3d_matrix_pop(1);
    }

    // Draw mecha model if loaded
    if (level->mecha_model) {
        t3d_matrix_push(level->modelMat);
        
        T3DModelDrawConf drawConf = {
            .userData = NULL,
            .tileCb = NULL,
            .filterCb = NULL,
            .dynTextureCb = NULL,
            .matrices = level->skeleton ? level->skeleton->boneMatricesFP : NULL
        };
        
        t3d_model_draw_custom(level->mecha_model, drawConf);
        t3d_matrix_pop(1);
    }

    // Draw UI
    rdpq_sync_pipe();
    rdpq_text_printf(NULL, 1, 10, 10, "DEEP SPACE");
    rdpq_text_printf(NULL, 1, 10, 30, "Press Start for Level 2");

    rdpq_detach_show();
}

void level1_cleanup(Level1* level) {
    // Cleanup animation system
    if (level->skeleton) {
        animation_system_cleanup(&level->anim_system);
        t3d_skeleton_destroy(level->skeleton);
        free_uncached(level->skeleton);
        level->skeleton = NULL;
    }
    
    // Cleanup stars animation system
    if (level->stars_skeleton) {
        animation_system_cleanup(&level->stars_anim_system);
        t3d_skeleton_destroy(level->stars_skeleton);
        free_uncached(level->stars_skeleton);
        level->stars_skeleton = NULL;
    }

    if (level->mecha_model) {
        t3d_model_free(level->mecha_model);
    }
    
    if (level->stars_model) {
        t3d_model_free(level->stars_model);
    }

    if (level->modelMat) {
        free_uncached(level->modelMat);
    }
    
    if (level->starsMat) {
        free_uncached(level->starsMat);
    }

    rdpq_text_unregister_font(1);
}
