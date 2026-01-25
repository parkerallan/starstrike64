#include "enemysystem.h"

void enemy_system_init(EnemySystem* system, int health) {
    system->max_health = health;
    system->health = health;
    system->active = true;
    system->hit_this_frame = false;
    system->hit_timer = 0.0f;
    system->flash_timer = 0.0f;
    system->flash_duration = 0.15f;  // Flash for 150ms
    system->previous_hit_timer = 0.0f;
    system->last_damage_taken = 0;
}

void enemy_system_update(EnemySystem* system, float delta_time, bool* show_hit_flag, float* hit_timer, CollisionSystem* collision_system, int damage_taken) {
    if (!system->active) {
        return;
    }
    
    // Detect new hit by checking if timer increased (got reset to 0.5f)
    bool new_hit = *show_hit_flag && (*hit_timer > system->previous_hit_timer);
    
    if (new_hit) {
        system->health -= damage_taken;
        system->last_damage_taken = damage_taken;
        system->flash_timer = system->flash_duration;  // Start flash
        
        if (system->health <= 0) {
            system->active = false;
            // Disable all enemy collision boxes
            if (collision_system) {
                for (int i = 0; i < collision_system->count; i++) {
                    if (collision_system->boxes[i].type == COLLISION_ENEMY) {
                        collision_system->boxes[i].active = false;
                    }
                }
            }
        }
    }
    
    // Store current timer value for next frame comparison
    system->previous_hit_timer = *hit_timer;
    
    // Update hit timer
    if (*hit_timer > 0.0f) {
        *hit_timer -= delta_time;
        if (*hit_timer <= 0.0f) {
            *show_hit_flag = false;
        }
    }
    
    // Update flash timer
    if (system->flash_timer > 0.0f) {
        system->flash_timer -= delta_time;
        if (system->flash_timer < 0.0f) {
            system->flash_timer = 0.0f;
        }
    }
}

bool enemy_system_is_active(const EnemySystem* system) {
    return system->active;
}

int enemy_system_get_health(const EnemySystem* system) {
    return system->health;
}

int enemy_system_get_max_health(const EnemySystem* system) {
    return system->max_health;
}

bool enemy_system_is_flashing(const EnemySystem* system) {
    return system->flash_timer > 0.0f;
}
