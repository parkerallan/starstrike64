#ifndef PROJECTILESYSTEM_H
#define PROJECTILESYSTEM_H

#include <stdbool.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>

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
} Projectile;

// Projectile system
typedef struct {
    Projectile projectiles[MAX_PROJECTILES];
    T3DModel* projectile_models[PROJECTILE_TYPE_COUNT];
    T3DMat4FP* projectile_matrices[MAX_PROJECTILES];
    
    float projectile_speed;
    float projectile_lifetime;
    float shoot_cooldown;
    float cooldown_timer;
    
    bool initialized;
} ProjectileSystem;

// Initialize the projectile system
void projectile_system_init(ProjectileSystem* ps, float speed, float lifetime, float cooldown);

// Cleanup the projectile system
void projectile_system_cleanup(ProjectileSystem* ps);

// Spawn a new projectile from a position with a direction and type
void projectile_system_spawn(ProjectileSystem* ps, T3DVec3 position, T3DVec3 direction, ProjectileType type);

// Update all projectiles
void projectile_system_update(ProjectileSystem* ps, float delta_time);

// Render all projectiles
void projectile_system_render(ProjectileSystem* ps);

// Check if can shoot (cooldown expired)
bool projectile_system_can_shoot(const ProjectileSystem* ps);

#endif // PROJECTILESYSTEM_H
