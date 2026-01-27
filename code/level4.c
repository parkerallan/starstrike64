#include "level4.h"
#include "scenes.h"

void level4_init(Level4* level, rdpq_font_t* font) {
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
    
    // Load sun map
    level->sun_model = t3d_model_load("rom:/sun.t3dm");
    if (!level->sun_model) {
        debugf("WARNING: Failed to load sun model\n");
    } else {
        debugf("Successfully loaded sun model\n");
    }
    
    // Initialize skeleton for sun animation
    const T3DChunkSkeleton* sunSkelChunk = t3d_model_get_skeleton(level->sun_model);
    if (sunSkelChunk && level->sun_model) {
        level->sun_skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *level->sun_skeleton = t3d_skeleton_create(level->sun_model);
        animation_system_init(&level->sun_anim_system, level->sun_model, level->sun_skeleton);
        animation_system_play(&level->sun_anim_system, "Rotate", true);
    } else {
        level->sun_skeleton = NULL;
    }
    
    level->sunMat = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(level->sunMat);
    
    // Load enemy model
    level->enemy_model = t3d_model_load("rom:/enemy1.t3dm");
    if (!level->enemy_model) {
        debugf("WARNING: Failed to load enemy1 model\n");
    } else {
        debugf("Successfully loaded enemy1 model\n");
    }
    
    // Allocate enemy matrix
    level->enemyMat = malloc_uncached(sizeof(T3DMat4FP));
    // Position enemy in front of player
    float enemy_scale[3] = {1.0f, 1.0f, 1.0f};
    float enemy_rotation[3] = {0.0f, 0.0f, 0.0f};
    float enemy_position[3] = {0.0f, -150.0f, -200.0f};
    t3d_mat4fp_from_srt_euler(level->enemyMat, enemy_scale, enemy_rotation, enemy_position);
    
    // Initialize player controls with boundaries
    T3DVec3 start_pos = {{0.0f, -150.0f, 0.0f}};
    PlayerBoundary boundary = {
        .min_x = -150.0f,
        .max_x = 150.0f,
        .min_y = -250.0f,
        .max_y = -50.0f,
        .min_z = -10.0f,
        .max_z = 10.0f
    };
    playercontrols_init(&level->player_controls, start_pos, boundary, 250.0f);
    
    // Initialize outfit system
    outfit_system_init(&level->outfit_system);
    
    // Initialize projectile system (speed: 400, lifetime: 3s, normal_cooldown: 0.2s, slash_cooldown: 1.5s)
    projectile_system_init(&level->projectile_system, 1000.0f, 3.0f, 0.2f, 1.5f);
    
    // Initialize collision system
    collision_system_init(&level->collision_system);
    
    // Extract collision boxes from player model
    collision_system_extract_from_model(&level->collision_system, level->mecha_model, "PLAYER_", COLLISION_PLAYER);
    
    // Initialize enemy orchestrator
    enemy_orchestrator_init(&level->enemy_orchestrator, level->enemy_model, &level->collision_system);
    
    debugf("Collision system initialized with %d boxes\n", level->collision_system.count);
    
    // Initialize hit display
    level->show_player_hit = false;
    level->player_hit_timer = 0.0f;
    
    level->font = font;
    rdpq_text_register_font(1, level->font);
    
    level->colorAmbient[0] = 180;
    level->colorAmbient[1] = 180;
    level->colorAmbient[2] = 180;
    level->colorAmbient[3] = 0xFF;
    
    level->colorDir[0] = 255;
    level->colorDir[1] = 255;
    level->colorDir[2] = 200;
    level->colorDir[3] = 0xFF;
    
    level->lightDirVec = (T3DVec3){{0.3f, -0.8f, 0.5f}};
    t3d_vec3_norm(&level->lightDirVec);
}

