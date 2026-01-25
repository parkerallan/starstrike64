#include "animationsystem.h"
#include <string.h>
#include <malloc.h>

void animation_system_init(AnimationSystem* anim_sys, T3DModel* model, T3DSkeleton* skeleton) {
    if (!anim_sys || !model || !skeleton) return;
    
    // Initialize system
    memset(anim_sys, 0, sizeof(AnimationSystem));
    anim_sys->model = model;
    anim_sys->skeleton = skeleton;
    anim_sys->is_playing = false;
    strcpy(anim_sys->current_name, "None");
    
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
    
    // Always update skeleton
    t3d_skeleton_update(anim_sys->skeleton);
}

void animation_system_cleanup(AnimationSystem* anim_sys) {
    if (!anim_sys) return;
    
    // Stop current animation
    animation_system_stop(anim_sys);
    
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
