#ifndef OUTFITSYSTEM_H
#define OUTFITSYSTEM_H

#include <stdbool.h>
#include <t3d/t3dmodel.h>

// Outfit types for the mecha
typedef enum {
    OUTFIT_BASE,        // Base mecha: Head, Arms, Body, Foot, LowerLeg, UpperLeg
    OUTFIT_THRUST,      // Thrust mode: Base + Thrust object
    OUTFIT_COUNT        // Total number of outfits
} OutfitType;

// Outfit system state
typedef struct {
    OutfitType current_outfit;
    bool initialized;
    float thrust_timer;  // Timer for thrust outfit duration (in seconds)
} OutfitSystem;

// Initialize the outfit system
void outfit_system_init(OutfitSystem* outfit_system);

// Set a specific outfit
void outfit_system_set_outfit(OutfitSystem* outfit_system, OutfitType outfit);

// Get current outfit
OutfitType outfit_system_get_current_outfit(const OutfitSystem* outfit_system);

// Cycle to next outfit
void outfit_system_cycle_next(OutfitSystem* outfit_system);

// Update outfit system (handles thrust timer)
void outfit_system_update(OutfitSystem* outfit_system, float delta_time);

// Activate thrust outfit for duration (in seconds)
void outfit_system_activate_thrust(OutfitSystem* outfit_system, float duration);

// Cycle to previous outfit
void outfit_system_cycle_previous(OutfitSystem* outfit_system);

// Get outfit name for display
const char* outfit_system_get_outfit_name(OutfitType outfit);

// Filter callback for T3D model rendering
bool outfit_system_filter_callback(void* userData, const T3DObject *obj);

#endif // OUTFITSYSTEM_H
