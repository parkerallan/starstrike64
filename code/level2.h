#ifndef LEVEL2_H
#define LEVEL2_H

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
    
    // Mars map model
    T3DModel* mars_model;
    T3DSkeleton* mars_skeleton;
    AnimationSystem mars_anim_system;
    T3DMat4FP* marsMat;
    
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
} Level2;

void level2_init(Level2* level, rdpq_font_t* font);
int level2_update(Level2* level);
void level2_render(Level2* level);
void level2_cleanup(Level2* level);

#endif // LEVEL2_H