int level4_update(Level4* level) {
    float current_time = (float)((double)get_ticks_us() / 1000000.0);
    float delta_time = (level->last_update_time == 0.0f) ? (1.0f / 60.0f) : (current_time - level->last_update_time);
    level->last_update_time = current_time;
    if (delta_time < 0.0f || delta_time > 0.5f) delta_time = 1.0f / 60.0f;
    
    if (level->skeleton) {
        animation_system_update(&level->anim_system, delta_time);
    }
    
    if (level->sun_skeleton) {
        animation_system_update(&level->sun_anim_system, delta_time);
    }
    
    joypad_buttons_t btn_held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
    
    // Update player controls
    playercontrols_update(&level->player_controls, inputs, delta_time);
    
    // Update outfit system
    outfit_system_update(&level->outfit_system, delta_time);
    
    // Update enemy orchestrator (handles spawning, movement, and individual enemy systems)
    enemy_orchestrator_update_level2(&level->enemy_orchestrator, delta_time);
    
    // Update player hit timer
    if (level->player_hit_timer > 0.0f) {
        level->player_hit_timer -= delta_time;
        if (level->player_hit_timer <= 0.0f) {
            level->show_player_hit = false;
        }
    }
    
    // Manual projectile collision checking with each enemy
    for (int p = 0; p < MAX_PROJECTILES; p++) {
        Projectile* proj = projectile_system_get_projectile(&level->projectile_system, p);
        if (proj && proj->active) {
            T3DVec3 proj_pos = proj->position;
            int enemy_index = -1;
            int damage = (proj->type == PROJECTILE_SLASH) ? 3 : 1;
            
            // Check if projectile hits any enemy
            if (enemy_orchestrator_check_hit(&level->enemy_orchestrator, &proj_pos, &enemy_index, damage)) {
                // Hit detected, deactivate projectile
                projectile_system_deactivate(&level->projectile_system, p);
            }
        }
    }
    
    // Update projectile system (movement and rendering)
    projectile_system_update(&level->projectile_system, delta_time);
    
    // A button - shoot slash projectile (hold for continuous fire)
    if (btn_held.a && projectile_system_can_shoot(&level->projectile_system, PROJECTILE_SLASH)) {
        T3DVec3 player_pos = playercontrols_get_position(&level->player_controls);
        T3DVec3 spawn_pos = {{player_pos.v[0], player_pos.v[1] + 100.0f, player_pos.v[2]}};
        T3DVec3 shoot_direction = {{0.0f, 0.0f, -1.0f}};
        projectile_system_spawn(&level->projectile_system, spawn_pos, shoot_direction, PROJECTILE_SLASH);
        // Activate thrust outfit for 1.5 seconds
        outfit_system_activate_thrust(&level->outfit_system, 1.5f);
    }
    
    // B button - shoot normal projectile (hold for continuous fire)
    if (btn_held.b && projectile_system_can_shoot(&level->projectile_system, PROJECTILE_NORMAL)) {
        T3DVec3 player_pos = playercontrols_get_position(&level->player_controls);
        // Offset spawn position to the right and higher (where gun would be)
        T3DVec3 spawn_pos = {{player_pos.v[0], player_pos.v[1] + 100.0f, player_pos.v[2]}};
        T3DVec3 shoot_direction = {{0.0f, 0.0f, -1.0f}};  // Forward direction
        projectile_system_spawn(&level->projectile_system, spawn_pos, shoot_direction, PROJECTILE_NORMAL);
    }
    
    // if (btn.start) return LEVEL_5;
    // if (btn.b) return LEVEL_3;
    
    // Set up camera
    const T3DVec3 camPos = {{0, 0.0f, 200.0f}};
    const T3DVec3 camTarget = {{0, -50.0f, 0}};

    t3d_viewport_set_projection(&level->viewport, T3D_DEG_TO_RAD(60.0f), 20.0f, 1000.0f);
    t3d_viewport_look_at(&level->viewport, &camPos, &camTarget, &(T3DVec3){{0,1,0}});

    // Set model matrix based on player position
    T3DVec3 player_pos = playercontrols_get_position(&level->player_controls);
    float scale[3] = {1.0f, 1.0f, 1.0f};
    float rotation[3] = {0.0f, T3D_DEG_TO_RAD(180.0f), 0.0f};
    float position[3] = {player_pos.v[0], player_pos.v[1], player_pos.v[2]};

    t3d_mat4fp_from_srt_euler(level->modelMat, scale, rotation, position);

    return -1;
}

