/**
 * @file enemyorchestrator.c
 * @brief Enemy spawning and movement orchestrator for shmup levels
 */

#include "enemyorchestrator.h"
#include "projectilesystem.h"
#include <stdlib.h>
#include <string.h>
#define M_PI 3.14159265358979323846

void enemy_orchestrator_init(EnemyOrchestrator* orch, T3DModel* enemy_model, CollisionSystem* collision_system) {
    orch->enemy_model = enemy_model;
    orch->bomber_model = NULL;  // Will be loaded for level 2
    orch->bomber_skeleton = NULL;
    orch->bomber_anim_system = NULL;  // Pointer, allocated only for level 2
    orch->collision_system = collision_system;
    orch->elapsed_time = 0.0f;
    orch->last_spawn_time = 0.0f;
    orch->active_count = 0;
    orch->wave_count = 0;
    orch->bomber_phase = 0;
    orch->bomber_phase_timer = 0.0f;
    
    // Load explosion model
    orch->explosion_model = t3d_model_load("rom:/explosion.t3dm");
    if (!orch->explosion_model) {
        debugf("WARNING: Failed to load enemy explosion model\n");
    }
    
    // Allocate explosion matrices
    orch->explosion_matrices = malloc_uncached(sizeof(T3DMat4FP*) * MAX_ENEMIES);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        orch->explosion_matrices[i] = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(orch->explosion_matrices[i]);
    }
    
    // Initialize all enemy instances
    for (int i = 0; i < MAX_ENEMIES; i++) {
        orch->enemies[i].matrix = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(orch->enemies[i].matrix);
        orch->enemies[i].active = false;
        orch->enemies[i].spawn_time = 0.0f;
        orch->enemies[i].collision_start_index = -1;
        orch->enemies[i].collision_count = 0;
        orch->enemies[i].movement_phase = 0;
        orch->enemies[i].phase_timer = 0.0f;
        orch->enemies[i].shoot_timer = 0.0f;
        orch->enemies[i].has_explosion = false;
        orch->enemies[i].explosion_timer = 0.0f;
    }
}

void enemy_orchestrator_spawn_enemy(
    EnemyOrchestrator* orch,
    float x, float y, float z,
    float vel_x, float vel_y, float vel_z
) {
    // Find inactive slot
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) {
            EnemyInstance* enemy = &orch->enemies[i];
            
            // Set position and velocity
            enemy->position = (T3DVec3){{x, y, z}};
            enemy->velocity = (T3DVec3){{vel_x, vel_y, vel_z}};
            enemy->spawn_time = orch->elapsed_time;
            
            // Set up transform matrix (use larger scale for bomber)
            float scale[3] = {1.0f, 1.0f, 1.0f};
            if (orch->bomber_model != NULL) {
                scale[0] = scale[1] = scale[2] = 2.5f;  // Bomber is larger
            }
            float rotation[3] = {0.0f, 0.0f, 0.0f};
            float position[3] = {x, y, z};
            t3d_mat4fp_from_srt_euler(enemy->matrix, scale, rotation, position);
            
            // Extract collision boxes for this enemy (use bomber model if available, otherwise standard enemy)
            T3DModel* collision_model = (orch->bomber_model != NULL) ? orch->bomber_model : orch->enemy_model;
            int collision_before = orch->collision_system->count;
            collision_system_extract_from_model(orch->collision_system, collision_model, "ENEMY_", COLLISION_ENEMY);
            enemy->collision_start_index = collision_before;
            enemy->collision_count = orch->collision_system->count - collision_before;
            
            // Immediately update collision boxes to spawn position
            collision_system_update_boxes_by_range(orch->collision_system, 
                                                   enemy->collision_start_index, 
                                                   enemy->collision_count, 
                                                   enemy->matrix);
            
            // Initialize enemy system with health
            int enemy_health = collision_system_get_enemy_health(orch->collision_system);
            enemy_system_init(&enemy->system, enemy_health);
            
            enemy->active = true;
            enemy->show_hit = false;
            enemy->hit_timer = 0.0f;
            enemy->shoot_timer = 0.0f;
            enemy->has_explosion = false;
            enemy->explosion_timer = 0.0f;
            orch->active_count++;
            
            debugf("Spawned enemy %d at (%.1f, %.1f, %.1f) with %d collision boxes\n", 
                   i, x, y, z, enemy->collision_count);
            
            return;
        }
    }
    
    debugf("WARNING: No enemy slots available!\n");
}

/**
 * Check if a point hits any enemy and apply damage
 * Returns true if hit, sets enemy_index to the hit enemy
 */
bool enemy_orchestrator_check_hit(EnemyOrchestrator* orch, const T3DVec3* position, int* enemy_index, int damage) {
    if (!orch || !position) return false;
    
    // Check collision against each active enemy's collision boxes
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* enemy = &orch->enemies[i];
        
        // Check collision boxes for this enemy
        for (int j = enemy->collision_start_index; j < enemy->collision_start_index + enemy->collision_count; j++) {
            if (j >= orch->collision_system->count) break;
            
            CollisionAABB* box = &orch->collision_system->boxes[j];
            if (!box->active || box->type != COLLISION_ENEMY) continue;
            
            // Check if point is inside this box
            if (position->v[0] >= box->min[0] && position->v[0] <= box->max[0] &&
                position->v[1] >= box->minY && position->v[1] <= box->maxY &&
                position->v[2] >= box->min[1] && position->v[2] <= box->max[1]) {
                
                // Hit detected! Apply damage
                enemy->show_hit = true;
                enemy->hit_timer = 0.5f;
                
                // Reduce health
                enemy->system.health -= damage;
                enemy->system.last_damage_taken = damage;
                enemy->system.flash_timer = enemy->system.flash_duration;
                
                if (enemy->system.health <= 0) {
                    enemy->system.active = false;
                    enemy->has_explosion = true;
                    enemy->explosion_timer = 0.25f;
                    enemy->explosion_position = enemy->position;
                    
                    debugf("*** EXPLOSION %d CREATED at (%.1f, %.1f, %.1f) timer=1.0\n", 
                           i, enemy->explosion_position.v[0], enemy->explosion_position.v[1], enemy->explosion_position.v[2]);
                    
                    // Initialize explosion matrix immediately
                    float exp_scale[3] = {1.0f, 1.0f, 1.0f};
                    float exp_rotation[3] = {0.0f, 0.0f, 0.0f};
                    float exp_position[3] = {enemy->explosion_position.v[0], 
                                            enemy->explosion_position.v[1], 
                                            enemy->explosion_position.v[2]};
                    t3d_mat4fp_from_srt_euler(orch->explosion_matrices[i], exp_scale, exp_rotation, exp_position);
                    
                    // Deactivate collision boxes
                    for (int k = enemy->collision_start_index; k < enemy->collision_start_index + enemy->collision_count; k++) {
                        if (k < orch->collision_system->count) {
                            orch->collision_system->boxes[k].active = false;
                        }
                    }
                }
                
                if (enemy_index) *enemy_index = i;
                return true;
            }
        }
    }
    
    return false;
}

/**
 * Level 1 enemy pattern - Realistic curved spaceship attack patterns
 * Enemies curve in from different directions using smooth bezier-like curves,
 * decelerate into position, hold formation while attacking, then accelerate away.
 * 5 waves total with varied approach vectors.
 */
