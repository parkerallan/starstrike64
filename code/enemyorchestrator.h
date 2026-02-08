#ifndef ENEMYORCHESTRATOR_H
#define ENEMYORCHESTRATOR_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>
#include "collisionsystem.h"
#include "enemysystem.h"
#include "animationsystem.h"

#define MAX_ENEMIES 16

// Enemy instance
typedef struct {
    T3DMat4FP* matrix;
    EnemySystem system;
    bool active;
    float spawn_time;
    T3DVec3 position;
    T3DVec3 velocity;
    T3DVec3 target_position;    // Target position for movement phase
    int collision_start_index;  // Index in collision system where this enemy's boxes start
    int collision_count;        // Number of collision boxes for this enemy
    bool show_hit;              // Visual hit indicator
    float hit_timer;            // Timer for hit display
    int movement_phase;         // 0=flying in, 1=paused, 2=flying off
    float phase_timer;          // Timer for current phase
    float shoot_timer;          // Timer for shooting projectiles
    bool has_explosion;         // True if explosion is active
    float explosion_timer;      // Timer for explosion display (1 second)
    T3DVec3 explosion_position; // Position where explosion should appear
} EnemyInstance;

// Enemy orchestrator for a level
typedef struct {
    EnemyInstance enemies[MAX_ENEMIES];
    T3DModel* enemy_model;
    T3DModel* bomber_model;  // Special bomber model for level 2
    T3DSkeleton* bomber_skeleton;  // Skeleton for bomber animation
    AnimationSystem* bomber_anim_system;  // Animation system for bomber (pointer to avoid alignment issues)
    CollisionSystem* collision_system;
    float elapsed_time;
    float last_spawn_time;  // Track last spawn for patterns
    int active_count;
    int wave_count;         // Track number of waves spawned (for level 1)
    T3DModel* explosion_model;  // Shared explosion model for all enemies
    T3DMat4FP** explosion_matrices;  // Array of matrices for each explosion
    int bomber_phase;       // 0=retreat, 1=approach, 2=strafe, 3=transition_to_wave, 4=wave pattern
    float bomber_phase_timer;  // Timer for bomber phase transitions
    
    // Level 4 boss state
    T3DModel* boss_model;
    T3DSkeleton* boss_skeleton;
    AnimationSystem boss_anim;
    float boss_side_progress;  // 0-1 for side-to-side movement
    bool boss_moving_right;
    float boss_barrage_cooldown;
    float boss_spin_timer;
} EnemyOrchestrator;

// Initialize orchestrator
void enemy_orchestrator_init(EnemyOrchestrator* orch, T3DModel* enemy_model, CollisionSystem* collision_system);

// Update for Level 1 - Simple shmup pattern (3 enemies in line, every 3s)
void enemy_orchestrator_update_level1(EnemyOrchestrator* orch, float delta_time);

// Update for Level 2 - Wave pattern (5 enemies in V-formation, every 4s)
void enemy_orchestrator_update_level2(EnemyOrchestrator* orch, float delta_time);

// Update for Level 3 - Zigzag pattern (enemies move left/right)
void enemy_orchestrator_update_level3(EnemyOrchestrator* orch, float delta_time);

// Check if a point (projectile) hits any enemy and apply damage
// Returns true if hit detected, enemy_index will be set to the hit enemy
bool enemy_orchestrator_check_hit(EnemyOrchestrator* orch, const T3DVec3* position, int* enemy_index, int damage);

// Spawn a single enemy
void enemy_orchestrator_spawn_enemy(
    EnemyOrchestrator* orch,
    float x, float y, float z,
    float vel_x, float vel_y, float vel_z
);

// Get enemy matrix by index for rendering
T3DMat4FP* enemy_orchestrator_get_matrix(EnemyOrchestrator* orch, int index);

// Get enemy system by index for hit detection
EnemySystem* enemy_orchestrator_get_system(EnemyOrchestrator* orch, int index);

// Check if enemy is active
bool enemy_orchestrator_is_active(EnemyOrchestrator* orch, int index);

// Get active enemy count
int enemy_orchestrator_get_active_count(EnemyOrchestrator* orch);

// Check if all waves are complete and no enemies remain
bool enemy_orchestrator_all_waves_complete(EnemyOrchestrator* orch, int max_waves);

// Get explosion matrix for rendering (returns NULL if no explosion)
T3DMat4FP* enemy_orchestrator_get_explosion_matrix(EnemyOrchestrator* orch, int index);

// Check if enemy has active explosion
bool enemy_orchestrator_has_explosion(EnemyOrchestrator* orch, int index);

// Get explosion model
T3DModel* enemy_orchestrator_get_explosion_model(EnemyOrchestrator* orch);

// Cleanup
void enemy_orchestrator_cleanup(EnemyOrchestrator* orch);

// Spawn enemy projectiles during level 1 (pass projectile system from level)
void enemy_orchestrator_spawn_projectiles_level1(EnemyOrchestrator* orch, void* projectile_system, float delta_time);

// Spawn enemy projectiles during level 2 (bomber pattern)
void enemy_orchestrator_spawn_projectiles_level2(EnemyOrchestrator* orch, void* projectile_system, float delta_time);

// Spawn enemy projectiles during level 3 (zigzag pattern)
void enemy_orchestrator_spawn_projectiles_level3(EnemyOrchestrator* orch, void* projectile_system, float delta_time);

// Level 4 Boss functions
void enemy_orchestrator_init_level4_boss(EnemyOrchestrator* orch, CollisionSystem* collision_system);
void enemy_orchestrator_update_level4_boss(EnemyOrchestrator* orch, float delta_time, void* projectile_system);
T3DModel* enemy_orchestrator_get_boss_model(EnemyOrchestrator* orch);
T3DSkeleton* enemy_orchestrator_get_boss_skeleton(EnemyOrchestrator* orch);

#endif // ENEMYORCHESTRATOR_H
