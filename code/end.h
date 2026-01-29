#ifndef END_H
#define END_H

#include <libdragon.h>
#include <t3d/t3d.h>

#define MAX_CREDITS_LINES 256
#define MAX_LINE_LENGTH 128

typedef struct {
    rdpq_font_t* font;
    
    // Credits text
    char credits_lines[MAX_CREDITS_LINES][MAX_LINE_LENGTH];
    int line_count;
    
    // Scrolling
    float scroll_offset;
    float scroll_speed;
    bool scroll_complete;
    
    // Timing
    float last_update_time;
    float scene_time;

    wav64_t music;
} SceneEnd;

void end_init(SceneEnd* scene, rdpq_font_t* font);
int end_update(SceneEnd* scene);
void end_render(SceneEnd* scene);
void end_cleanup(SceneEnd* scene);

#endif // END_H
