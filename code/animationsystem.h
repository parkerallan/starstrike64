#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3danim.h>
#include <t3d/t3dskeleton.h>

typedef struct {
    T3DModel* model;           // Reference to the model
    T3DSkeleton* skeleton;     // Reference to the skeleton (main)
    T3DSkeleton* blend_skeleton; // Optional blend skeleton for blending
    T3DAnim current_anim;      // Currently playing animation
    T3DAnim blend_anim;        // Blend animation (for transitions)
    char current_name[64];     // Name of current animation
    char blend_name[64];       // Name of blend animation
    float blend_factor;        // Blend factor (0.0 = current, 1.0 = blend)
    bool is_playing;           // Is animation currently playing
    bool is_blending;          // Is currently blending between animations
    bool initialized;          // Initialization flag
} AnimationSystem;

// Core functions
void animation_system_init(AnimationSystem* anim_sys, T3DModel* model, T3DSkeleton* skeleton);
void animation_system_update(AnimationSystem* anim_sys, float delta_time);
void animation_system_cleanup(AnimationSystem* anim_sys);

// Simple animation control by name
bool animation_system_play(AnimationSystem* anim_sys, const char* anim_name, bool loop);
void animation_system_stop(AnimationSystem* anim_sys);
void animation_system_pause(AnimationSystem* anim_sys);
void animation_system_resume(AnimationSystem* anim_sys);

// Blend animation control
bool animation_system_blend_to(AnimationSystem* anim_sys, const char* target_anim_name, float blend_speed, bool loop);
void animation_system_set_blend_factor(AnimationSystem* anim_sys, float factor);
float animation_system_get_blend_factor(AnimationSystem* anim_sys);

// Position-based blending helper
void animation_system_update_position_blend(AnimationSystem* anim_sys, float normalized_position, 
                                            const char* left_anim, const char* right_anim, bool loop);

// Utility functions
const char* animation_system_get_current_name(AnimationSystem* anim_sys);
bool animation_system_is_playing(AnimationSystem* anim_sys);
bool animation_system_is_blending(AnimationSystem* anim_sys);

#endif
