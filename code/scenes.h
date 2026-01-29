#ifndef SCENES_H
#define SCENES_H

// Scene enumeration - return these values from scene_update() functions
typedef enum {
    SCENE_INTRO = 0,
    LEVEL_1 = 1,
    LEVEL_2 = 2,
    LEVEL_3 = 3,
    LEVEL_4 = 4,
    LEVEL_5 = 5,
    SCENE_END = 6,
    SCENE_COUNT = 7
} GameScene;

// Return -1 from scene_update() to indicate no transition

#endif // SCENES_H
