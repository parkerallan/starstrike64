/**
 * @file enemyorchestrator.c
 * @brief Enemy spawning and movement orchestrator for shmup levels
 */

#include "enemyorchestrator.h"
#include <stdlib.h>
#include <string.h>
#define M_PI 3.14159265358979323846

// Forward declaration of common update helper
static void enemy_orchestrator_update_common(EnemyOrchestrator* orch, float delta_time);

void enemy_orchestrator_init(EnemyOrchestrator* orch, T3DModel* enemy_model, CollisionSystem* collision_system) {
    orch->enemy_model = enemy_model;
    orch->collision_system = collision_system;
    orch->elapsed_time = 0.0f;
    orch->last_spawn_time = 0.0f;
    orch->active_count = 0;
    orch->wave_count = 0;
    
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
            
            // Set up transform matrix
            float scale[3] = {1.0f, 1.0f, 1.0f};
            float rotation[3] = {0.0f, 0.0f, 0.0f};
            float position[3] = {x, y, z};
            t3d_mat4fp_from_srt_euler(enemy->matrix, scale, rotation, position);
            
            // Extract collision boxes for this enemy (extracted at model origin)
            int collision_before = orch->collision_system->count;
            collision_system_extract_from_model(orch->collision_system, orch->enemy_model, "ENEMY_", COLLISION_ENEMY);
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
}

/**
 * Level 2 enemy pattern - V-formation waves
 * 5 enemies spawn in a V pattern, every 4 seconds
 */
void enemy_orchestrator_update_level2(EnemyOrchestrator* orch, float delta_time) {
    orch->elapsed_time += delta_time;
    
    // Spawn pattern: Wave of 5 enemies in V-formation every 4 seconds
    if (orch->elapsed_time - orch->last_spawn_time > 4.0f) {
        // V-formation: 2 on each side angled inward, 1 at point
        enemy_orchestrator_spawn_enemy(orch, -100.0f, -150.0f, -450.0f, 20.0f, 0.0f, 90.0f);  // Left outer
        enemy_orchestrator_spawn_enemy(orch, -50.0f, -120.0f, -400.0f, 10.0f, 0.0f, 85.0f);   // Left inner
        enemy_orchestrator_spawn_enemy(orch, 0.0f, -100.0f, -380.0f, 0.0f, 0.0f, 80.0f);      // Point
        enemy_orchestrator_spawn_enemy(orch, 50.0f, -120.0f, -400.0f, -10.0f, 0.0f, 85.0f);   // Right inner
        enemy_orchestrator_spawn_enemy(orch, 100.0f, -150.0f, -450.0f, -20.0f, 0.0f, 90.0f);  // Right outer
        orch->last_spawn_time = orch->elapsed_time;
    }
    
    // Update all active enemies (same straight movement logic)
    enemy_orchestrator_update_common(orch, delta_time);
}

/**
 * Level 3 enemy pattern - Zigzag movement
 * Enemies spawn individually and move in sine wave pattern across screen
 */
void enemy_orchestrator_update_level3(EnemyOrchestrator* orch, float delta_time) {
    orch->elapsed_time += delta_time;
    
    // Spawn pattern: Single enemy every 1.5 seconds from alternating sides
    if (orch->elapsed_time - orch->last_spawn_time > 1.5f) {
        // Alternate sides based on spawn count
        bool from_left = ((int)(orch->elapsed_time / 1.5f) % 2) == 0;
        float start_x = from_left ? -150.0f : 150.0f;
        float vel_x = from_left ? 40.0f : -40.0f;  // Move across screen
        
        enemy_orchestrator_spawn_enemy(orch, start_x, -100.0f, -400.0f, vel_x, 0.0f, 60.0f);
        orch->last_spawn_time = orch->elapsed_time;
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
}

/**
 * Common enemy update logic (used by level 1 and 2)
 * Updates position, collision, timers for straight-moving enemies
 */
static void enemy_orchestrator_update_common(EnemyOrchestrator* orch, float delta_time) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!orch->enemies[i].active) continue;
        
        EnemyInstance* enemy = &orch->enemies[i];
        
        // Update position based on velocity
        enemy->position.v[0] += enemy->velocity.v[0] * delta_time;
        enemy->position.v[1] += enemy->velocity.v[1] * delta_time;
        enemy->position.v[2] += enemy->velocity.v[2] * delta_time;
        
        // Update transform matrix
        float scale[3] = {1.0f, 1.0f, 1.0f};
        float rotation[3] = {0.0f, 0.0f, 0.0f};
        float position[3] = {enemy->position.v[0], enemy->position.v[1], enemy->position.v[2]};
        t3d_mat4fp_from_srt_euler(enemy->matrix, scale, rotation, position);
        
        // Update collision boxes for THIS specific enemy only
        collision_system_update_boxes_by_range(orch->collision_system, 
                                               enemy->collision_start_index, 
                                               enemy->collision_count, 
                                               enemy->matrix);
        
        // Update hit timer
        if (enemy->hit_timer > 0.0f) {
            enemy->hit_timer -= delta_time;
            if (enemy->hit_timer <= 0.0f) {
                enemy->show_hit = false;
            }
        }
        
        // Update flash timer
        if (enemy->system.flash_timer > 0.0f) {
            enemy->system.flash_timer -= delta_time;
            if (enemy->system.flash_timer < 0.0f) {
                enemy->system.flash_timer = 0.0f;
            }
        }
        
        // Deactivate if moved past player (z > 100)
        if (enemy->position.v[2] > 100.0f) {
            // Remove collision boxes for this enemy
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
            // Remove collision boxes for this enemy
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
    
    // Include projectile system header for spawning
    #include "projectilesystem.h"
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
