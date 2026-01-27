#ifndef LEVEL3_H
#define LEVEL3_H

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
    
    // Jupiter map model
    T3DModel* jupiter_model;
    T3DSkeleton* jupiter_skeleton;
    AnimationSystem jupiter_anim_system;
    T3DMat4FP* jupiterMat;
    
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
} Level3;

void level3_init(Level3* level, rdpq_font_t* font);
int level3_update(Level3* level);
void level3_render(Level3* level);
void level3_cleanup(Level3* level);

#endif // LEVEL3_H
