#include <libdragon.h>
#include <t3d/t3d.h>
#include "scenes.h"
#include "intro.h"
#include "level1.h"
#include "level2.h"
#include "level3.h"
#include "level4.h"
#include "level5.h"

// Scene instances
SceneIntro scene_intro;
Level1 level1;
Level2 level2;
Level3 level3;
Level4 level4;
Level5 level5;

// Global builtin font (loaded once, reused by all scenes)
rdpq_font_t* builtin_font;

// Debug: Set starting scene (0 = INTRO, 1-5 = LEVEL_1 through LEVEL_5)
#define START_SCENE 5

GameScene current_scene = SCENE_INTRO;

int main() {
    // Initialize libdragon
    debug_init_isviewer();
    debug_init_usblog();
    asset_init_compression(2);
    dfs_init(DFS_DEFAULT_LOCATION);

    // Audio initialization
    audio_init(48000, 16);  // 48kHz, 16 buffers
    mixer_init(16);        // 16 channels
    wav64_init_compression(3);  // Opus compression

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);
    rdpq_init();
    joypad_init();

    // Initialize T3D
    t3d_init((T3DInitParams){});

    // Load builtin font once for all scenes
    builtin_font = rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_VAR);

    // Initialize starting scene based on debug define
    #if START_SCENE == 0
        intro_init(&scene_intro, builtin_font);
        current_scene = SCENE_INTRO;
    #elif START_SCENE == 1
        level1_init(&level1, builtin_font);
        current_scene = LEVEL_1;
    #elif START_SCENE == 2
        level2_init(&level2, builtin_font);
        current_scene = LEVEL_2;
    #elif START_SCENE == 3
        level3_init(&level3, builtin_font);
        current_scene = LEVEL_3;
    #elif START_SCENE == 4
        level4_init(&level4, builtin_font);
        current_scene = LEVEL_4;
    #elif START_SCENE == 5
        level5_init(&level5, builtin_font);
        current_scene = LEVEL_5;
    #else
    #error "Invalid START_SCENE value"
    #endif

    // Main loop
    while (1) {
        joypad_poll();
        
        // Handle scene updates and transitions
        int transition_result = -1;
        switch (current_scene) {
            case SCENE_INTRO:
                transition_result = intro_update(&scene_intro);
                break;
            case LEVEL_1:
                transition_result = level1_update(&level1);
                break;
            case LEVEL_2:
                transition_result = level2_update(&level2);
                break;
            case LEVEL_3:
                transition_result = level3_update(&level3);
                break;
            case LEVEL_4:
                transition_result = level4_update(&level4);
                break;
            case LEVEL_5:
                transition_result = level5_update(&level5);
                break;
            default:
                break;
        }
        
        // Handle scene transition if requested (transition_result >= 0)
        if (transition_result >= 0 && transition_result != current_scene) {
            // Cleanup current scene
            switch (current_scene) {
                case SCENE_INTRO:
                    intro_cleanup(&scene_intro);
                    break;
                case LEVEL_1:
                    level1_cleanup(&level1);
                    break;
                case LEVEL_2:
                    level2_cleanup(&level2);
                    break;
                case LEVEL_3:
                    level3_cleanup(&level3);
                    break;
                case LEVEL_4:
                    level4_cleanup(&level4);
                    break;
                case LEVEL_5:
                    level5_cleanup(&level5);
                    break;
                default:
                    break;
            }
            
            // Initialize new scene
            switch (transition_result) {
                case SCENE_INTRO:
                    intro_init(&scene_intro, builtin_font);
                    break;
                case LEVEL_1:
                    level1_init(&level1, builtin_font);
                    break;
                case LEVEL_2:
                    level2_init(&level2, builtin_font);
                    break;
                case LEVEL_3:
                    level3_init(&level3, builtin_font);
                    break;
                case LEVEL_4:
                    level4_init(&level4, builtin_font);
                    break;
                case LEVEL_5:
                    level5_init(&level5, builtin_font);
                    break;
                default:
                    break;
            }
            
            current_scene = (GameScene)transition_result;
        }
        
        // Poll audio mixer (required for audio playback)
        mixer_try_play();
        
        // Render current scene
        switch (current_scene) {
            case SCENE_INTRO:
                intro_render(&scene_intro);
                break;
            case LEVEL_1:
                level1_render(&level1);
                break;
            case LEVEL_2:
                level2_render(&level2);
                break;
            case LEVEL_3:
                level3_render(&level3);
                break;
            case LEVEL_4:
                level4_render(&level4);
                break;
            case LEVEL_5:
                level5_render(&level5);
                break;
            default:
                break;
        }
    }

    // Cleanup current scene
    switch (current_scene) {
        case SCENE_INTRO:
            intro_cleanup(&scene_intro);
            break;
        case LEVEL_1:
            level1_cleanup(&level1);
            break;
        case LEVEL_2:
            level2_cleanup(&level2);
            break;
        case LEVEL_3:
            level3_cleanup(&level3);
            break;
        case LEVEL_4:
            level4_cleanup(&level4);
            break;
        case LEVEL_5:
            level5_cleanup(&level5);
            break;
        default:
            break;
    }
    
    t3d_destroy();
    return 0;
}
