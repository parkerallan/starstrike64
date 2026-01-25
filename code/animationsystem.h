#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3danim.h>
#include <t3d/t3dskeleton.h>

typedef struct {
    T3DModel* model;           // Reference to the model
    T3DSkeleton* skeleton;     // Reference to the skeleton
    T3DAnim current_anim;      // Currently playing animation
    char current_name[64];     // Name of current animation
    bool is_playing;           // Is animation currently playing
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

// Utility functions
const char* animation_system_get_current_name(AnimationSystem* anim_sys);
bool animation_system_is_playing(AnimationSystem* anim_sys);

#endif
