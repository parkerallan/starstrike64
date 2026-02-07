#include "playerhealthsystem.h"

void player_health_init(PlayerHealthSystem* system, CollisionSystem* collision_system) {
    // Initialize player health from PLAYER collision box name
    system->max_health = 1;  // Default
    for (int i = 0; i < collision_system->count; i++) {
        if (collision_system->boxes[i].type == COLLISION_PLAYER) {
            system->max_health = collision_system_parse_health_from_name(collision_system->boxes[i].name);
            debugf("Found player collision box: %s with health %d\n", 
                   collision_system->boxes[i].name, system->max_health);
            break;
        }
    }
    system->health = system->max_health;
    system->is_dead = false;
    system->hit_display_timer = 0.0f;
    system->show_hit = false;
    system->flash_timer = 0.0f;
    system->flash_duration = 0.15f;  // Flash for 150ms
    system->death_timer = 0.0f;
    
    debugf("Player initialized with %d health\n", system->health);
    
    // Load health sprite
    system->health_sprite = sprite_load("rom:/health.sprite");
    if (!system->health_sprite) {
        debugf("WARNING: Failed to load health sprite\n");
    }
}

bool player_health_take_damage(PlayerHealthSystem* system, int damage) {
    if (system->is_dead) return false;
    
    system->health -= damage;
    system->show_hit = true;
    system->hit_display_timer = 0.5f;
    system->flash_timer = system->flash_duration;  // Start flash
    
    debugf("Player hit! Health: %d/%d\n", system->health, system->max_health);
    
    if (system->health <= 0) {
        system->is_dead = true;
        debugf("PLAYER DESTROYED!\n");
        return true;
    }
    
    return false;
}

void player_health_update(PlayerHealthSystem* system, float delta_time) {
    if (system->hit_display_timer > 0.0f) {
        system->hit_display_timer -= delta_time;
        if (system->hit_display_timer <= 0.0f) {
            system->show_hit = false;
        }
    }
    
    // Update flash timer
    if (system->flash_timer > 0.0f) {
        system->flash_timer -= delta_time;
        if (system->flash_timer < 0.0f) {
            system->flash_timer = 0.0f;
        }
    }
    
    // Update death timer
    if (system->is_dead && system->death_timer < 5.0f) {
        system->death_timer += delta_time;
    }
}

void player_health_render(PlayerHealthSystem* system) {
    rdpq_sync_pipe();
    
    // Draw player health as sprites
    if (!system->is_dead && system->health_sprite) {
        // Reset color mode for sprite rendering
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        
        // Draw health icons in top-right corner, scaled down, spaced closely
        int spacing = 28;
        int start_x = 320 - (system->health * spacing) - 10;  // Right-aligned with margin
        
        for (int i = 0; i < system->health; i++) {
            rdpq_blitparms_t params = {
                .scale_x = 0.4f,
                .scale_y = 0.4f,
            };
            rdpq_sprite_blit(system->health_sprite, start_x + (i * spacing), 10, &params);
        }
    } else if (system->is_dead) {
        // Draw DESTROYED in center of screen
        rdpq_text_printf(NULL, 1, 115, 110, "DESTROYED");
    }
}

bool player_health_is_dead(PlayerHealthSystem* system) {
    return system->is_dead;
}

bool player_health_should_reload(PlayerHealthSystem* system) {
    return system->is_dead && system->death_timer >= 5.0f;
}

bool player_health_is_showing_hit(PlayerHealthSystem* system) {
    return system->show_hit;
}

bool player_health_is_flashing(PlayerHealthSystem* system) {
    return system->flash_timer > 0.0f;
}

void player_health_cleanup(PlayerHealthSystem* system) {
    if (system->health_sprite) {
        sprite_free(system->health_sprite);
        system->health_sprite = NULL;
    }
}
