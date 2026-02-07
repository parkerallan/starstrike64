#include "animationsystem.h"
#include <string.h>
#include <malloc.h>

void animation_system_init(AnimationSystem* anim_sys, T3DModel* model, T3DSkeleton* skeleton) {
    if (!anim_sys || !model || !skeleton) return;
    
    // Initialize system
    memset(anim_sys, 0, sizeof(AnimationSystem));
    anim_sys->model = model;
    anim_sys->skeleton = skeleton;
    anim_sys->blend_skeleton = NULL;  // Will be created when blending is needed
    anim_sys->is_playing = false;
    anim_sys->is_blending = false;
    anim_sys->blend_factor = 0.0f;
    strcpy(anim_sys->current_name, "None");
    strcpy(anim_sys->blend_name, "None");
    
    debugf("Animation system initialized\n");
    
    // List available animations for reference
    uint32_t anim_count = t3d_model_get_animation_count(model);
    if (anim_count > 0) {
        T3DChunkAnim** anim_refs = malloc(anim_count * sizeof(T3DChunkAnim*));
        t3d_model_get_animations(model, anim_refs);
        
        debugf("Available animations (%lu total):\n", anim_count);
        for (uint32_t i = 0; i < anim_count; i++) {
            debugf("  - %s\n", anim_refs[i]->name);
        }
        
        free(anim_refs);
    } else {
        debugf("No animations found in model\n");
    }
    
    anim_sys->initialized = true;
}

void animation_system_update(AnimationSystem* anim_sys, float delta_time) {
    if (!anim_sys || !anim_sys->initialized) return;
    
    // Update current animation if playing
    if (anim_sys->is_playing) {
        t3d_anim_update(&anim_sys->current_anim, delta_time);
    }
    
    // Update blend animation if blending
    if (anim_sys->is_blending && anim_sys->blend_skeleton) {
        t3d_anim_update(&anim_sys->blend_anim, delta_time);
        
        // Perform the blend
        t3d_skeleton_blend(anim_sys->skeleton, anim_sys->skeleton, anim_sys->blend_skeleton, anim_sys->blend_factor);
    }
    
    // Always update skeleton
    t3d_skeleton_update(anim_sys->skeleton);
}

void animation_system_cleanup(AnimationSystem* anim_sys) {
    if (!anim_sys) return;
    
    // Stop current animation
    animation_system_stop(anim_sys);
    
    // Free blend skeleton if allocated
    if (anim_sys->blend_skeleton) {
        t3d_skeleton_destroy(anim_sys->blend_skeleton);
        free_uncached(anim_sys->blend_skeleton);
        anim_sys->blend_skeleton = NULL;
    }
    
    memset(anim_sys, 0, sizeof(AnimationSystem));
}

bool animation_system_play(AnimationSystem* anim_sys, const char* anim_name, bool loop) {
    if (!anim_sys || !anim_name || !anim_sys->initialized) return false;
    
    // Stop current animation first
    animation_system_stop(anim_sys);
    
    // Try to create and start new animation
    anim_sys->current_anim = t3d_anim_create(anim_sys->model, anim_name);
    
    // Check if animation was created successfully
    if (anim_sys->current_anim.animRef == NULL) {
        debugf("Animation '%s' not found\n", anim_name);
        strcpy(anim_sys->current_name, "None");
        return false;
    }
    
    // Attach and start animation
    t3d_anim_attach(&anim_sys->current_anim, anim_sys->skeleton);
    t3d_anim_set_looping(&anim_sys->current_anim, loop);
    t3d_anim_set_playing(&anim_sys->current_anim, true);
    
    // Update state
    anim_sys->is_playing = true;
    strncpy(anim_sys->current_name, anim_name, sizeof(anim_sys->current_name) - 1);
    anim_sys->current_name[sizeof(anim_sys->current_name) - 1] = '\0';
    
    debugf("Playing animation: %s (loop: %s)\n", anim_name, loop ? "yes" : "no");
    return true;
}

void animation_system_stop(AnimationSystem* anim_sys) {
    if (!anim_sys || !anim_sys->is_playing) return;
    
    t3d_anim_set_playing(&anim_sys->current_anim, false);
    t3d_anim_destroy(&anim_sys->current_anim);
    
    anim_sys->is_playing = false;
    strcpy(anim_sys->current_name, "None");
    
    debugf("Animation stopped\n");
}

void animation_system_pause(AnimationSystem* anim_sys) {
    if (!anim_sys || !anim_sys->is_playing) return;
    
    t3d_anim_set_playing(&anim_sys->current_anim, false);
    debugf("Animation paused\n");
}

void animation_system_resume(AnimationSystem* anim_sys) {
    if (!anim_sys || !anim_sys->is_playing) return;
    
    t3d_anim_set_playing(&anim_sys->current_anim, true);
    debugf("Animation resumed\n");
}

