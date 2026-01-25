/**
 * @file collisionsystem.c
 * @brief Collision detection for projectiles
 * 
 * This system extracts collision boxes from 3D models based on object naming:
 * - PROJ_* : Projectile collision boxes
 * - PLAYER_* : Player collision boxes
 * - ENEMY_* : Enemy collision boxes
 */

#include "collisionsystem.h"
#include <stdlib.h>
#include <string.h>

int collision_system_parse_health_from_name(const char* name) {
    if (!name) return 1;
    
    // Find last underscore in name
    const char* lastUnderscore = strrchr(name, '_');
    if (lastUnderscore && lastUnderscore[1] != '\0') {
        int health = atoi(lastUnderscore + 1);
        if (health > 0) {
            return health;
        }
    }
    
    return 1;  // Default health
}

int collision_system_get_enemy_health(CollisionSystem* system) {
    if (!system) return 1;
    
    for (int i = 0; i < system->count; i++) {
        if (system->boxes[i].type == COLLISION_ENEMY) {
            return collision_system_parse_health_from_name(system->boxes[i].name);
        }
    }
    
    return 1;  // Default health
}
#include <stdio.h>

#define INITIAL_CAPACITY 16

/**
 * Initialize the collision system
 */
void collision_system_init(CollisionSystem* system) {
    system->boxes = malloc(sizeof(CollisionAABB) * INITIAL_CAPACITY);
    system->count = 0;
    system->capacity = INITIAL_CAPACITY;
    system->initialized = true;
    
    if (!system->boxes) {
        debugf("ERROR: Failed to allocate collision boxes\n");
        system->initialized = false;
    }
}

/**
 * Helper: Ensure capacity for one more box
 */
static void ensure_capacity(CollisionSystem* system) {
    if (system->count >= system->capacity) {
        system->capacity *= 2;
        CollisionAABB* newBoxes = realloc(system->boxes, sizeof(CollisionAABB) * system->capacity);
        if (!newBoxes) {
            debugf("ERROR: Failed to reallocate collision boxes\n");
            return;
        }
        system->boxes = newBoxes;
    }
}

/**
 * Add a collision box to the system
 */
void collision_system_add_box(
    CollisionSystem* system,
    float minX, float minZ, float minY,
    float maxX, float maxZ, float maxY,
    const char* name,
    CollisionType type
) {
    if (!system || !system->initialized) return;
    
    ensure_capacity(system);
    
    CollisionAABB* box = &system->boxes[system->count];
    
    box->min[0] = minX;
    box->min[1] = minZ;
    box->max[0] = maxX;
    box->max[1] = maxZ;
    box->minY = minY;
    box->maxY = maxY;
    box->type = type;
    box->active = true;
    
    if (name) {
        strncpy(box->name, name, sizeof(box->name) - 1);
        box->name[sizeof(box->name) - 1] = '\0';
    } else {
        snprintf(box->name, sizeof(box->name), "box_%d", system->count);
    }
    
    system->count++;
    
    debugf("Added collision box: %s (type %d) at (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)\n",
           box->name, type, minX, minY, minZ, maxX, maxY, maxZ);
}

/**
 * Extract collision boxes from model based on prefix
 */
void collision_system_extract_from_model(
    CollisionSystem* system,
    T3DModel* model,
    const char* prefix,
    CollisionType type
) {
    if (!system || !system->initialized || !model || !prefix) {
        debugf("ERROR: Invalid parameters for collision extraction\n");
        return;
    }
    
    debugf("Extracting collision boxes with prefix '%s'...\n", prefix);
    
    size_t prefix_len = strlen(prefix);
    int found_count = 0;
    
    // Iterate through all objects in the model using T3D iterator
    T3DModelIter it = t3d_model_iter_create(model, T3D_CHUNK_TYPE_OBJECT);
    
    while (t3d_model_iter_next(&it)) {
        T3DObject* obj = it.object;
        if (!obj || !obj->name) continue;
        
        // Check if object name starts with the prefix
        if (strncmp(obj->name, prefix, prefix_len) == 0) {
            // Get bounding box from object's AABB
            float minX = (float)obj->aabbMin[0];
            float minY = (float)obj->aabbMin[1];
            float minZ = (float)obj->aabbMin[2];
            float maxX = (float)obj->aabbMax[0];
            float maxY = (float)obj->aabbMax[1];
            float maxZ = (float)obj->aabbMax[2];
            
            // Add the collision box
            collision_system_add_box(system, minX, minZ, minY, maxX, maxZ, maxY, obj->name, type);
            found_count++;
        }
    }
    
    debugf("Found %d collision boxes with prefix '%s'\n", found_count, prefix);
}

