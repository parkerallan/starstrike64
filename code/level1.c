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

    // Load enemy model
    level->enemy_model = t3d_model_load("rom:/enemy1.t3dm");
    if (!level->enemy_model) {
        debugf("WARNING: Failed to load enemy1 model\n");
    } else {
        debugf("Successfully loaded enemy1 model\n");
    }

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

    // Initialize projectile system (speed: 600, lifetime: 3s, normal_cooldown: 0.2s, slash_cooldown: 1.5s)
    projectile_system_init(&level->projectile_system, 1000.0f, 3.0f, 0.2f, 1.5f);

    // Initialize collision system
    collision_system_init(&level->collision_system);
    
    // Extract collision boxes for player model only
    collision_system_extract_from_model(&level->collision_system, level->mecha_model, "PLAYER_", COLLISION_PLAYER);
    
    // Initialize enemy orchestrator (will handle enemy spawning and collision)
    enemy_orchestrator_init(&level->enemy_orchestrator, level->enemy_model, &level->collision_system);
    
    debugf("Collision system initialized with %d boxes\n", level->collision_system.count);
    
    // Initialize hit display
    level->show_enemy_hit = false;
    level->enemy_hit_timer = 0.0f;
    
    // Initialize player health system
    player_health_init(&level->player_health, &level->collision_system);

    // Use pre-loaded font
    level->font = font;
    rdpq_text_register_font(1, level->font);

    // Initialize title animation
    title_animation_init(&level->title_anim, "DEEP SPACE");

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

    // Load and start music
    wav64_open(&level->music, "rom:/HELIOS_EDGE.wav64");
    wav64_set_loop(&level->music, true);
    mixer_ch_set_limits(0, 0, 48000, 0);
    wav64_play(&level->music, 0);
    mixer_ch_set_vol(0, 0.5f, 0.5f);  // Standard volume
}

int level1_update(Level1* level) {
    // Calculate delta time
    float current_time = (float)((double)get_ticks_us() / 1000000.0);
    float delta_time;
    
    // Validate current_time
    if (current_time != current_time || current_time < 0.0f) {
        current_time = level->last_update_time + (1.0f / 60.0f);
    }
    
    if (level->last_update_time == 0.0f) {
        delta_time = 1.0f / 60.0f;
    } else {
        delta_time = current_time - level->last_update_time;
    }
    level->last_update_time = current_time;
    
    // Validate delta_time (check for NaN, negative, or too large)
    if (delta_time != delta_time || delta_time < 0.0f || delta_time > 0.5f) {
        delta_time = 1.0f / 60.0f;
    }
    
    // Ensure delta_time is never zero to prevent division by zero
    if (delta_time < 0.0001f) {
        delta_time = 0.0001f;
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
    joypad_buttons_t btn_held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
    
    // Update player controls
    playercontrols_update(&level->player_controls, inputs, delta_time);
    
    // Update outfit system
    outfit_system_update(&level->outfit_system, delta_time);
    
    // Update enemy orchestrator (handles spawning, movement, and individual enemy systems)
    enemy_orchestrator_update_level1(&level->enemy_orchestrator, delta_time);
    
    // Have enemies shoot projectiles during level 1
    enemy_orchestrator_spawn_projectiles_level1(&level->enemy_orchestrator, &level->projectile_system, delta_time);
    
    // Update title animation
    title_animation_update(&level->title_anim, delta_time);
    
    // Update player health system
    player_health_update(&level->player_health, delta_time);
    
    // Update collision boxes to match current player position
    collision_system_update_boxes_by_type(&level->collision_system, COLLISION_PLAYER, level->modelMat);
    
    // Update projectiles (movement only, no collision yet)
    projectile_system_update(&level->projectile_system, delta_time);
    
    // Manual collision checking for projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* proj = projectile_system_get_projectile(&level->projectile_system, i);
        if (!proj || !proj->active) continue;
        
        if (proj->is_enemy) {
            // Enemy projectile - check collision with player
            char hit_name[64];
            if (!player_health_is_dead(&level->player_health) && 
                collision_system_check_point(&level->collision_system, &proj->position, COLLISION_PLAYER, hit_name)) {
                player_health_take_damage(&level->player_health, 1);
                projectile_system_deactivate(&level->projectile_system, i);
            }
        } else {
            // Player projectile - check collision with enemies
            int hit_enemy_index = -1;
            if (enemy_orchestrator_check_hit(&level->enemy_orchestrator, &proj->position, &hit_enemy_index, proj->damage)) {
                projectile_system_deactivate(&level->projectile_system, i);
            }
        }
    }

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

    // Set model matrix based on player position
    T3DVec3 player_pos = playercontrols_get_position(&level->player_controls);
    float scale[3] = {1.0f, 1.0f, 1.0f};
    float rotation[3] = {0.0f, T3D_DEG_TO_RAD(180.0f), 0.0f};
    float position[3] = {player_pos.v[0], player_pos.v[1], player_pos.v[2]};

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
    
    // Draw active enemies from orchestrator
    if (level->enemy_model) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemy_orchestrator_is_active(&level->enemy_orchestrator, i)) {
                EnemySystem* enemy_sys = enemy_orchestrator_get_system(&level->enemy_orchestrator, i);
                T3DMat4FP* enemy_mat = enemy_orchestrator_get_matrix(&level->enemy_orchestrator, i);
                
                if (!enemy_sys || !enemy_mat) continue;
                
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
    }

    // Draw mecha model if loaded
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

    // Draw UI
    title_animation_render(&level->title_anim, level->font, 1, 70);
    
    // Draw player health
    player_health_render(&level->player_health);
    
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
    
    if (level->enemy_model) {
        t3d_model_free(level->enemy_model);
    }
    
    // Cleanup player health system
    player_health_cleanup(&level->player_health);

    if (level->modelMat) {
        free_uncached(level->modelMat);
    }
    
    if (level->starsMat) {
        free_uncached(level->starsMat);
    }

    enemy_orchestrator_cleanup(&level->enemy_orchestrator);

    projectile_system_cleanup(&level->projectile_system);
    
    collision_system_cleanup(&level->collision_system);

    wav64_close(&level->music);
    rdpq_text_unregister_font(1);
}
