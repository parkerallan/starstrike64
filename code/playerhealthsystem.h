#ifndef PLAYERHEALTHSYSTEM_H
#define PLAYERHEALTHSYSTEM_H

#include <libdragon.h>
#include <stdbool.h>
#include "collisionsystem.h"

typedef struct {
    int health;
    int max_health;
    bool is_dead;
    sprite_t* health_sprite;
    float hit_display_timer;
    bool show_hit;
} PlayerHealthSystem;

/**
 * Initialize the player health system
 * Automatically extracts max health from PLAYER collision boxes
 */
void player_health_init(PlayerHealthSystem* system, CollisionSystem* collision_system);

/**
 * Take damage and update health
 * Returns true if player died from this hit
 */
bool player_health_take_damage(PlayerHealthSystem* system, int damage);

/**
 * Update timers (call every frame)
 */
void player_health_update(PlayerHealthSystem* system, float delta_time);

/**
 * Render health display in top-right corner
 */
void player_health_render(PlayerHealthSystem* system);

/**
 * Check if player is dead
 */
bool player_health_is_dead(PlayerHealthSystem* system);

/**
 * Check if hit display is active
 */
bool player_health_is_showing_hit(PlayerHealthSystem* system);

/**
 * Cleanup resources
 */
void player_health_cleanup(PlayerHealthSystem* system);

#endif // PLAYERHEALTHSYSTEM_H