void level4_render(Level4* level) {
    rdpq_attach(display_get(), display_get_zbuf());
    t3d_frame_start();
    t3d_viewport_attach(&level->viewport);
    
    t3d_screen_clear_color(RGBA32(200, 200, 50, 0xFF));
    t3d_screen_clear_depth();
    
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_TEXTURED | T3D_FLAG_DEPTH);
    t3d_light_set_ambient(level->colorAmbient);
    t3d_light_set_directional(0, level->colorDir, &level->lightDirVec);
    t3d_light_set_count(1);
    
    if (level->sun_model) {
        t3d_matrix_push(level->sunMat);
        T3DModelDrawConf sunDrawConf = {
            .matrices = level->sun_skeleton ? level->sun_skeleton->boneMatricesFP : NULL
        };
        t3d_model_draw_custom(level->sun_model, sunDrawConf);
        t3d_matrix_pop(1);
    }
    
    // Draw all active enemies from orchestrator
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemy_orchestrator_is_active(&level->enemy_orchestrator, i)) continue;
        
        EnemySystem* enemy_sys = enemy_orchestrator_get_system(&level->enemy_orchestrator, i);
        T3DMat4FP* enemy_mat = enemy_orchestrator_get_matrix(&level->enemy_orchestrator, i);
        
        if (enemy_sys && enemy_mat) {
            // Apply red lighting if flashing
            if (enemy_system_is_flashing(enemy_sys)) {
                uint8_t flashColor[4] = {255, 80, 80, 0xFF};
                t3d_light_set_ambient(flashColor);
                t3d_light_set_directional(0, flashColor, &level->lightDirVec);
            }
            
            t3d_matrix_push(enemy_mat);
            
            T3DModelDrawConf enemyDrawConf = {
                .userData = NULL,
                .tileCb = NULL,
                .filterCb = NULL,
                .dynTextureCb = NULL,
                .matrices = NULL
            };
            
            t3d_model_draw_custom(level->enemy_model, enemyDrawConf);
            
            t3d_matrix_pop(1);
            
            // Restore normal lighting
            if (enemy_system_is_flashing(enemy_sys)) {
                t3d_light_set_ambient(level->colorAmbient);
                t3d_light_set_directional(0, level->colorDir, &level->lightDirVec);
            }
        }
    }
    
    if (level->mecha_model) {
        t3d_matrix_push(level->modelMat);
        T3DModelDrawConf drawConf = {
            .userData = &level->outfit_system,
            .tileCb = NULL,
            .filterCb = outfit_system_filter_callback,
            .dynTextureCb = NULL,
            .matrices = level->skeleton ? level->skeleton->boneMatricesFP : NULL
        };
        t3d_model_draw_custom(level->mecha_model, drawConf);
        t3d_matrix_pop(1);
    }
    
    // Draw projectiles
    projectile_system_render(&level->projectile_system);
    
    rdpq_sync_pipe();
    rdpq_text_printf(NULL, 1, 10, 10, "SUN");
    rdpq_detach_show();
}

void level4_cleanup(Level4* level) {
    if (level->skeleton) {
        animation_system_cleanup(&level->anim_system);
        t3d_skeleton_destroy(level->skeleton);
        free_uncached(level->skeleton);
        level->skeleton = NULL;
    }
    
    if (level->sun_skeleton) {
        animation_system_cleanup(&level->sun_anim_system);
        t3d_skeleton_destroy(level->sun_skeleton);
        free_uncached(level->sun_skeleton);
        level->sun_skeleton = NULL;
    }
    
    if (level->mecha_model) t3d_model_free(level->mecha_model);
    if (level->sun_model) t3d_model_free(level->sun_model);
    if (level->enemy_model) t3d_model_free(level->enemy_model);
    if (level->modelMat) free_uncached(level->modelMat);
    if (level->sunMat) free_uncached(level->sunMat);
    if (level->enemyMat) free_uncached(level->enemyMat);
    projectile_system_cleanup(&level->projectile_system);
    
    collision_system_cleanup(&level->collision_system);
    
    rdpq_text_unregister_font(1);
}
