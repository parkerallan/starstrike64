#ifndef LEVEL1_H
#define LEVEL1_H

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
    
    // Character model
    T3DModel* mecha_model;
    T3DSkeleton* skeleton;
    AnimationSystem anim_system;
    T3DMat4FP* modelMat;
    
    // Stars map model
    T3DModel* stars_model;
    T3DSkeleton* stars_skeleton;
    AnimationSystem stars_anim_system;
    T3DMat4FP* starsMat;
    
    // Enemy model and orchestrator
    T3DModel* enemy_model;
    EnemyOrchestrator enemy_orchestrator;
    
    // Player controls
    PlayerControls player_controls;
    
    // Outfit system
    OutfitSystem outfit_system;
    
    // Projectile system
    ProjectileSystem projectile_system;
    
    // Collision system
    CollisionSystem collision_system;
    
    // Hit display
    bool show_enemy_hit;
    bool show_player_hit;
    float enemy_hit_timer;
    float player_hit_timer;
    
    // Lighting
    uint8_t colorAmbient[4];
    uint8_t colorDir[4];
    T3DVec3 lightDirVec;
    
    float last_update_time;
} Level1;

void level1_init(Level1* level, rdpq_font_t* font);
int level1_update(Level1* level);
void level1_render(Level1* level);
void level1_cleanup(Level1* level);

#endif // LEVEL1_H
