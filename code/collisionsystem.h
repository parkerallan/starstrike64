#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <stdbool.h>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>

#define MAX_COLLISION_BOXES 32

// Collision box types
typedef enum {
    COLLISION_PROJECTILE,
    COLLISION_PLAYER,
    COLLISION_ENEMY
} CollisionType;

// Axis-aligned bounding box for collision
typedef struct {
    float min[2];  // minX, minZ
    float max[2];  // maxX, maxZ
    float minY;    // Y min
    float maxY;    // Y max
    char name[64];
    CollisionType type;
    bool active;
} CollisionAABB;

// Collision system
typedef struct {
    CollisionAABB* boxes;
    int count;
    int capacity;
    bool initialized;
} CollisionSystem;

// Initialize the collision system
void collision_system_init(CollisionSystem* system);

// Cleanup the collision system
void collision_system_cleanup(CollisionSystem* system);

// Extract collision boxes from a model based on prefix
void collision_system_extract_from_model(
    CollisionSystem* system,
    T3DModel* model,
    const char* prefix,
    CollisionType type
);

// Extract collision boxes from a model with position offset
void collision_system_extract_from_model_with_offset(
    CollisionSystem* system,
    T3DModel* model,
    const char* prefix,
    CollisionType type,
    float offset_x,
    float offset_y,
    float offset_z
);

// Add a collision box manually
void collision_system_add_box(
    CollisionSystem* system,
    float minX, float minZ, float minY,
    float maxX, float maxZ, float maxY,
    const char* name,
    CollisionType type
);

// Check collision between a point and all collision boxes of a specific type
// Returns true if collision detected, and sets hit_name if provided
bool collision_system_check_point(
    CollisionSystem* system,
    const T3DVec3* position,
    CollisionType target_type,
    char* hit_name
);

// Update collision boxes (for projectiles that move)
void collision_system_update_box_position(
    CollisionSystem* system,
    const char* name,
    const T3DVec3* position
);

// Remove a collision box by name
void collision_system_remove_box(
    CollisionSystem* system,
    const char* name
);

// Parse health value from collision box name (e.g., ENEMY_Ship_5 returns 5)
// Returns parsed health value, or 1 if no valid health suffix found
int collision_system_parse_health_from_name(const char* name);

// Get health value from first ENEMY collision box in system
// Returns parsed health value, or 1 if no enemy boxes found
int collision_system_get_enemy_health(CollisionSystem* system);

#endif // COLLISIONSYSTEM_H
