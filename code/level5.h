#ifndef LEVEL5_H
#define LEVEL5_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include "animationsystem.h"
#include "playercontrols.h"
#include "outfitsystem.h"
#include "projectilesystem.h"

typedef struct {
    T3DViewport viewport;
    rdpq_font_t* font;
    
    T3DModel* mecha_model;
    T3DSkeleton* skeleton;
    AnimationSystem anim_system;
    T3DMat4FP* modelMat;
    
    // Mercury map model
    T3DModel* mercury_model;
    T3DSkeleton* mercury_skeleton;
    AnimationSystem mercury_anim_system;
    T3DMat4FP* mercuryMat;
    
    // Player controls
    PlayerControls player_controls;
    
    // Outfit system
    OutfitSystem outfit_system;
    
    // Projectile system
    ProjectileSystem projectile_system;
    
    uint8_t colorAmbient[4];
    uint8_t colorDir[4];
    T3DVec3 lightDirVec;
    
    float last_update_time;
} Level5;

void level5_init(Level5* level, rdpq_font_t* font);
int level5_update(Level5* level);
void level5_render(Level5* level);
void level5_cleanup(Level5* level);

#endif // LEVEL5_H
