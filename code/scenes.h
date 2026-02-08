#ifndef SCENES_H
#define SCENES_H

// Scene enumeration - return these values from scene_update() functions
typedef enum {
    SCENE_STARTUP = 0,
    SCENE_INTRO = 1,
    LEVEL_1 = 2,
    LEVEL_2 = 3,
    LEVEL_3 = 4,
    LEVEL_4 = 5,
    LEVEL_5 = 6,
    SCENE_END = 7,
    SCENE_COUNT = 8
} GameScene;

// Return -1 from scene_update() to indicate no transition

#endif // SCENES_H