void enemy_orchestrator_update_level1(EnemyOrchestrator* orch, float delta_time) {
    // Validate delta_time
    if (delta_time <= 0.0f || delta_time != delta_time || delta_time > 1.0f) {
        delta_time = 0.0001f;
    }
    
    orch->elapsed_time += delta_time;
    
    // Spawn 5 waves of enemies with varied approach patterns
    if (orch->wave_count < 5 && 
        orch->elapsed_time - orch->last_spawn_time > 5.0f &&
        orch->active_count == 0) {
        
        int wave = orch->wave_count;
        
        // Wave patterns alternate between different approach vectors
        switch (wave % 3) {
            case 0: // Arc from top-right, curving down and forward
            {
                // Enemy 1: High right arc
                enemy_orchestrator_spawn_enemy(orch, 300.0f, 50.0f, -400.0f, -120.0f, -80.0f, 100.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{-100.0f, -90.0f, -250.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                
                // Enemy 2: Mid-right arc
                enemy_orchestrator_spawn_enemy(orch, 350.0f, 20.0f, -450.0f, -130.0f, -70.0f, 110.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{0.0f, -100.0f, -240.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                
                // Enemy 3: Lower right arc
                enemy_orchestrator_spawn_enemy(orch, 400.0f, -10.0f, -480.0f, -140.0f, -60.0f, 120.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{100.0f, -110.0f, -230.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                break;
            }
            
            case 1: // Arc from top-left, curving down and forward
            {
                // Enemy 1: High left arc
                enemy_orchestrator_spawn_enemy(orch, -300.0f, 60.0f, -420.0f, 110.0f, -85.0f, 105.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{100.0f, -85.0f, -255.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                
                // Enemy 2: Mid-left arc  
                enemy_orchestrator_spawn_enemy(orch, -350.0f, 30.0f, -460.0f, 125.0f, -75.0f, 115.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{0.0f, -100.0f, -245.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                
                // Enemy 3: Lower left arc
                enemy_orchestrator_spawn_enemy(orch, -380.0f, 0.0f, -490.0f, 135.0f, -65.0f, 125.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{-100.0f, -115.0f, -235.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                break;
            }
            
            case 2: // Pincer from both sides, converging on center
            {
                // Left pincer
                enemy_orchestrator_spawn_enemy(orch, -400.0f, -50.0f, -350.0f, 130.0f, -30.0f, 90.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{-80.0f, -95.0f, -240.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                
                // Center approach
                enemy_orchestrator_spawn_enemy(orch, 0.0f, 80.0f, -500.0f, 0.0f, -90.0f, 130.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{0.0f, -100.0f, -250.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                
                // Right pincer
                enemy_orchestrator_spawn_enemy(orch, 400.0f, -50.0f, -350.0f, -130.0f, -30.0f, 90.0f);
                orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{80.0f, -95.0f, -240.0f}};
                orch->enemies[orch->active_count - 1].movement_phase = 0;
                orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
                break;
            }
        }
        
        orch->last_spawn_time = orch->elapsed_time;
        orch->wave_count++;
    }
    
    // Update all active enemies with realistic curved movement
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* enemy = &orch->enemies[i];
        enemy->phase_timer += delta_time;
        
        // Validate enemy positions to prevent NaN
        if (enemy->position.v[0] != enemy->position.v[0] || 
            enemy->position.v[1] != enemy->position.v[1] ||
            enemy->position.v[2] != enemy->position.v[2]) {
            // Position is NaN, deactivate enemy
            enemy->active = false;
            orch->active_count--;
            continue;
        }
        
        switch (enemy->movement_phase) {
            case 0: // Curved approach with deceleration
            {
                // Calculate distance to target
                float dx = enemy->target_position.v[0] - enemy->position.v[0];
                float dy = enemy->target_position.v[1] - enemy->position.v[1];
                float dz = enemy->target_position.v[2] - enemy->position.v[2];
                float distance = sqrtf(dx*dx + dy*dy + dz*dz);
                
                if (distance < 10.0f) {
                    // Arrived at target - transition to hold phase
                    enemy->position = enemy->target_position;
                    enemy->velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};
                    enemy->movement_phase = 1;
                    enemy->phase_timer = 0.0f;
                } else {
                    // Smooth deceleration curve using quadratic easing
                    // Ships slow down as they approach target (realistic physics)
                    float progress = 1.0f - (distance / 600.0f); // Normalize based on typical approach distance
                    if (progress < 0.0f) progress = 0.0f;
                    if (progress > 1.0f) progress = 1.0f;
                    
                    // Ease-out quadratic: fast approach, slow arrival
                    float ease = 1.0f - (1.0f - progress) * (1.0f - progress);
                    float decel_factor = 0.3f + (1.0f - ease) * 0.7f; // Range: 0.3 to 1.0
                    
                    // Apply curved velocity with deceleration
                    // Normalize direction
                    float inv_dist = 1.0f / distance;
                    float dir_x = dx * inv_dist;
                    float dir_y = dy * inv_dist;
                    float dir_z = dz * inv_dist;
                    
                    // Base speed scales with deceleration
                    float base_speed = 200.0f * decel_factor;
                    
                    // Smooth interpolation toward target direction (banking turn effect)
                    float turn_rate = 2.0f * delta_time; // How quickly ship can turn
                    float current_speed = sqrtf(enemy->velocity.v[0]*enemy->velocity.v[0] + 
                                               enemy->velocity.v[1]*enemy->velocity.v[1] + 
                                               enemy->velocity.v[2]*enemy->velocity.v[2]);
                    
                    if (current_speed > 0.01f) {
                        float inv_current = 1.0f / current_speed;
                        float current_dir_x = enemy->velocity.v[0] * inv_current;
                        float current_dir_y = enemy->velocity.v[1] * inv_current;
                        float current_dir_z = enemy->velocity.v[2] * inv_current;
                        
                        // Smoothly blend current direction toward target direction
                        float blend_x = current_dir_x + (dir_x - current_dir_x) * turn_rate;
                        float blend_y = current_dir_y + (dir_y - current_dir_y) * turn_rate;
                        float blend_z = current_dir_z + (dir_z - current_dir_z) * turn_rate;
                        
                        // Renormalize
                        float blend_len = sqrtf(blend_x*blend_x + blend_y*blend_y + blend_z*blend_z);
                        if (blend_len > 0.01f) {
                            float inv_blend = 1.0f / blend_len;
                            blend_x *= inv_blend;
                            blend_y *= inv_blend;
                            blend_z *= inv_blend;
                        }
                        
                        enemy->velocity.v[0] = blend_x * base_speed;
                        enemy->velocity.v[1] = blend_y * base_speed;
                        enemy->velocity.v[2] = blend_z * base_speed;
                    } else {
                        // Initialize velocity if stationary
                        enemy->velocity.v[0] = dir_x * base_speed;
                        enemy->velocity.v[1] = dir_y * base_speed;
                        enemy->velocity.v[2] = dir_z * base_speed;
                    }
                    
                    // Update position
                    enemy->position.v[0] += enemy->velocity.v[0] * delta_time;
                    enemy->position.v[1] += enemy->velocity.v[1] * delta_time;
                    enemy->position.v[2] += enemy->velocity.v[2] * delta_time;
                }
                break;
            }
            
            case 1: // Hold position and attack
            {
                // Stay in formation for 4 seconds while shooting
                if (enemy->phase_timer > 4.0f) {
                    // Choose exit direction based on current position
                    // Ships exit away from center in varied directions
                    float exit_x = enemy->position.v[0] > 0.0f ? 200.0f : -200.0f;
                    float exit_y = -100.0f; // Slightly down
                    float exit_z = -150.0f; // Move back into distance
                    
                    enemy->velocity = (T3DVec3){{exit_x, exit_y, exit_z}};
                    enemy->movement_phase = 2;
                    enemy->phase_timer = 0.0f;
                }
                break;
            }
            
            case 2: // Exit with acceleration
            {
                // Accelerate away from combat zone (realistic thrust increase)
                float accel_factor = 1.0f + enemy->phase_timer * 0.8f; // Accelerate over time
                if (accel_factor > 2.5f) accel_factor = 2.5f; // Cap acceleration
                
                enemy->position.v[0] += enemy->velocity.v[0] * accel_factor * delta_time;
                enemy->position.v[1] += enemy->velocity.v[1] * accel_factor * delta_time;
                enemy->position.v[2] += enemy->velocity.v[2] * accel_factor * delta_time;
                
                // Deactivate when far enough away
                float dist_from_center = sqrtf(enemy->position.v[0]*enemy->position.v[0] + 
                                               enemy->position.v[1]*enemy->position.v[1]);
                
                if (dist_from_center > 600.0f || enemy->position.v[2] < -600.0f) {
                    for (int j = enemy->collision_start_index; j < enemy->collision_start_index + enemy->collision_count; j++) {
                        if (j < orch->collision_system->count) {
                            orch->collision_system->boxes[j].active = false;
                        }
                    }
                    enemy->active = false;
                    orch->active_count--;
                }
                break;
            }
        }
        
        // Update transform matrix
        float scale[3] = {1.0f, 1.0f, 1.0f};
        float rotation[3] = {0.0f, 0.0f, 0.0f};
        float position[3] = {enemy->position.v[0], enemy->position.v[1], enemy->position.v[2]};
        t3d_mat4fp_from_srt_euler(enemy->matrix, scale, rotation, position);
        
        // Update collision boxes
        collision_system_update_boxes_by_range(orch->collision_system, 
                                               enemy->collision_start_index, 
                                               enemy->collision_count, 
                                               enemy->matrix);
        
        // Update hit timer
        if (enemy->hit_timer > 0.0f) {
            enemy->hit_timer -= delta_time;
            if (enemy->hit_timer <= 0.0f) enemy->show_hit = false;
        }
        
        // Update flash timer
        if (enemy->system.flash_timer > 0.0f) {
            enemy->system.flash_timer -= delta_time;
            if (enemy->system.flash_timer < 0.0f) enemy->system.flash_timer = 0.0f;
        }
        
        // Deactivate if destroyed
        if (!enemy_system_is_active(&enemy->system)) {
            for (int j = enemy->collision_start_index; j < enemy->collision_start_index + enemy->collision_count; j++) {
                if (j < orch->collision_system->count) {
                    orch->collision_system->boxes[j].active = false;
                }
            }
            enemy->active = false;
            orch->active_count--;
        }
    }
    
    // Update explosions
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (orch->enemies[i].has_explosion) {
            EnemyInstance* enemy = &orch->enemies[i];
            enemy->explosion_timer -= delta_time;
            
            if (enemy->explosion_timer <= 0.0f) {
                enemy->has_explosion = false;
                float far_scale[3] = {0.0f, 0.0f, 0.0f};
                float far_rotation[3] = {0.0f, 0.0f, 0.0f};
                float far_position[3] = {0.0f, -10000.0f, 0.0f};
                t3d_mat4fp_from_srt_euler(orch->explosion_matrices[i], far_scale, far_rotation, far_position);
            }
        }
    }
}

/**
 * Level 2 enemy pattern - Single bomber with alternating attack phases
 * Phase 0: Retreat - fly far away
 * Phase 1: Approach - fly back in aggressively  
 * Phase 2: Strafe - rapid fire attack run
 * Phase 3: Transition to wave - smoothly move to wave position
 * Phase 4: Wave pattern - side-to-side barrage
 */
void enemy_orchestrator_update_level2(EnemyOrchestrator* orch, float delta_time) {
    // Load bomber model and setup animation on first call
    if (!orch->bomber_model) {
        orch->bomber_model = t3d_model_load("rom:/enemy2.t3dm");
        if (!orch->bomber_model) {
            debugf("ERROR: Failed to load enemy2.t3dm bomber model\n");
            return;
        }
        
        // Initialize skeleton and animation for bomber
        const T3DChunkSkeleton* skelChunk = t3d_model_get_skeleton(orch->bomber_model);
        if (skelChunk) {
            orch->bomber_skeleton = malloc_uncached(sizeof(T3DSkeleton));
            *orch->bomber_skeleton = t3d_skeleton_create(orch->bomber_model);
            orch->bomber_anim_system = malloc_uncached(sizeof(AnimationSystem));
            animation_system_init(orch->bomber_anim_system, orch->bomber_model, orch->bomber_skeleton);
            animation_system_play(orch->bomber_anim_system, "spin", true);  // Loop spin animation
        }
    }
    
    // Update bomber animation
    if (orch->bomber_skeleton && orch->bomber_anim_system) {
        animation_system_update(orch->bomber_anim_system, delta_time);
    }
    
    orch->elapsed_time += delta_time;
    
    // Spawn bomber if none active
    if (orch->active_count == 0 && orch->wave_count == 0) {
        // Spawn bomber at distant starting position
        enemy_orchestrator_spawn_enemy(orch, 0.0f, -20.0f, -800.0f, 0.0f, 0.0f, 0.0f);
        if (orch->active_count > 0) {
            EnemyInstance* bomber = &orch->enemies[0];
            bomber->movement_phase = 0;
            bomber->phase_timer = 0.0f;
            // Health is already set from collision box name (ENEMY_bomber_10 = 10 health)
            orch->bomber_phase = 0;  // Start with retreat
            orch->bomber_phase_timer = 0.0f;
            orch->wave_count = 1;
        }
    }
    
    // Update bomber behavior with realistic physics
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* bomber = &orch->enemies[i];
        bomber->phase_timer += delta_time;
        orch->bomber_phase_timer += delta_time;
        
        switch (orch->bomber_phase) {
            case 0: // Retreat - fly back to distant position
            {
                T3DVec3 retreat_target = {{0.0f, 0.0f, -1000.0f}};
                T3DVec3 to_target = {{
                    retreat_target.v[0] - bomber->position.v[0],
                    retreat_target.v[1] - bomber->position.v[1],
                    retreat_target.v[2] - bomber->position.v[2]
                }};
                
                float dist = sqrtf(to_target.v[0]*to_target.v[0] + to_target.v[1]*to_target.v[1] + to_target.v[2]*to_target.v[2]);
                
                if (dist < 20.0f || bomber->phase_timer > 3.0f) {
                    // Reached retreat position, prepare to charge
                    bomber->position = retreat_target;
                    bomber->velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};
                    orch->bomber_phase = 1;
                    bomber->phase_timer = 0.0f;
                } else {
                    // Accelerate away with easing
                    float speed = 180.0f + (bomber->phase_timer * 60.0f);  // Accelerate from 180 to 240
                    if (speed > 240.0f) speed = 240.0f;
                    
                    bomber->velocity.v[0] = (to_target.v[0] / dist) * speed;
                    bomber->velocity.v[1] = (to_target.v[1] / dist) * speed;
                    bomber->velocity.v[2] = (to_target.v[2] / dist) * speed;
                    
                    bomber->position.v[0] += bomber->velocity.v[0] * delta_time;
                    bomber->position.v[1] += bomber->velocity.v[1] * delta_time;
                    bomber->position.v[2] += bomber->velocity.v[2] * delta_time;
                }
                break;
            }
            
            case 1: // Approach - charge in aggressively
            {
                T3DVec3 approach_target = {{0.0f, -120.0f, -250.0f}};  // Close to player zone
                T3DVec3 to_target = {{
                    approach_target.v[0] - bomber->position.v[0],
                    approach_target.v[1] - bomber->position.v[1],
                    approach_target.v[2] - bomber->position.v[2]
                }};
                
                float dist = sqrtf(to_target.v[0]*to_target.v[0] + to_target.v[1]*to_target.v[1] + to_target.v[2]*to_target.v[2]);
                
                if (dist < 30.0f) {
                    // Reached attack position, begin strafe
                    orch->bomber_phase = 2;
                    bomber->phase_timer = 0.0f;
                } else {
                    // Aggressive approach with realistic acceleration
                    float progress = bomber->phase_timer / 2.5f;
                    if (progress > 1.0f) progress = 1.0f;
                    
                    // Ease-in-out for smooth acceleration/deceleration
                    float ease = progress < 0.5f ? 
                        2.0f * progress * progress : 
                        1.0f - powf(-2.0f * progress + 2.0f, 2.0f) / 2.0f;
                    
                    float speed = 280.0f * ease;  // Max 280 speed
                    
                    bomber->velocity.v[0] = (to_target.v[0] / dist) * speed;
                    bomber->velocity.v[1] = (to_target.v[1] / dist) * speed;
                    bomber->velocity.v[2] = (to_target.v[2] / dist) * speed;
                    
                    bomber->position.v[0] += bomber->velocity.v[0] * delta_time;
                    bomber->position.v[1] += bomber->velocity.v[1] * delta_time;
                    bomber->position.v[2] += bomber->velocity.v[2] * delta_time;
                }
                break;
            }
            
            case 2: // Strafe - rapid shooting attack
            {
                // Strafe side-to-side while shooting
                float strafe_speed = 120.0f;
                float strafe_pattern = sinf(bomber->phase_timer * 3.0f);
                
                bomber->velocity.v[0] = strafe_pattern * strafe_speed;
                bomber->velocity.v[1] = -15.0f * sinf(bomber->phase_timer * 2.0f);  // Slight bobbing
                bomber->velocity.v[2] = 0.0f;
                
                bomber->position.v[0] += bomber->velocity.v[0] * delta_time;
                bomber->position.v[1] += bomber->velocity.v[1] * delta_time;
                bomber->position.v[2] += bomber->velocity.v[2] * delta_time;
                
                // Clamp X position to stay in view
                if (bomber->position.v[0] < -180.0f) bomber->position.v[0] = -180.0f;
                if (bomber->position.v[0] > 180.0f) bomber->position.v[0] = 180.0f;
                
                // After 4 seconds of strafing, transition to wave pattern
                if (bomber->phase_timer >= 4.0f) {
                    orch->bomber_phase = 3;
                    bomber->phase_timer = 0.0f;
                }
                break;
            }
            
            case 3: // Transition to wave pattern position
            {
                T3DVec3 wave_target = {{-200.0f, -40.0f, -420.0f}};
                T3DVec3 to_target = {{
                    wave_target.v[0] - bomber->position.v[0],
                    wave_target.v[1] - bomber->position.v[1],
                    wave_target.v[2] - bomber->position.v[2]
                }};
                
                float dist = sqrtf(to_target.v[0]*to_target.v[0] + to_target.v[1]*to_target.v[1] + to_target.v[2]*to_target.v[2]);
                
                if (dist < 25.0f) {
                    orch->bomber_phase = 4;
                    bomber->phase_timer = 0.0f;
                    orch->bomber_phase_timer = 0.0f;
                } else {
                    // Smooth transition with deceleration
                    float speed = 160.0f * (1.0f - (bomber->phase_timer / 2.0f));
                    if (speed < 80.0f) speed = 80.0f;
                    
                    bomber->velocity.v[0] = (to_target.v[0] / dist) * speed;
                    bomber->velocity.v[1] = (to_target.v[1] / dist) * speed;
                    bomber->velocity.v[2] = (to_target.v[2] / dist) * speed;
                    
                    bomber->position.v[0] += bomber->velocity.v[0] * delta_time;
                    bomber->position.v[1] += bomber->velocity.v[1] * delta_time;
                    bomber->position.v[2] += bomber->velocity.v[2] * delta_time;
                }
                break;
            }
            
            case 4: // Wave pattern - side-to-side barrage
            {
                // Smooth sinusoidal movement
                float wave_amplitude = 200.0f;
                float wave_frequency = 1.5f;
                float target_x = sinf(orch->bomber_phase_timer * wave_frequency) * wave_amplitude;
                
                // Smooth velocity interpolation
                float current_x = bomber->position.v[0];
                float x_diff = target_x - current_x;
                bomber->velocity.v[0] = x_diff * 3.0f;  // Smooth tracking
                
                bomber->position.v[0] += bomber->velocity.v[0] * delta_time;
                bomber->position.v[1] = -40.0f + sinf(orch->bomber_phase_timer * 0.8f) * 15.0f;  // Gentle bobbing
                bomber->position.v[2] = -420.0f;
                
                // After 7 seconds, cycle back to retreat
                if (bomber->phase_timer >= 7.0f) {
                    orch->bomber_phase = 0;
                    bomber->phase_timer = 0.0f;
                    orch->bomber_phase_timer = 0.0f;
                }
                break;
            }
        }
        
        // Update transform matrix with animation bones
        float scale[3] = {2.5f, 2.5f, 2.5f};  // Bomber is larger
        float rotation[3] = {0.0f, 0.0f, 0.0f};  // Face player
        float position[3] = {bomber->position.v[0], bomber->position.v[1], bomber->position.v[2]};
        t3d_mat4fp_from_srt_euler(bomber->matrix, scale, rotation, position);
        
        // Update collision boxes
        collision_system_update_boxes_by_range(orch->collision_system, 
                                               bomber->collision_start_index, 
                                               bomber->collision_count, 
                                               bomber->matrix);
        
        // Update hit timer
        if (bomber->hit_timer > 0.0f) {
            bomber->hit_timer -= delta_time;
            if (bomber->hit_timer <= 0.0f) bomber->show_hit = false;
        }
        
        // Update flash timer
        if (bomber->system.flash_timer > 0.0f) {
            bomber->system.flash_timer -= delta_time;
            if (bomber->system.flash_timer < 0.0f) bomber->system.flash_timer = 0.0f;
        }
        
        // Deactivate if destroyed
        if (!enemy_system_is_active(&bomber->system)) {
            for (int j = bomber->collision_start_index; j < bomber->collision_start_index + bomber->collision_count; j++) {
                if (j < orch->collision_system->count) {
                    orch->collision_system->boxes[j].active = false;
                }
            }
            bomber->active = false;
            orch->active_count--;
        }
    }
    
    // Update explosions
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (orch->enemies[i].has_explosion) {
            EnemyInstance* enemy = &orch->enemies[i];
            enemy->explosion_timer -= delta_time;
            
            if (enemy->explosion_timer <= 0.0f) {
                enemy->has_explosion = false;
                float far_scale[3] = {0.0f, 0.0f, 0.0f};
                float far_rotation[3] = {0.0f, 0.0f, 0.0f};
                float far_position[3] = {0.0f, -10000.0f, 0.0f};
                t3d_mat4fp_from_srt_euler(orch->explosion_matrices[i], far_scale, far_rotation, far_position);
            }
        }
    }
}

/**
 * Level 3 enemy pattern - Zigzag movement
 * Enemies spawn individually and move in sine wave pattern across screen
 */
void enemy_orchestrator_update_level3(EnemyOrchestrator* orch, float delta_time) {
    orch->elapsed_time += delta_time;
    
    // Spawn 15 enemies total, one at a time every 1.2 seconds, alternating from left and right
    if (orch->wave_count < 15 && orch->elapsed_time - orch->last_spawn_time > 1.2f) {
        // Alternate sides based on wave count
        bool from_left = (orch->wave_count % 2) == 0;
        float start_x = from_left ? -150.0f : 150.0f;
        float vel_x = from_left ? 40.0f : -40.0f;  // Move across screen
        
        enemy_orchestrator_spawn_enemy(orch, start_x, -100.0f, -400.0f, vel_x, 0.0f, 60.0f);
        orch->last_spawn_time = orch->elapsed_time;
        orch->wave_count++;
    }
    
    // Update all active enemies with zigzag motion
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* enemy = &orch->enemies[i];
        
        // Standard velocity movement
        enemy->position.v[0] += enemy->velocity.v[0] * delta_time;
        enemy->position.v[1] += enemy->velocity.v[1] * delta_time;
        enemy->position.v[2] += enemy->velocity.v[2] * delta_time;
        
        // Add sine wave to X for zigzag effect
        float age = orch->elapsed_time - enemy->spawn_time;
        enemy->position.v[0] += sinf(age * 3.0f) * 50.0f * delta_time;
        
        // Update transform matrix
        float scale[3] = {1.0f, 1.0f, 1.0f};
        float rotation[3] = {0.0f, 0.0f, 0.0f};
        float position[3] = {enemy->position.v[0], enemy->position.v[1], enemy->position.v[2]};
        t3d_mat4fp_from_srt_euler(enemy->matrix, scale, rotation, position);
        
        // Update collision boxes
        collision_system_update_boxes_by_range(orch->collision_system, 
                                               enemy->collision_start_index, 
                                               enemy->collision_count, 
                                               enemy->matrix);
        
        // Update hit timer
        if (enemy->hit_timer > 0.0f) {
            enemy->hit_timer -= delta_time;
            if (enemy->hit_timer <= 0.0f) enemy->show_hit = false;
        }
        
        // Update flash timer
        if (enemy->system.flash_timer > 0.0f) {
            enemy->system.flash_timer -= delta_time;
            if (enemy->system.flash_timer < 0.0f) enemy->system.flash_timer = 0.0f;
        }
        
        // Deactivate if moved past player
        if (enemy->position.v[2] > 100.0f) {
            for (int j = enemy->collision_start_index; j < enemy->collision_start_index + enemy->collision_count; j++) {
                if (j < orch->collision_system->count) {
                    orch->collision_system->boxes[j].active = false;
                }
            }
            enemy->active = false;
            orch->active_count--;
            debugf("Enemy %d moved past player, deactivated\n", i);
        }
        
        // Deactivate if destroyed
        if (!enemy_system_is_active(&enemy->system)) {
            for (int j = enemy->collision_start_index; j < enemy->collision_start_index + enemy->collision_count; j++) {
                if (j < orch->collision_system->count) {
                    orch->collision_system->boxes[j].active = false;
                }
            }
            enemy->active = false;
            orch->active_count--;
            debugf("Enemy %d destroyed\n", i);
        }
    }
    
    // Update explosions
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (orch->enemies[i].has_explosion) {
            EnemyInstance* enemy = &orch->enemies[i];
            enemy->explosion_timer -= delta_time;
            
            if (enemy->explosion_timer <= 0.0f) {
                enemy->has_explosion = false;
                float far_scale[3] = {0.0f, 0.0f, 0.0f};
                float far_rotation[3] = {0.0f, 0.0f, 0.0f};
                float far_position[3] = {0.0f, -10000.0f, 0.0f};
                t3d_mat4fp_from_srt_euler(orch->explosion_matrices[i], far_scale, far_rotation, far_position);
            }
        }
    }
}

T3DMat4FP* enemy_orchestrator_get_matrix(EnemyOrchestrator* orch, int index) {
    if (index >= 0 && index < MAX_ENEMIES) {
        return orch->enemies[index].matrix;
    }
    return NULL;
}

EnemySystem* enemy_orchestrator_get_system(EnemyOrchestrator* orch, int index) {
    if (index >= 0 && index < MAX_ENEMIES) {
        return &orch->enemies[index].system;
    }
    return NULL;
}

bool enemy_orchestrator_is_active(EnemyOrchestrator* orch, int index) {
    if (index >= 0 && index < MAX_ENEMIES) {
        return orch->enemies[index].active;
    }
    return false;
}

int enemy_orchestrator_get_active_count(EnemyOrchestrator* orch) {
    return orch->active_count;
}

void enemy_orchestrator_cleanup(EnemyOrchestrator* orch) {
    // Free explosion model
    if (orch->explosion_model) {
        t3d_model_free(orch->explosion_model);
        orch->explosion_model = NULL;
    }
    
    // Free bomber model and skeleton
    if (orch->bomber_model) {
        t3d_model_free(orch->bomber_model);
        orch->bomber_model = NULL;
    }
    
    if (orch->bomber_anim_system) {
        free_uncached(orch->bomber_anim_system);
        orch->bomber_anim_system = NULL;
    }
    
    if (orch->bomber_skeleton) {
        t3d_skeleton_destroy(orch->bomber_skeleton);
        free_uncached(orch->bomber_skeleton);
        orch->bomber_skeleton = NULL;
    }
    
    // Free explosion matrices
    if (orch->explosion_matrices) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (orch->explosion_matrices[i]) {
                free_uncached(orch->explosion_matrices[i]);
            }
        }
        free_uncached(orch->explosion_matrices);
        orch->explosion_matrices = NULL;
    }
    
    // Free enemy matrices
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (orch->enemies[i].matrix) {
            free_uncached(orch->enemies[i].matrix);
            orch->enemies[i].matrix = NULL;
        }
    }
}

/**
 * Spawn enemy projectiles for level 1 enemies in pause phase
 * Enemies shoot towards the player at regular intervals
 */
void enemy_orchestrator_spawn_projectiles_level1(EnemyOrchestrator* orch, void* projectile_system_ptr, float delta_time) {
    if (!orch || !projectile_system_ptr) return;
    
    ProjectileSystem* ps = (ProjectileSystem*)projectile_system_ptr;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* enemy = &orch->enemies[i];
        
        // Shoot during flying in (phase 0) and pause phase (phase 1)
        if (enemy->movement_phase == 0 || enemy->movement_phase == 1) {
            enemy->shoot_timer += delta_time;
            
            // Shoot every 1 second
            if (enemy->shoot_timer >= 1.0f) {
                enemy->shoot_timer = 0.0f;
                
                // Spawn projectile at enemy position
                T3DVec3 spawn_pos = enemy->position;
                
                // Shoot towards player (assuming player at 0, -150, 0)
                T3DVec3 shoot_direction = {{0.0f, 0.0f, 1.0f}};  // Forward towards player
                
                projectile_system_spawn(ps, spawn_pos, shoot_direction, PROJECTILE_ENEMY);
            }
        }
    }
}

void enemy_orchestrator_spawn_projectiles_level2(EnemyOrchestrator* orch, void* projectile_system_ptr, float delta_time) {
    if (!orch || !projectile_system_ptr) return;
    
    ProjectileSystem* ps = (ProjectileSystem*)projectile_system_ptr;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* bomber = &orch->enemies[i];
        bomber->shoot_timer += delta_time;
        
        // Phase 2: Strafe attack - rapid fire from 4 locations
        if (orch->bomber_phase == 2) {
            if (bomber->shoot_timer >= 0.15f) {  // Very rapid fire during strafe
                bomber->shoot_timer = 0.0f;
                
                T3DVec3 shoot_dir = {{0.0f, 0.0f, 1.0f}};  // Forward
                
                // 4 spawn positions: front, left wing, right wing, rear
                T3DVec3 positions[4] = {
                    {{bomber->position.v[0], bomber->position.v[1] - 10.0f, bomber->position.v[2] + 60.0f}},   // Front turret
                    {{bomber->position.v[0] - 90.0f, bomber->position.v[1] - 5.0f, bomber->position.v[2] + 20.0f}},  // Left wing
                    {{bomber->position.v[0] + 90.0f, bomber->position.v[1] - 5.0f, bomber->position.v[2] + 20.0f}},  // Right wing
                    {{bomber->position.v[0], bomber->position.v[1] + 5.0f, bomber->position.v[2] - 40.0f}}     // Rear turret
                };
                
                // Cycle through positions
                int pos_index = ((int)(orch->elapsed_time * 6.67f)) % 4;
                projectile_system_spawn(ps, positions[pos_index], shoot_dir, PROJECTILE_ENEMY);
            }
        }
        // Phase 4: Wave pattern - spread barrage
        else if (orch->bomber_phase == 4) {
            if (bomber->shoot_timer >= 0.4f) {  // Shoot waves every 0.4 seconds
                bomber->shoot_timer = 0.0f;
                
                // Spawn 7 projectiles in a wave pattern
                for (int j = -3; j <= 3; j++) {
                    T3DVec3 spawn_pos = {{
                        bomber->position.v[0] + j * 45.0f,
                        bomber->position.v[1] - 15.0f,
                        bomber->position.v[2] + 30.0f
                    }};
                    
                    // Angled shots creating wave spread
                    float angle = j * 0.15f;  // Spread pattern
                    T3DVec3 shoot_dir = {{sinf(angle), 0.0f, cosf(angle)}};
                    
                    projectile_system_spawn(ps, spawn_pos, shoot_dir, PROJECTILE_ENEMY);
                }
            }
        }
    }
}

void enemy_orchestrator_spawn_projectiles_level3(EnemyOrchestrator* orch, void* projectile_system_ptr, float delta_time) {
    if (!orch || !projectile_system_ptr) return;
    
    ProjectileSystem* ps = (ProjectileSystem*)projectile_system_ptr;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* enemy = &orch->enemies[i];
        enemy->shoot_timer += delta_time;
        
        // Shoot every 0.8 seconds
        if (enemy->shoot_timer >= 0.8f) {
            enemy->shoot_timer = 0.0f;
            
            // Spawn projectile at enemy position
            T3DVec3 spawn_pos = enemy->position;
            
            // Shoot towards player
            T3DVec3 shoot_direction = {{0.0f, 0.0f, 1.0f}};
            
            projectile_system_spawn(ps, spawn_pos, shoot_direction, PROJECTILE_ENEMY);
        }
    }
}

bool enemy_orchestrator_all_waves_complete(EnemyOrchestrator* orch, int max_waves) {
    return orch->wave_count >= max_waves && orch->active_count == 0;
}

T3DMat4FP* enemy_orchestrator_get_explosion_matrix(EnemyOrchestrator* orch, int index) {
    if (index >= 0 && index < MAX_ENEMIES && orch->enemies[index].has_explosion) {
        return orch->explosion_matrices[index];
    }
    return NULL;
}

bool enemy_orchestrator_has_explosion(EnemyOrchestrator* orch, int index) {
    if (index >= 0 && index < MAX_ENEMIES) {
        return orch->enemies[index].has_explosion;
    }
    return false;
}

T3DModel* enemy_orchestrator_get_explosion_model(EnemyOrchestrator* orch) {
    return orch->explosion_model;
}

// Level 4 Boss Implementation
void enemy_orchestrator_init_level4_boss(EnemyOrchestrator* orch, CollisionSystem* collision_system) {
    orch->enemy_model = NULL;
    orch->bomber_model = NULL;
    orch->bomber_skeleton = NULL;
    orch->bomber_anim_system = NULL;
    orch->collision_system = collision_system;
    orch->elapsed_time = 0.0f;
    orch->last_spawn_time = 0.0f;
    orch->active_count = 0;
    orch->wave_count = 0;
    orch->bomber_phase = 0;
    orch->bomber_phase_timer = 0.0f;
    
    // Load explosion model
    orch->explosion_model = t3d_model_load("rom:/explosion.t3dm");
    
    // Allocate explosion matrices
    orch->explosion_matrices = malloc_uncached(sizeof(T3DMat4FP*) * MAX_ENEMIES);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        orch->explosion_matrices[i] = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(orch->explosion_matrices[i]);
    }
    
    // Initialize all enemy slots
    for (int i = 0; i < MAX_ENEMIES; i++) {
        orch->enemies[i].matrix = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(orch->enemies[i].matrix);
        orch->enemies[i].active = false;
        orch->enemies[i].collision_start_index = -1;
        orch->enemies[i].collision_count = 0;
        orch->enemies[i].movement_phase = 0;
        orch->enemies[i].phase_timer = 0.0f;
        orch->enemies[i].shoot_timer = 0.0f;
        orch->enemies[i].has_explosion = false;
        orch->enemies[i].explosion_timer = 0.0f;
    }
    
    // Load boss model
    orch->boss_model = t3d_model_load("rom:/enemy3.t3dm");
    if (!orch->boss_model) {
        debugf("ERROR: Failed to load enemy3.t3dm for boss\n");
        return;
    }
    
    // Initialize boss skeleton and animation
    const T3DChunkSkeleton* skelChunk = t3d_model_get_skeleton(orch->boss_model);
    if (skelChunk) {
        orch->boss_skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *orch->boss_skeleton = t3d_skeleton_create(orch->boss_model);
        animation_system_init(&orch->boss_anim, orch->boss_model, orch->boss_skeleton);
        animation_system_play(&orch->boss_anim, "Idle", true);
    }
    
    // Initialize boss movement state
    orch->boss_side_progress = 0.5f;
    orch->boss_moving_right = true;
    orch->boss_barrage_cooldown = 10.0f;
    orch->boss_spin_timer = 0.0f;
    
    // Spawn boss in slot 0
    EnemyInstance* boss = &orch->enemies[0];
    boss->position = (T3DVec3){{0.0f, -100.0f, -300.0f}};
    boss->velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};
    
    // Setup transform with scale 1.0
    float scale[3] = {1.0f, 1.0f, 1.0f};
    float rotation[3] = {0.0f, 0.0f, 0.0f};
    float position[3] = {0.0f, -100.0f, -300.0f};
    t3d_mat4fp_from_srt_euler(boss->matrix, scale, rotation, position);
    
    // Extract collision
    int collision_before = collision_system->count;
    collision_system_extract_from_model(collision_system, orch->boss_model, "ENEMY_", COLLISION_ENEMY);
    boss->collision_start_index = collision_before;
    boss->collision_count = collision_system->count - collision_before;
    collision_system_update_boxes_by_range(collision_system, boss->collision_start_index, boss->collision_count, boss->matrix);
    
    // Initialize with 50 HP
    enemy_system_init(&boss->system, 50);
    boss->active = true;
    boss->show_hit = false;
    boss->hit_timer = 0.0f;
    boss->shoot_timer = 0.0f;
    boss->movement_phase = 0;
    boss->phase_timer = 0.0f;
    
    orch->active_count = 1;
    debugf("Boss initialized: 50 HP\n");
}

