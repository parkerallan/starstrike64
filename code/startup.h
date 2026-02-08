#ifndef STARTUP_H
#define STARTUP_H

#include <libdragon.h>
#include <t3d/t3d.h>

typedef struct {
    float scene_time;
    float last_update_time;
    rdpq_font_t* font;
    sprite_t* libdragon_sprite;
    sprite_t* tiny3d_sprite;
    wav64_t startup_sound;
    int current_logo; // 0 = libdragon, 1 = tiny3d
    bool sound_played;
} SceneStartup;

void startup_init(SceneStartup* scene, rdpq_font_t* font);
int startup_update(SceneStartup* scene);
void startup_render(SceneStartup* scene);
void startup_cleanup(SceneStartup* scene);

#endif // STARTUP_H
