#ifndef LEVEL4_H
#define LEVEL4_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include "animationsystem.h"
#include "playercontrols.h"
#include "outfitsystem.h"
#include "projectilesystem.h"
#include "collisionsystem.h"
#include "enemyorchestrator.h"

typedef struct {
    T3DViewport viewport;
    rdpq_font_t* font;
    
    T3DModel* mecha_model;
    T3DSkeleton* skeleton;
    AnimationSystem anim_system;
    T3DMat4FP* modelMat;
    
    // Sun map model
    T3DModel* sun_model;
    T3DSkeleton* sun_skeleton;
    AnimationSystem sun_anim_system;
    T3DMat4FP* sunMat;
    
    // Enemy model
    T3DModel* enemy_model;
    T3DMat4FP* enemyMat;
    
    // Player controls
    PlayerControls player_controls;
    
    // Outfit system
    OutfitSystem outfit_system;
    
    // Projectile system
    ProjectileSystem projectile_system;
    
    // Collision system
    CollisionSystem collision_system;
    
    // Enemy orchestrator
    EnemyOrchestrator enemy_orchestrator;
    
    // Hit display
    bool show_player_hit;
    float player_hit_timer;
    
    uint8_t colorAmbient[4];
    uint8_t colorDir[4];
    T3DVec3 lightDirVec;
    
    float last_update_time;

    wav64_t music;
} Level4;

void level4_init(Level4* level, rdpq_font_t* font);
int level4_update(Level4* level);
void level4_render(Level4* level);
void level4_cleanup(Level4* level);

#endif // LEVEL4_H
