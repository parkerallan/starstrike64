#include "playercontrols.h"
#include <math.h>

#define DEADZONE 8.0f  // Analog stick deadzone

void playercontrols_init(PlayerControls* pc, T3DVec3 start_pos, PlayerBoundary boundary, float move_speed) {
    pc->position = start_pos;
    pc->velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};
    pc->boundary = boundary;
    pc->move_speed = move_speed;
    pc->acceleration = move_speed * 8.0f;  // Quick acceleration
    pc->deceleration = move_speed * 6.0f;  // Quick deceleration
    pc->max_velocity = move_speed;
}

void playercontrols_update(PlayerControls* pc, joypad_inputs_t inputs, float delta_time) {
    // Safety check for delta_time
    if (delta_time <= 0.0f || delta_time > 1.0f || isnan(delta_time)) {
        delta_time = 1.0f / 60.0f;
    }
    
    // Read analog stick input
    float stick_x = inputs.stick_x;
    float stick_y = inputs.stick_y;
    
    // Apply deadzone
    if (fabsf(stick_x) < DEADZONE) {
        stick_x = 0.0f;
    }
    if (fabsf(stick_y) < DEADZONE) {
        stick_y = 0.0f;
    }
    
    // Normalize stick input to -1.0 to 1.0 range
    stick_x = fmaxf(-1.0f, fminf(1.0f, stick_x / 80.0f));
    stick_y = fmaxf(-1.0f, fminf(1.0f, stick_y / 80.0f));
    
    // Calculate target velocity based on stick input
    float target_vel_x = stick_x * pc->max_velocity;
    float target_vel_y = stick_y * pc->max_velocity;  // Up stick = positive Y in world
    
    // Smooth acceleration/deceleration
    float accel_rate = (fabsf(stick_x) > 0.01f || fabsf(stick_y) > 0.01f) ? 
                       pc->acceleration : pc->deceleration;
    
    // Update velocity with acceleration
    if (fabsf(target_vel_x - pc->velocity.v[0]) > 0.01f) {
        float vel_diff_x = target_vel_x - pc->velocity.v[0];
        float change_x = accel_rate * delta_time;
        if (fabsf(vel_diff_x) < change_x) {
            pc->velocity.v[0] = target_vel_x;
        } else {
            pc->velocity.v[0] += (vel_diff_x > 0 ? change_x : -change_x);
        }
    }
    
    if (fabsf(target_vel_y - pc->velocity.v[1]) > 0.01f) {
        float vel_diff_y = target_vel_y - pc->velocity.v[1];
        float change_y = accel_rate * delta_time;
        if (fabsf(vel_diff_y) < change_y) {
            pc->velocity.v[1] = target_vel_y;
        } else {
            pc->velocity.v[1] += (vel_diff_y > 0 ? change_y : -change_y);
        }
    }
    
    // Update position based on velocity
    pc->position.v[0] += pc->velocity.v[0] * delta_time;
    pc->position.v[1] += pc->velocity.v[1] * delta_time;
    // Z position stays constant for now (depth into screen)
    
    // Clamp position to boundaries
    playercontrols_clamp_to_boundary(pc);
}

void playercontrols_clamp_to_boundary(PlayerControls* pc) {
    // Clamp X position
    if (pc->position.v[0] < pc->boundary.min_x) {
        pc->position.v[0] = pc->boundary.min_x;
        pc->velocity.v[0] = 0.0f;  // Stop movement when hitting boundary
    } else if (pc->position.v[0] > pc->boundary.max_x) {
        pc->position.v[0] = pc->boundary.max_x;
        pc->velocity.v[0] = 0.0f;
    }
    
    // Clamp Y position
    if (pc->position.v[1] < pc->boundary.min_y) {
        pc->position.v[1] = pc->boundary.min_y;
        pc->velocity.v[1] = 0.0f;
    } else if (pc->position.v[1] > pc->boundary.max_y) {
        pc->position.v[1] = pc->boundary.max_y;
        pc->velocity.v[1] = 0.0f;
    }
    
    // Clamp Z position (if needed)
    if (pc->position.v[2] < pc->boundary.min_z) {
        pc->position.v[2] = pc->boundary.min_z;
        pc->velocity.v[2] = 0.0f;
    } else if (pc->position.v[2] > pc->boundary.max_z) {
        pc->position.v[2] = pc->boundary.max_z;
        pc->velocity.v[2] = 0.0f;
    }
}

T3DVec3 playercontrols_get_position(const PlayerControls* pc) {
    return pc->position;
}

void playercontrols_set_position(PlayerControls* pc, T3DVec3 new_pos) {
    pc->position = new_pos;
    pc->velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};  // Reset velocity when repositioning
    playercontrols_clamp_to_boundary(pc);
}