void enemy_orchestrator_update_level4_boss(EnemyOrchestrator* orch, float delta_time, void* projectile_system_ptr) {
    if (delta_time <= 0.0f || delta_time > 1.0f) delta_time = 0.016f;
    
    ProjectileSystem* ps = (ProjectileSystem*)projectile_system_ptr;
    orch->elapsed_time += delta_time;
    
    if (!orch->boss_model || !orch->boss_skeleton) return;
    
    EnemyInstance* boss = &orch->enemies[0];
    if (!boss->active) return;
    
    // Update health system
    enemy_system_update(&boss->system, delta_time, &boss->show_hit, &boss->hit_timer, 
                       orch->collision_system, boss->system.last_damage_taken);
    
    // Check defeat
    if (!boss->system.active && !boss->has_explosion) {
        boss->active = false;
        boss->has_explosion = true;
        boss->explosion_timer = 0.25f;
        boss->explosion_position = boss->position;
        orch->active_count = 0;
        
        float exp_scale[3] = {3.0f, 3.0f, 3.0f};
        float exp_rotation[3] = {0.0f, 0.0f, 0.0f};
        float exp_position[3] = {boss->explosion_position.v[0], boss->explosion_position.v[1], boss->explosion_position.v[2]};
        t3d_mat4fp_from_srt_euler(orch->explosion_matrices[0], exp_scale, exp_rotation, exp_position);
        return;
    }
    
    // Update explosion
    if (boss->has_explosion) {
        boss->explosion_timer -= delta_time;
        if (boss->explosion_timer <= 0.0f) {
            boss->has_explosion = false;
        }
        return;
    }
    
    // Immediate phase transitions
    if (boss->movement_phase == 0) {
        boss->movement_phase = 1;
        animation_system_play(&orch->boss_anim, "Move", true);
    }
    
    // Side-to-side movement (both phases)
    const float move_speed = 0.4f;
    const float move_range = 120.0f;
    
    bool is_barraging = (boss->movement_phase == 2 && orch->boss_barrage_cooldown > 0.0f && orch->boss_barrage_cooldown < 3.0f);
    
    if (!is_barraging) {
        if (orch->boss_moving_right) {
            orch->boss_side_progress += move_speed * delta_time;
            if (orch->boss_side_progress >= 1.0f) {
                orch->boss_side_progress = 1.0f;
                orch->boss_moving_right = false;
            }
        } else {
            orch->boss_side_progress -= move_speed * delta_time;
            if (orch->boss_side_progress <= 0.0f) {
                orch->boss_side_progress = 0.0f;
                orch->boss_moving_right = true;
            }
        }
        boss->position.v[0] = (orch->boss_side_progress - 0.5f) * 2.0f * move_range;
    }
    
    // Add sine wave vertical movement
    orch->boss_spin_timer += delta_time;
    boss->position.v[1] = -100.0f + sinf(orch->boss_spin_timer * 1.5f) * 30.0f;
    
    // Phase-specific behavior
    if (boss->movement_phase == 1) {
        // Phase 1: Move animation, SlashLeft/SlashRight shoot fan towards player
        animation_system_update(&orch->boss_anim, delta_time);
        
        boss->shoot_timer += delta_time;
        if (boss->shoot_timer >= 2.0f) {
            boss->shoot_timer = 0.0f;
            
            // Play slash animation based on side position
            if (orch->boss_side_progress < 0.5f) {
                animation_system_play(&orch->boss_anim, "SlashLeft", false);
            } else {
                animation_system_play(&orch->boss_anim, "SlashRight", false);
            }
            
            // Shoot fan of 4 projectiles angled towards player
            float base_angle = 0.0f;  // Forward direction
            float spread = 0.3f;  // Spread angle in radians
            
            T3DVec3 spawn_pos = {{boss->position.v[0], boss->position.v[1] + 100.0f, boss->position.v[2]}};
            
            for (int i = 0; i < 4; i++) {
                float angle = base_angle + (i - 1.5f) * spread;
                T3DVec3 dir = {{sinf(angle), 0.0f, cosf(angle)}};
                t3d_vec3_norm(&dir);
                projectile_system_spawn(ps, spawn_pos, dir, PROJECTILE_ENEMY);
            }
        }
        
        // Transition to barrage phase
        orch->boss_barrage_cooldown -= delta_time;
        if (orch->boss_barrage_cooldown <= 0.0f) {
            boss->movement_phase = 2;
            orch->boss_barrage_cooldown = 0.0f;
        }
        
    } else if (boss->movement_phase == 2) {
        // Phase 2: Move side to side, occasionally do SlashBarage with stream
        
        orch->boss_barrage_cooldown += delta_time;
        
        if (orch->boss_barrage_cooldown < 3.0f) {
            // Doing barrage attack
            animation_system_update(&orch->boss_anim, delta_time);
            
            if (orch->boss_barrage_cooldown < 0.1f) {
                // Start barrage - stop near current position
                animation_system_play(&orch->boss_anim, "SlashBarage", false);
            }
            
            // Stream of projectiles
            boss->shoot_timer += delta_time;
            if (boss->shoot_timer >= 0.15f) {
                boss->shoot_timer = 0.0f;
                T3DVec3 spawn_pos = {{boss->position.v[0], boss->position.v[1] + 100.0f, boss->position.v[2]}};
                T3DVec3 dir = {{0.0f, 0.0f, 1.0f}};
                projectile_system_spawn(ps, spawn_pos, dir, PROJECTILE_ENEMY);
            }
        } else if (orch->boss_barrage_cooldown < 6.0f) {
            // Moving side to side between barrages
            animation_system_update(&orch->boss_anim, delta_time);
            
            if (orch->boss_barrage_cooldown > 3.1f && orch->boss_barrage_cooldown < 3.2f) {
                animation_system_play(&orch->boss_anim, "Move", true);
            }
        } else {
            // Return to phase 1
            boss->movement_phase = 1;
            animation_system_play(&orch->boss_anim, "Move", true);
            orch->boss_barrage_cooldown = 10.0f;
            boss->shoot_timer = 0.0f;
        }
        
    } else {
        // Idle fallback
        animation_system_update(&orch->boss_anim, delta_time);
    }
    
    // Update transform
    float scale[3] = {1.0f, 1.0f, 1.0f};
    float rotation[3] = {0.0f, 0.0f, 0.0f};
    float position[3] = {boss->position.v[0], boss->position.v[1], boss->position.v[2]};
    t3d_mat4fp_from_srt_euler(boss->matrix, scale, rotation, position);
    
    // Update collision
    collision_system_update_boxes_by_range(orch->collision_system, 
                                          boss->collision_start_index, 
                                          boss->collision_count, 
                                          boss->matrix);
}

