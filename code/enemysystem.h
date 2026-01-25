#ifndef ENEMYSYSTEM_H
#define ENEMYSYSTEM_H

#include <stdbool.h>
#include "collisionsystem.h"

typedef struct {
    int health;
    int max_health;
    bool active;
    bool hit_this_frame;
    float hit_timer;
    float flash_timer;
    float flash_duration;
    float previous_hit_timer;
    int last_damage_taken;  // Track damage from last hit
} EnemySystem;

// Initialize enemy system with health value
void enemy_system_init(EnemySystem* system, int health);

// Update enemy system (handles hit detection and health reduction)
void enemy_system_update(EnemySystem* system, float delta_time, bool* show_hit_flag, float* hit_timer, CollisionSystem* collision_system, int damage_taken);

// Check if enemy is alive
bool enemy_system_is_active(const EnemySystem* system);

// Get current health
int enemy_system_get_health(const EnemySystem* system);

// Get max health
int enemy_system_get_max_health(const EnemySystem* system);

// Check if enemy is flashing (just took damage)
bool enemy_system_is_flashing(const EnemySystem* system);

#endif // ENEMYSYSTEM_H
