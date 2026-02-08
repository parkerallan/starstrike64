#include <libdragon.h>
#include <t3d/t3d.h>
#include "scenes.h"
#include "startup.h"
#include "intro.h"
#include "level1.h"
#include "level2.h"
#include "level3.h"
#include "level4.h"
#include "level5.h"
#include "end.h"

// Scene instances
SceneStartup scene_startup;
SceneIntro scene_intro;
Level1 level1;
Level2 level2;
Level3 level3;
Level4 level4;
Level5 level5;
SceneEnd scene_end;

// Global builtin font (loaded once, reused by all scenes)
rdpq_font_t* builtin_font;

// Debug: Set starting scene (0 = STARTUP, 1 = INTRO, 2-6 = LEVEL_1 through LEVEL_5, 7 = END)
#define START_SCENE 0

GameScene current_scene = SCENE_STARTUP;

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

    // Load Prototype font once for all scenes
    builtin_font = rdpq_font_load("rom:/Prototype.font64");
    rdpq_font_style(builtin_font, 0, &(rdpq_fontstyle_t){
        .color = RGBA32(0xFF, 0xFF, 0xFF, 0xFF), // White color
    });

    // Initialize starting scene based on debug define
    #if START_SCENE == 0
        startup_init(&scene_startup, builtin_font);
        current_scene = SCENE_STARTUP;
    #elif START_SCENE == 1
        intro_init(&scene_intro, builtin_font);
        current_scene = SCENE_INTRO;
    #elif START_SCENE == 2
        level1_init(&level1, builtin_font);
        current_scene = LEVEL_1;
    #elif START_SCENE == 3
        level2_init(&level2, builtin_font);
        current_scene = LEVEL_2;
    #elif START_SCENE == 4
        level3_init(&level3, builtin_font);
        current_scene = LEVEL_3;
    #elif START_SCENE == 5
        level4_init(&level4, builtin_font);
        current_scene = LEVEL_4;
    #elif START_SCENE == 6
        level5_init(&level5, builtin_font);
        current_scene = LEVEL_5;
    #elif START_SCENE == 7
        end_init(&scene_end, builtin_font);
        current_scene = SCENE_END;
    #else
    #error "Invalid START_SCENE value"
    #endif

    // Main loop
    while (1) {
        joypad_poll();
        
        // Handle scene updates and transitions
        int transition_result = -1;
        switch (current_scene) {
            case SCENE_STARTUP:
                transition_result = startup_update(&scene_startup);
                break;
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
            case SCENE_END:
                transition_result = end_update(&scene_end);
                break;
            default:
                break;
        }
        
        // Handle scene transition if requested (transition_result >= 0)
        if (transition_result >= 0) {
            // Cleanup current scene
            switch (current_scene) {
                case SCENE_STARTUP:
                    startup_cleanup(&scene_startup);
                    break;
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
                case SCENE_END:
                    end_cleanup(&scene_end);
                    break;
                default:
                    break;
            }
            
            // Initialize new scene
            switch (transition_result) {
                case SCENE_STARTUP:
                    startup_init(&scene_startup, builtin_font);
                    break;
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
                case SCENE_END:
                    end_init(&scene_end, builtin_font);
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
            case SCENE_STARTUP:
                startup_render(&scene_startup);
                break;
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
            case SCENE_END:
                end_render(&scene_end);
                break;
            default:
                break;
        }
    }

    // Cleanup current scene
    switch (current_scene) {
        case SCENE_STARTUP:
            startup_cleanup(&scene_startup);
            break;
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
        case SCENE_END:
            end_cleanup(&scene_end);
            break;
        default:
            break;
    }
    
    t3d_destroy();
    return 0;
}
