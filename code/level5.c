#include "level5.h"
#include "scenes.h"

void level5_init(Level5* level, rdpq_font_t* font) {
    level->last_update_time = 0.0f;
    level->viewport = t3d_viewport_create();
    level->mecha_model = t3d_model_load("rom:/mecha.t3dm");
    
    // Initialize skeleton
    const T3DChunkSkeleton* skelChunk = t3d_model_get_skeleton(level->mecha_model);
    if (skelChunk && level->mecha_model) {
        level->skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *level->skeleton = t3d_skeleton_create(level->mecha_model);
        animation_system_init(&level->anim_system, level->mecha_model, level->skeleton);
        animation_system_play(&level->anim_system, "Idle", true);
    } else {
        level->skeleton = NULL;
    }
    
    level->modelMat = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(level->modelMat);
    
    // Load mercury map
    level->mercury_model = t3d_model_load("rom:/mercury.t3dm");
    if (!level->mercury_model) {
        debugf("WARNING: Failed to load mercury model\n");
    } else {
        debugf("Successfully loaded mercury model\n");
    }
    
    // Initialize skeleton for mercury animation
    const T3DChunkSkeleton* mercurySkelChunk = t3d_model_get_skeleton(level->mercury_model);
    if (mercurySkelChunk && level->mercury_model) {
        level->mercury_skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *level->mercury_skeleton = t3d_skeleton_create(level->mercury_model);
        animation_system_init(&level->mercury_anim_system, level->mercury_model, level->mercury_skeleton);
        animation_system_play(&level->mercury_anim_system, "Rotate", true);
    } else {
        level->mercury_skeleton = NULL;
    }
    
    level->mercuryMat = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(level->mercuryMat);
    
    level->font = font;
    rdpq_text_register_font(1, level->font);
    
    level->colorAmbient[0] = 180;
    level->colorAmbient[1] = 180;
    level->colorAmbient[2] = 180;
    level->colorAmbient[3] = 0xFF;
    
    level->colorDir[0] = 200;
    level->colorDir[1] = 200;
    level->colorDir[2] = 255;
    level->colorDir[3] = 0xFF;
    
    level->lightDirVec = (T3DVec3){{0.3f, -0.8f, 0.5f}};
    t3d_vec3_norm(&level->lightDirVec);
}

int level5_update(Level5* level) {
    float current_time = (float)((double)get_ticks_us() / 1000000.0);
    float delta_time = (level->last_update_time == 0.0f) ? (1.0f / 60.0f) : (current_time - level->last_update_time);
    level->last_update_time = current_time;
    if (delta_time < 0.0f || delta_time > 0.5f) delta_time = 1.0f / 60.0f;
    
    if (level->skeleton) {
        animation_system_update(&level->anim_system, delta_time);
    }
    
    if (level->mercury_skeleton) {
        animation_system_update(&level->mercury_anim_system, delta_time);
    }
    
    joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (btn.start) return LEVEL_1;  // Loop back to level 1
    if (btn.b) return LEVEL_4;
    
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
    
    return -1;
}

void level5_render(Level5* level) {
    rdpq_attach(display_get(), display_get_zbuf());
    t3d_frame_start();
    t3d_viewport_attach(&level->viewport);
    
    t3d_screen_clear_color(RGBA32(0, 0, 128, 0xFF));
    t3d_screen_clear_depth();
    
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);
    t3d_light_set_ambient(level->colorAmbient);
    t3d_light_set_directional(0, level->colorDir, &level->lightDirVec);
    t3d_light_set_count(1);
    
    if (level->mercury_model) {
        t3d_matrix_push(level->mercuryMat);
        T3DModelDrawConf mercuryDrawConf = {
            .matrices = level->mercury_skeleton ? level->mercury_skeleton->boneMatricesFP : NULL
        };
        t3d_model_draw_custom(level->mercury_model, mercuryDrawConf);
        t3d_matrix_pop(1);
    }
    
    if (level->mecha_model) {
        t3d_matrix_push(level->modelMat);
        T3DModelDrawConf drawConf = {
            .matrices = level->skeleton ? level->skeleton->boneMatricesFP : NULL
        };
        t3d_model_draw_custom(level->mecha_model, drawConf);
        t3d_matrix_pop(1);
    }
    
    rdpq_sync_pipe();
    rdpq_text_printf(NULL, 1, 10, 10, "Mercury");
    rdpq_text_printf(NULL, 1, 10, 30, "Start: Level 1  B: Level 4");
    
    rdpq_detach_show();
}

void level5_cleanup(Level5* level) {
    if (level->skeleton) {
        animation_system_cleanup(&level->anim_system);
        t3d_skeleton_destroy(level->skeleton);
        free_uncached(level->skeleton);
        level->skeleton = NULL;
    }
    
    if (level->mercury_skeleton) {
        animation_system_cleanup(&level->mercury_anim_system);
        t3d_skeleton_destroy(level->mercury_skeleton);
        free_uncached(level->mercury_skeleton);
        level->mercury_skeleton = NULL;
    }
    
    if (level->mecha_model) t3d_model_free(level->mecha_model);
    if (level->mercury_model) t3d_model_free(level->mercury_model);
    if (level->modelMat) free_uncached(level->modelMat);
    if (level->mercuryMat) free_uncached(level->mercuryMat);
    rdpq_text_unregister_font(1);
}
