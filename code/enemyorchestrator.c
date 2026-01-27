/**
 * @file enemyorchestrator.c
 * @brief Enemy spawning and movement orchestrator for shmup levels
 */

#include "enemyorchestrator.h"
#include <stdlib.h>
#include <string.h>

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
 * Level 1 enemy pattern - Classic shmup wave attack
 * Groups of 3 enemies fly in from the right side in diagonal formation,
 * pause in front of player in a horizontal line, then fly off to the left.
 * 5 waves total with 3 second intervals.
 */
void enemy_orchestrator_update_level1(EnemyOrchestrator* orch, float delta_time) {
    // Validate delta_time
    if (delta_time <= 0.0f || delta_time != delta_time || delta_time > 1.0f) {
        delta_time = 0.0001f;
    }
    
    orch->elapsed_time += delta_time;
    
    // Spawn 5 waves of 3 enemies - wait for previous wave to clear before spawning next
    // Check if enough time has passed AND no enemies are currently active
    if (orch->wave_count < 5 && 
        orch->elapsed_time - orch->last_spawn_time > 5.0f &&
        orch->active_count == 0) {
        
        // Spawn 3 enemies in diagonal formation from right side
        // They fly in horizontally (left direction only), arranged diagonally
        
        // Top-right enemy (furthest back in Z)
        enemy_orchestrator_spawn_enemy(orch, 250.0f, -80.0f, -450.0f, -100.0f, 0.0f, 0.0f);
        orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{-80.0f, -80.0f, -450.0f}};
        orch->enemies[orch->active_count - 1].movement_phase = 0;
        orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
        
        // Middle enemy
        enemy_orchestrator_spawn_enemy(orch, 280.0f, -100.0f, -350.0f, -100.0f, 0.0f, 0.0f);
        orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{0.0f, -100.0f, -350.0f}};
        orch->enemies[orch->active_count - 1].movement_phase = 0;
        orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
        
        // Bottom-right enemy (closest in Z)
        enemy_orchestrator_spawn_enemy(orch, 310.0f, -120.0f, -250.0f, -100.0f, 0.0f, 0.0f);
        orch->enemies[orch->active_count - 1].target_position = (T3DVec3){{80.0f, -120.0f, -250.0f}};
        orch->enemies[orch->active_count - 1].movement_phase = 0;
        orch->enemies[orch->active_count - 1].phase_timer = 0.0f;
        
        orch->last_spawn_time = orch->elapsed_time;
        orch->wave_count++;
    }
    
    // Update all active enemies with phase-based movement
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
            case 0: // Flying in from right side (horizontal movement only)
            {
                // Move towards target position (only X changes, Z stays constant)
                enemy->position.v[0] += enemy->velocity.v[0] * delta_time;
                enemy->position.v[1] += enemy->velocity.v[1] * delta_time;
                enemy->position.v[2] += enemy->velocity.v[2] * delta_time;
                
                // Check if reached target X position (horizontal arrival)
                float dist_x = fabsf(enemy->target_position.v[0] - enemy->position.v[0]);
                
                if (dist_x < 5.0f) { // Within 5 units of target X
                    // Snap to target position and transition to pause phase
                    enemy->position.v[0] = enemy->target_position.v[0];
                    enemy->velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};
                    enemy->movement_phase = 1;
                    enemy->phase_timer = 0.0f;
                }
                break;
            }
            
            case 1: // Paused in formation in front of player
            {
                // Stay in place for 3 seconds (increased pause time)
                if (enemy->phase_timer > 3.0f) {
                    // Transition to fly off phase
                    enemy->velocity = (T3DVec3){{-120.0f, 0.0f, 0.0f}}; // Fly left faster
                    enemy->movement_phase = 2;
                    enemy->phase_timer = 0.0f;
                }
                break;
            }
            
            case 2: // Flying off to the left
            {
                enemy->position.v[0] += enemy->velocity.v[0] * delta_time;
                
                // Deactivate when off screen
                if (enemy->position.v[0] < -250.0f) {
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
