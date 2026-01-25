#ifndef PLAYERCONTROLS_H
#define PLAYERCONTROLS_H

#include <libdragon.h>
#include <t3d/t3d.h>

// Player boundaries (in world space)
typedef struct {
    float min_x;
    float max_x;
    float min_y;
    float max_y;
    float min_z;  // For forward/back movement if needed later
    float max_z;
} PlayerBoundary;

// Player control state
typedef struct {
    T3DVec3 position;
    T3DVec3 velocity;
    PlayerBoundary boundary;
    
    float move_speed;
    float acceleration;
    float deceleration;
    float max_velocity;
} PlayerControls;

// Initialize player controls with starting position and boundaries
void playercontrols_init(PlayerControls* pc, T3DVec3 start_pos, PlayerBoundary boundary, float move_speed);

// Update player position based on control stick input
void playercontrols_update(PlayerControls* pc, joypad_inputs_t inputs, float delta_time);

// Get current player position
T3DVec3 playercontrols_get_position(const PlayerControls* pc);

// Set player position (useful for respawning, etc.)
void playercontrols_set_position(PlayerControls* pc, T3DVec3 new_pos);

// Check if player is within boundaries (internal use)
void playercontrols_clamp_to_boundary(PlayerControls* pc);

#endif // PLAYERCONTROLS_H