T3DModel* enemy_orchestrator_get_boss_model(EnemyOrchestrator* orch) {
    return orch->boss_model;
}

T3DSkeleton* enemy_orchestrator_get_boss_skeleton(EnemyOrchestrator* orch) {
    return orch->boss_skeleton;
}

// Level 5 Boss Functions

void enemy_orchestrator_init_level5_boss(EnemyOrchestrator* orch, CollisionSystem* collision_system) {
    orch->enemy_model = NULL;
    orch->bomber_model = NULL;
    orch->bomber_skeleton = NULL;
    orch->bomber_anim_system = NULL;
    orch->collision_system = collision_system;
    orch->elapsed_time = 0.0f;
    orch->last_spawn_time = 0.0f;
    orch->active_count = 0;
    orch->wave_count = 0;
    
    // Load explosion model
    orch->explosion_model = t3d_model_load("rom:/explosion.t3dm");
    
    // Allocate explosion matrices
    orch->explosion_matrices = malloc_uncached(sizeof(T3DMat4FP*) * MAX_ENEMIES);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        orch->explosion_matrices[i] = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(orch->explosion_matrices[i]);
    }
    
    // Initialize all enemy slots
    for (int i = 0; i < MAX_ENEMIES; i++) {
        orch->enemies[i].matrix = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(orch->enemies[i].matrix);
        orch->enemies[i].active = false;
        orch->enemies[i].collision_start_index = -1;
        orch->enemies[i].collision_count = 0;
        orch->enemies[i].movement_phase = 0;
        orch->enemies[i].phase_timer = 0.0f;
        orch->enemies[i].shoot_timer = 0.0f;
        orch->enemies[i].has_explosion = false;
        orch->enemies[i].explosion_timer = 0.0f;
    }
    
    // Load Level 5 boss model
    orch->level5_boss_model = t3d_model_load("rom:/enemy4.t3dm");
    if (!orch->level5_boss_model) {
        debugf("ERROR: Failed to load enemy4.t3dm for Level 5 boss\n");
        return;
    }
    
    // Initialize boss skeleton and animation
    const T3DChunkSkeleton* skelChunk = t3d_model_get_skeleton(orch->level5_boss_model);
    if (skelChunk) {
        orch->level5_boss_skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *orch->level5_boss_skeleton = t3d_skeleton_create(orch->level5_boss_model);
        animation_system_init(&orch->level5_boss_anim, orch->level5_boss_model, orch->level5_boss_skeleton);
        animation_system_play(&orch->level5_boss_anim, "Move", true);
    }
    
    // Initialize Level 5 boss state
    orch->level5_boss_sine_timer = 0.0f;
    orch->level5_boss_phase = 0;  // Start with Phase 0 (MachineGun)
    orch->level5_boss_attack_timer = 0.0f;
    orch->level5_boss_curve_offset = 0.0f;
    orch->level5_boss_curve_right = true;
    orch->level5_boss_cannon_shots = 0;
    
    // Spawn boss in slot 0
    EnemyInstance* boss = &orch->enemies[0];
    boss->position = (T3DVec3){{0.0f, -100.0f, -300.0f}};
    boss->velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};
    
    // Setup transform with scale 1.2
    float scale[3] = {1.2f, 1.2f, 1.2f};
    float rotation[3] = {0.0f, 0.0f, 0.0f};
    float position[3] = {0.0f, -100.0f, -300.0f};
    t3d_mat4fp_from_srt_euler(boss->matrix, scale, rotation, position);
    
    // Extract collision
    int collision_before = collision_system->count;
    collision_system_extract_from_model(collision_system, orch->level5_boss_model, "ENEMY_", COLLISION_ENEMY);
    boss->collision_start_index = collision_before;
    boss->collision_count = collision_system->count - collision_before;
    collision_system_update_boxes_by_range(collision_system, boss->collision_start_index, boss->collision_count, boss->matrix);
    
    // Initialize with 100 HP (final boss)
    enemy_system_init(&boss->system, 100);
    boss->active = true;
    boss->show_hit = false;
    boss->hit_timer = 0.0f;
    boss->shoot_timer = 0.0f;
    boss->movement_phase = 0;
    boss->phase_timer = 0.0f;
    
    orch->active_count = 1;
    debugf("Level 5 Boss initialized: 100 HP\n");
}

