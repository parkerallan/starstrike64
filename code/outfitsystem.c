#include "outfitsystem.h"
#include <string.h>
#include <libdragon.h>

// Static outfit object definitions
static const char* base_objects[] = {"Head", "Arms", "Body", "Foot", "LowerLeg", "UpperLeg"};
static const char* thrust_objects[] = {"Head", "Arms", "Body", "Foot", "LowerLeg", "UpperLeg", "Thrust"};

static const int base_object_count = 6;
static const int thrust_object_count = 7;

// Outfit names for display
static const char* outfit_names[] = {
    "Base Mecha",
    "Thrust Mode",
};

void outfit_system_init(OutfitSystem* outfit_system) {
    if (!outfit_system) return;
    
    outfit_system->current_outfit = OUTFIT_BASE;
    outfit_system->initialized = true;
    outfit_system->thrust_timer = 0.0f;
    
    debugf("Outfit system initialized - starting with: %s\n", outfit_names[OUTFIT_BASE]);
}

void outfit_system_set_outfit(OutfitSystem* outfit_system, OutfitType outfit) {
    if (!outfit_system || !outfit_system->initialized) return;
    if (outfit >= OUTFIT_COUNT) return;  // Invalid outfit
    
    outfit_system->current_outfit = outfit;
    debugf("Outfit set to: %s\n", outfit_names[outfit]);
}

OutfitType outfit_system_get_current_outfit(const OutfitSystem* outfit_system) {
    if (!outfit_system || !outfit_system->initialized) {
        return OUTFIT_BASE;  // Default fallback
    }
    return outfit_system->current_outfit;
}

void outfit_system_cycle_next(OutfitSystem* outfit_system) {
    if (!outfit_system || !outfit_system->initialized) return;
    
    outfit_system->current_outfit = (outfit_system->current_outfit + 1) % OUTFIT_COUNT;
    debugf("Cycled to next outfit: %s\n", outfit_names[outfit_system->current_outfit]);
}

void outfit_system_update(OutfitSystem* outfit_system, float delta_time) {
    if (!outfit_system || !outfit_system->initialized) return;
    
    // Validate delta_time
    if (delta_time <= 0.0f || delta_time != delta_time || delta_time > 1.0f) {
        return;
    }
    
    // Update thrust timer
    if (outfit_system->thrust_timer > 0.0f) {
        outfit_system->thrust_timer -= delta_time;
        if (outfit_system->thrust_timer <= 0.0f) {
            // Timer expired, revert to base outfit
            outfit_system->thrust_timer = 0.0f;
            outfit_system->current_outfit = OUTFIT_BASE;
        }
    }
}

void outfit_system_activate_thrust(OutfitSystem* outfit_system, float duration) {
    if (!outfit_system || !outfit_system->initialized) return;
    
    outfit_system->current_outfit = OUTFIT_THRUST;
    outfit_system->thrust_timer = duration;
}

void outfit_system_cycle_previous(OutfitSystem* outfit_system) {
    if (!outfit_system || !outfit_system->initialized) return;
    
    outfit_system->current_outfit = (outfit_system->current_outfit + OUTFIT_COUNT - 1) % OUTFIT_COUNT;
    debugf("Cycled to previous outfit: %s\n", outfit_names[outfit_system->current_outfit]);
}

const char* outfit_system_get_outfit_name(OutfitType outfit) {
    if (outfit >= OUTFIT_COUNT) {
        return "Invalid Outfit";
    }
    return outfit_names[outfit];
}

bool outfit_system_filter_callback(void* userData, const T3DObject *obj) {
    OutfitSystem* outfit_system = (OutfitSystem*)userData;
    
    if (!outfit_system || !outfit_system->initialized || !obj->name) {
        return true;  // Show unnamed objects or if system not initialized
    }
    
    const char** current_objects;
    int object_count;
    
    // Select current outfit objects
    switch (outfit_system->current_outfit) {
        case OUTFIT_BASE:
            current_objects = base_objects;
            object_count = base_object_count;
            break;
        case OUTFIT_THRUST:
            current_objects = thrust_objects;
            object_count = thrust_object_count;
            break;
        default:
            return true; // Show everything if invalid outfit
    }
    
    // Check if this object should be visible in current outfit
    // Use prefix matching to support variants like "Foot.L", "Foot.R", etc.
    for (int i = 0; i < object_count; i++) {
        size_t pattern_len = strlen(current_objects[i]);
        if (strncmp(obj->name, current_objects[i], pattern_len) == 0) {
            // Check if it's an exact match or has a valid suffix (like .L, .R, .001, etc.)
            char next_char = obj->name[pattern_len];
            if (next_char == '\0' || next_char == '.' || next_char == '_') {
                return true;  // Object is part of current outfit
            }
        }
    }
    
    return false;  // Object not part of current outfit, hide it
}
