#ifndef PROJECTILESYSTEM_H
#define PROJECTILESYSTEM_H

#include <stdbool.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include "collisionsystem.h"

#define MAX_PROJECTILES 32

// Projectile types
typedef enum {
    PROJECTILE_NORMAL,
    PROJECTILE_SLASH,
    PROJECTILE_TYPE_COUNT
} ProjectileType;

// Individual projectile
typedef struct {
    T3DVec3 position;
    T3DVec3 velocity;
    float lifetime;
    bool active;
    ProjectileType type;
    int damage;  // Damage dealt on hit
} Projectile;

// Projectile system
typedef struct {
    Projectile projectiles[MAX_PROJECTILES];
    T3DModel* projectile_models[PROJECTILE_TYPE_COUNT];
    T3DMat4FP* projectile_matrices[MAX_PROJECTILES];
    
    float projectile_speed;
    float projectile_lifetime;
    float shoot_cooldowns[PROJECTILE_TYPE_COUNT];
    float cooldown_timers[PROJECTILE_TYPE_COUNT];
    
    bool initialized;
} ProjectileSystem;

// Initialize the projectile system
void projectile_system_init(ProjectileSystem* ps, float speed, float lifetime, float normal_cooldown, float slash_cooldown);

// Cleanup the projectile system
void projectile_system_cleanup(ProjectileSystem* ps);

// Spawn a new projectile from a position with a direction and type
void projectile_system_spawn(ProjectileSystem* ps, T3DVec3 position, T3DVec3 direction, ProjectileType type);

// Update all projectiles
void projectile_system_update(ProjectileSystem* ps, float delta_time);

// Update all projectiles with collision checking
void projectile_system_update_with_collision(ProjectileSystem* ps, float delta_time, CollisionSystem* collision, bool* enemy_hit, bool* player_hit, float* enemy_timer, float* player_timer);

// Render all projectiles
void projectile_system_render(ProjectileSystem* ps);

// Check if can shoot (cooldown expired)
bool projectile_system_can_shoot(const ProjectileSystem* ps, ProjectileType type);

// Get the damage dealt by the last projectile hit
int projectile_system_get_last_damage(void);

#endif // PROJECTILESYSTEM_H