/**
 * Extract collision boxes from model with position offset
 */
void collision_system_extract_from_model_with_offset(
    CollisionSystem* system,
    T3DModel* model,
    const char* prefix,
    CollisionType type,
    float offset_x,
    float offset_y,
    float offset_z
) {
    if (!system || !system->initialized || !model || !prefix) {
        debugf("ERROR: Invalid parameters for collision extraction\n");
        return;
    }
    
    debugf("Extracting collision boxes with prefix '%s' and offset (%.1f, %.1f, %.1f)...\n", prefix, offset_x, offset_y, offset_z);
    
    size_t prefix_len = strlen(prefix);
    int found_count = 0;
    
    // Iterate through all objects in the model using T3D iterator
    T3DModelIter it = t3d_model_iter_create(model, T3D_CHUNK_TYPE_OBJECT);
    
    while (t3d_model_iter_next(&it)) {
        T3DObject* obj = it.object;
        if (!obj || !obj->name) continue;
        
        // Check if object name starts with the prefix
        if (strncmp(obj->name, prefix, prefix_len) == 0) {
            // Get bounding box from object's AABB and apply offset
            float minX = (float)obj->aabbMin[0] + offset_x;
            float minY = (float)obj->aabbMin[1] + offset_y;
            float minZ = (float)obj->aabbMin[2] + offset_z;
            float maxX = (float)obj->aabbMax[0] + offset_x;
            float maxY = (float)obj->aabbMax[1] + offset_y;
            float maxZ = (float)obj->aabbMax[2] + offset_z;
            
            // Add the collision box
            collision_system_add_box(system, minX, minZ, minY, maxX, maxZ, maxY, obj->name, type);
            found_count++;
        }
    }
    
    debugf("Found %d collision boxes with prefix '%s'\n", found_count, prefix);
}

/**
 * Check if a point is inside an AABB
 */
static bool point_in_aabb(const CollisionAABB* box, const T3DVec3* position) {
    return position->v[0] >= box->min[0] && position->v[0] <= box->max[0] &&
           position->v[1] >= box->minY && position->v[1] <= box->maxY &&
           position->v[2] >= box->min[1] && position->v[2] <= box->max[1];
}

/**
 * Check collision between a point and all boxes of a specific type
 */
bool collision_system_check_point(
    CollisionSystem* system,
    const T3DVec3* position,
    CollisionType target_type,
    char* hit_name
) {
    if (!system || !system->initialized || !position) return false;
    
    for (int i = 0; i < system->count; i++) {
        if (!system->boxes[i].active) continue;
        if (system->boxes[i].type != target_type) continue;
        
        if (point_in_aabb(&system->boxes[i], position)) {
            if (hit_name) {
                strncpy(hit_name, system->boxes[i].name, 63);
                hit_name[63] = '\0';
            }
            return true;
        }
    }
    
    return false;
}

/**
 * Update collision box position (useful for moving objects)
 */
void collision_system_update_box_position(
    CollisionSystem* system,
    const char* name,
    const T3DVec3* position
) {
    if (!system || !system->initialized || !name || !position) return;
    
    for (int i = 0; i < system->count; i++) {
        if (strcmp(system->boxes[i].name, name) == 0) {
            float width = system->boxes[i].max[0] - system->boxes[i].min[0];
            float height = system->boxes[i].maxY - system->boxes[i].minY;
            float depth = system->boxes[i].max[1] - system->boxes[i].min[1];
            
            // Center the box on the new position
            system->boxes[i].min[0] = position->v[0] - width * 0.5f;
            system->boxes[i].max[0] = position->v[0] + width * 0.5f;
            system->boxes[i].minY = position->v[1] - height * 0.5f;
            system->boxes[i].maxY = position->v[1] + height * 0.5f;
            system->boxes[i].min[1] = position->v[2] - depth * 0.5f;
            system->boxes[i].max[1] = position->v[2] + depth * 0.5f;
            return;
        }
    }
}

/**
 * Remove a collision box by name
 */
void collision_system_remove_box(
    CollisionSystem* system,
    const char* name
) {
    if (!system || !system->initialized || !name) return;
    
    for (int i = 0; i < system->count; i++) {
        if (strcmp(system->boxes[i].name, name) == 0) {
            system->boxes[i].active = false;
            return;
        }
    }
}

/**
 * Cleanup the collision system
 */
void collision_system_cleanup(CollisionSystem* system) {
    if (!system) return;
    
    if (system->boxes) {
        free(system->boxes);
        system->boxes = NULL;
    }
    
    system->count = 0;
    system->capacity = 0;
    system->initialized = false;
}
