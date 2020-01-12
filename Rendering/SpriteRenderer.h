#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H

#include <stdint.h>

#include "Math/vec2.h"

#define MAX_SPRITES 64

class SpriteRenderer
{
    static SpriteRenderer* renderers[MAX_SPRITES];
    static uint16_t activeSprites;

    public:
    SpriteRenderer(vec2 position, vec2 rotation, vec2 scale, const char* file);
};

#endif /* !SPRITE_RENDERER_H */
