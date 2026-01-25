#ifndef INTRO_H
#define INTRO_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include "animationsystem.h"

typedef struct {
    T3DViewport viewport;
    rdpq_font_t* font;
    
    // Character model
    T3DModel* mecha_model;
    T3DSkeleton* skeleton;
    AnimationSystem anim_system;
    T3DMat4FP* modelMat;
    
    // Map model
    T3DModel* tunnel_model;
    T3DMat4FP* tunnelMat;
    
    // UI sprites
    sprite_t* logo_sprite;
    sprite_t* press_start_sprite;
    
    // Lighting
    uint8_t colorAmbient[4];
    uint8_t colorDir[4];
    T3DVec3 lightDirVec;
    
    // Music
    wav64_t music;
    
    float last_update_time;
    float scene_time;
} SceneIntro;

void intro_init(SceneIntro* scene, rdpq_font_t* font);
int intro_update(SceneIntro* scene);
void intro_render(SceneIntro* scene);
void intro_cleanup(SceneIntro* scene);

#endif // INTRO_H