void enemy_orchestrator_update_level5_boss(EnemyOrchestrator* orch, float delta_time, void* projectile_system_ptr) {
    if (delta_time <= 0.0f || delta_time > 1.0f) delta_time = 0.016f;
    
    ProjectileSystem* ps = (ProjectileSystem*)projectile_system_ptr;
    orch->elapsed_time += delta_time;
    
    if (!orch->level5_boss_model || !orch->level5_boss_skeleton) return;
    
    EnemyInstance* boss = &orch->enemies[0];
    if (!boss->active) return;
    
    // Update animation system once per frame
    animation_system_update(&orch->level5_boss_anim, delta_time);
    
    // Update health system
    enemy_system_update(&boss->system, delta_time, &boss->show_hit, &boss->hit_timer, 
                       orch->collision_system, boss->system.last_damage_taken);
    
    // Check defeat
    if (!boss->system.active && !boss->has_explosion) {
        boss->active = false;
        boss->has_explosion = true;
        boss->explosion_timer = 0.25f;
        boss->explosion_position = boss->position;
        orch->active_count = 0;
        
        float exp_scale[3] = {4.0f, 4.0f, 4.0f};
        float exp_rotation[3] = {0.0f, 0.0f, 0.0f};
        float exp_position[3] = {boss->explosion_position.v[0], boss->explosion_position.v[1], boss->explosion_position.v[2]};
        t3d_mat4fp_from_srt_euler(orch->explosion_matrices[0], exp_scale, exp_rotation, exp_position);
        return;
    }
    
    // Update explosion
    if (boss->has_explosion) {
        boss->explosion_timer -= delta_time;
        if (boss->explosion_timer <= 0.0f) {
            boss->has_explosion = false;
        }
        return;
    }
    
    // Sine wave vertical movement
    orch->level5_boss_sine_timer += delta_time;
    boss->position.v[1] = -100.0f + sinf(orch->level5_boss_sine_timer * 1.2f) * 40.0f;
    
    // Phase management
    orch->level5_boss_attack_timer += delta_time;
    
    if (orch->level5_boss_phase == 0) {
        // Phase 0: MachineGun attack with curving projectiles
        
        if (orch->level5_boss_attack_timer < 0.1f) {
            // Just entered phase - play MachineGun animation
            animation_system_play(&orch->level5_boss_anim, "MachineGun", true);
            orch->level5_boss_curve_offset = 0.0f;
            orch->level5_boss_curve_right = true;
        }
        
        // Shoot tightly packed projectiles that curve
        boss->shoot_timer += delta_time;
        if (boss->shoot_timer >= 0.1f) {
            boss->shoot_timer = 0.0f;
            
            // Update curve direction
            if (orch->level5_boss_curve_right) {
                orch->level5_boss_curve_offset += 0.08f;
                if (orch->level5_boss_curve_offset >= 0.6f) {
                    orch->level5_boss_curve_right = false;
                }
            } else {
                orch->level5_boss_curve_offset -= 0.08f;
                if (orch->level5_boss_curve_offset <= -0.6f) {
                    orch->level5_boss_curve_right = true;
                }
            }
            
            // Spawn projectile with curve
            T3DVec3 spawn_pos = {{boss->position.v[0], boss->position.v[1] + 100.0f, boss->position.v[2]}};
            float angle = orch->level5_boss_curve_offset;
            T3DVec3 dir = {{sinf(angle), 0.0f, cosf(angle)}};
            t3d_vec3_norm(&dir);
            projectile_system_spawn(ps, spawn_pos, dir, PROJECTILE_ENEMY);
        }
        
        // Transition to Cannon phase after 8 seconds
        if (orch->level5_boss_attack_timer >= 8.0f) {
            orch->level5_boss_phase = 1;
            orch->level5_boss_attack_timer = 0.0f;
            orch->level5_boss_cannon_shots = 0;
            animation_system_play(&orch->level5_boss_anim, "Cannon", false);
        }
        
    } else if (orch->level5_boss_phase == 1) {
        // Phase 1: Cannon attack - fan of 3 projectiles twice
        
        if (orch->level5_boss_attack_timer < 0.1f) {
            // Just entered phase - play Cannon animation
            animation_system_play(&orch->level5_boss_anim, "Cannon", false);
        }
        
        // Shoot fan of 3 projectiles at specific intervals
        if (orch->level5_boss_cannon_shots == 0 && orch->level5_boss_attack_timer >= 0.5f) {
            // First shot
            orch->level5_boss_cannon_shots = 1;
            
            T3DVec3 spawn_pos = {{boss->position.v[0], boss->position.v[1] + 100.0f, boss->position.v[2]}};
            
            // Fan of 3 projectiles
            for (int i = 0; i < 3; i++) {
                float angle = (i - 1) * 0.4f;  // -0.4, 0, 0.4 radians
                T3DVec3 dir = {{sinf(angle), 0.0f, cosf(angle)}};
                t3d_vec3_norm(&dir);
                projectile_system_spawn(ps, spawn_pos, dir, PROJECTILE_ENEMY);
            }
            
        } else if (orch->level5_boss_cannon_shots == 1 && orch->level5_boss_attack_timer >= 1.5f) {
            // Second shot
            orch->level5_boss_cannon_shots = 2;
            
            T3DVec3 spawn_pos = {{boss->position.v[0], boss->position.v[1] + 100.0f, boss->position.v[2]}};
            
            // Fan of 3 projectiles
            for (int i = 0; i < 3; i++) {
                float angle = (i - 1) * 0.4f;  // -0.4, 0, 0.4 radians
                T3DVec3 dir = {{sinf(angle), 0.0f, cosf(angle)}};
                t3d_vec3_norm(&dir);
                projectile_system_spawn(ps, spawn_pos, dir, PROJECTILE_ENEMY);
            }
        }
        
        // Return to MachineGun phase after 3 seconds
        if (orch->level5_boss_attack_timer >= 3.0f) {
            orch->level5_boss_phase = 0;
            orch->level5_boss_attack_timer = 0.0f;
            animation_system_play(&orch->level5_boss_anim, "Move", true);
        }
    }
    
    // Update transform
    float scale[3] = {1.2f, 1.2f, 1.2f};
    float rotation[3] = {0.0f, 0.0f, 0.0f};
    float position[3] = {boss->position.v[0], boss->position.v[1], boss->position.v[2]};
    t3d_mat4fp_from_srt_euler(boss->matrix, scale, rotation, position);
    
    // Update collision
    collision_system_update_boxes_by_range(orch->collision_system, 
                                          boss->collision_start_index, 
                                          boss->collision_count, 
                                          boss->matrix);
}

T3DModel* enemy_orchestrator_get_level5_boss_model(EnemyOrchestrator* orch) {
    return orch->level5_boss_model;
}

T3DSkeleton* enemy_orchestrator_get_level5_boss_skeleton(EnemyOrchestrator* orch) {
    return orch->level5_boss_skeleton;
}
