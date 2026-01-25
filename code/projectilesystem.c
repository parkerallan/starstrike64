#include "projectilesystem.h"
#include <string.h>
#include <math.h>

void projectile_system_init(ProjectileSystem* ps, float speed, float lifetime, float cooldown) {
    if (!ps) return;
    
    memset(ps, 0, sizeof(ProjectileSystem));
    
    ps->projectile_speed = speed;
    ps->projectile_lifetime = lifetime;
    ps->shoot_cooldown = cooldown;
    ps->cooldown_timer = 0.0f;
    
    // Load projectile models
    ps->projectile_models[PROJECTILE_NORMAL] = t3d_model_load("rom:/playerproj.t3dm");
    if (!ps->projectile_models[PROJECTILE_NORMAL]) {
        debugf("WARNING: Failed to load playerproj model\n");
    } else {
        debugf("Successfully loaded playerproj model\n");
    }
    
    ps->projectile_models[PROJECTILE_SLASH] = t3d_model_load("rom:/slash.t3dm");
    if (!ps->projectile_models[PROJECTILE_SLASH]) {
        debugf("WARNING: Failed to load slash model\n");
    } else {
        debugf("Successfully loaded slash model\n");
    }
    
    // Allocate matrices for each projectile
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        ps->projectile_matrices[i] = malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(ps->projectile_matrices[i]);
        ps->projectiles[i].active = false;
    }
    
    ps->initialized = true;
    debugf("Projectile system initialized\n");
}

void projectile_system_cleanup(ProjectileSystem* ps) {
    if (!ps || !ps->initialized) return;
    
    for (int i = 0; i < PROJECTILE_TYPE_COUNT; i++) {
        if (ps->projectile_models[i]) {
            t3d_model_free(ps->projectile_models[i]);
            ps->projectile_models[i] = NULL;
        }
    }
    
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (ps->projectile_matrices[i]) {
            free_uncached(ps->projectile_matrices[i]);
            ps->projectile_matrices[i] = NULL;
        }
    }
    
    ps->initialized = false;
    debugf("Projectile system cleaned up\n");
}

void projectile_system_spawn(ProjectileSystem* ps, T3DVec3 position, T3DVec3 direction, ProjectileType type) {
    if (!ps || !ps->initialized) return;
    if (type >= PROJECTILE_TYPE_COUNT) return;
    
    // Check cooldown
    if (ps->cooldown_timer > 0.0f) return;
    
    // Find an inactive projectile slot
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!ps->projectiles[i].active) {
            ps->projectiles[i].position = position;
            ps->projectiles[i].type = type;
            
            // Normalize direction and apply speed
            float len = sqrtf(direction.v[0] * direction.v[0] + 
                            direction.v[1] * direction.v[1] + 
                            direction.v[2] * direction.v[2]);
            if (len > 0.001f) {
                ps->projectiles[i].velocity.v[0] = (direction.v[0] / len) * ps->projectile_speed;
                ps->projectiles[i].velocity.v[1] = (direction.v[1] / len) * ps->projectile_speed;
                ps->projectiles[i].velocity.v[2] = (direction.v[2] / len) * ps->projectile_speed;
            } else {
                // Default forward direction if no valid direction given
                ps->projectiles[i].velocity.v[0] = 0.0f;
                ps->projectiles[i].velocity.v[1] = 0.0f;
                ps->projectiles[i].velocity.v[2] = -ps->projectile_speed;
            }
            
            ps->projectiles[i].lifetime = ps->projectile_lifetime;
            ps->projectiles[i].active = true;
            
            // Reset cooldown
            ps->cooldown_timer = ps->shoot_cooldown;
            
            debugf("Spawned projectile at (%.1f, %.1f, %.1f)\n", 
                   position.v[0], position.v[1], position.v[2]);
            return;
        }
    }
    
    debugf("WARNING: No available projectile slots\n");
}

void projectile_system_update(ProjectileSystem* ps, float delta_time) {
    if (!ps || !ps->initialized) return;
    
    // Update cooldown timer
    if (ps->cooldown_timer > 0.0f) {
        ps->cooldown_timer -= delta_time;
        if (ps->cooldown_timer < 0.0f) {
            ps->cooldown_timer = 0.0f;
        }
    }
    
    // Update all active projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (ps->projectiles[i].active) {
            // Update position
            ps->projectiles[i].position.v[0] += ps->projectiles[i].velocity.v[0] * delta_time;
            ps->projectiles[i].position.v[1] += ps->projectiles[i].velocity.v[1] * delta_time;
            ps->projectiles[i].position.v[2] += ps->projectiles[i].velocity.v[2] * delta_time;
            
            // Update lifetime
            ps->projectiles[i].lifetime -= delta_time;
            
            // Deactivate if lifetime expired
            if (ps->projectiles[i].lifetime <= 0.0f) {
                ps->projectiles[i].active = false;
            }
            
            // Update matrix for rendering
            if (ps->projectiles[i].active) {
                float scale[3] = {1.0f, 1.0f, 1.0f};
                float rotation[3] = {0.0f, 0.0f, 0.0f};
                float position[3] = {
                    ps->projectiles[i].position.v[0],
                    ps->projectiles[i].position.v[1],
                    ps->projectiles[i].position.v[2]
                };
                
                t3d_mat4fp_from_srt_euler(ps->projectile_matrices[i], scale, rotation, position);
            }
        }
    }
}

void projectile_system_render(ProjectileSystem* ps) {
    if (!ps || !ps->initialized) return;
    
    // Render all active projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (ps->projectiles[i].active) {
            T3DModel* model = ps->projectile_models[ps->projectiles[i].type];
            if (!model) continue;
            
            t3d_matrix_push(ps->projectile_matrices[i]);
            
            T3DModelDrawConf drawConf = {
                .userData = NULL,
                .tileCb = NULL,
                .filterCb = NULL,
                .dynTextureCb = NULL,
                .matrices = NULL
            };
            
            t3d_model_draw_custom(ps->projectile_models[ps->projectiles[i].type], drawConf);
            t3d_matrix_pop(1);
        }
    }
}

bool projectile_system_can_shoot(const ProjectileSystem* ps) {
    if (!ps || !ps->initialized) return false;
    return ps->cooldown_timer <= 0.0f;
}