const char* animation_system_get_current_name(AnimationSystem* anim_sys) {
    return anim_sys ? anim_sys->current_name : "None";
}

bool animation_system_is_playing(AnimationSystem* anim_sys) {
    return anim_sys ? anim_sys->is_playing : false;
}

bool animation_system_blend_to(AnimationSystem* anim_sys, const char* target_anim_name, float blend_speed, bool loop) {
    if (!anim_sys || !target_anim_name || !anim_sys->initialized) return false;
    
    // If not currently playing anything, just play the target animation
    if (!anim_sys->is_playing) {
        return animation_system_play(anim_sys, target_anim_name, loop);
    }
    
    // If we're already blending to this animation, nothing to do
    if (anim_sys->is_blending && strcmp(anim_sys->blend_name, target_anim_name) == 0) {
        return true;
    }
    
    // If current animation is already the target, nothing to do
    if (strcmp(anim_sys->current_name, target_anim_name) == 0) {
        return true;
    }
    
    // Create blend skeleton if it doesn't exist
    if (!anim_sys->blend_skeleton) {
        anim_sys->blend_skeleton = malloc_uncached(sizeof(T3DSkeleton));
        *anim_sys->blend_skeleton = t3d_skeleton_clone(anim_sys->skeleton, false);
        debugf("Created blend skeleton\n");
    }
    
    // If we were already blending, clean up the old blend animation
    if (anim_sys->is_blending) {
        t3d_anim_destroy(&anim_sys->blend_anim);
    }
    
    // Create and start the blend animation
    anim_sys->blend_anim = t3d_anim_create(anim_sys->model, target_anim_name);
    
    if (anim_sys->blend_anim.animRef == NULL) {
        debugf("Blend animation '%s' not found\\n", target_anim_name);
        anim_sys->is_blending = false;
        return false;
    }
    
    // Attach and start blend animation
    t3d_anim_attach(&anim_sys->blend_anim, anim_sys->blend_skeleton);
    t3d_anim_set_looping(&anim_sys->blend_anim, loop);
    t3d_anim_set_playing(&anim_sys->blend_anim, true);
    
    // Update state
    anim_sys->is_blending = true;
    strncpy(anim_sys->blend_name, target_anim_name, sizeof(anim_sys->blend_name) - 1);
    anim_sys->blend_name[sizeof(anim_sys->blend_name) - 1] = '\0';
    
    debugf("Blending from '%s' to '%s'\n", anim_sys->current_name, target_anim_name);
    return true;
}

void animation_system_set_blend_factor(AnimationSystem* anim_sys, float factor) {
    if (!anim_sys) return;
    
    // Clamp factor to [0, 1]
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    
    anim_sys->blend_factor = factor;
}

float animation_system_get_blend_factor(AnimationSystem* anim_sys) {
    return anim_sys ? anim_sys->blend_factor : 0.0f;
}

bool animation_system_is_blending(AnimationSystem* anim_sys) {
    return anim_sys ? anim_sys->is_blending : false;
}

void animation_system_update_position_blend(AnimationSystem* anim_sys, float normalized_position,
                                            const char* left_anim, const char* right_anim, bool loop) {
    if (!anim_sys || !anim_sys->initialized) return;
    
    // Clamp normalized_position to [0, 1]
    if (normalized_position < 0.0f) normalized_position = 0.0f;
    if (normalized_position > 1.0f) normalized_position = 1.0f;
    
    // Check if animation names have changed (e.g., switching from Combat to Slash animations)
    bool animations_changed = false;
    if (anim_sys->is_blending) {
        animations_changed = (strcmp(anim_sys->current_name, left_anim) != 0 ||
                             strcmp(anim_sys->blend_name, right_anim) != 0);
    }
    
    // If animations changed, stop and clean up current blend
    if (animations_changed) {
        t3d_anim_set_playing(&anim_sys->current_anim, false);
        t3d_anim_destroy(&anim_sys->current_anim);
        if (anim_sys->is_blending) {
            t3d_anim_set_playing(&anim_sys->blend_anim, false);
            t3d_anim_destroy(&anim_sys->blend_anim);
        }
        anim_sys->is_playing = false;
        anim_sys->is_blending = false;
    }
    
    // Check if we need to set up or restart the blending
    if (!anim_sys->is_blending) {
        // Start blending between left and right animations
        animation_system_play(anim_sys, left_anim, loop);
        animation_system_blend_to(anim_sys, right_anim, 0.0f, loop);
    }
    
    // Apply blend factor based on normalized position
    // 0.0 = full left animation, 1.0 = full right animation
    animation_system_set_blend_factor(anim_sys, normalized_position);
}
