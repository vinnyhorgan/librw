#include "rw.h"
#include <stdlib.h>
#include <string.h>

static int engine_state = RW_ENGINE_DEAD;

RwEngine rw_engine;

void *
rw_malloc(size_t size)
{
    return rw_engine.malloc(size);
}

void *
rw_realloc(void *ptr, size_t size)
{
    return rw_engine.realloc(ptr, size);
}

void
rw_free(void *ptr)
{
    rw_engine.free(ptr);
}

int
rw_engine_init(RwMemoryFuncs *memfuncs)
{
    if (engine_state != RW_ENGINE_DEAD)
        return 0;

    if (memfuncs) {
        if (!memfuncs->malloc || !memfuncs->realloc || !memfuncs->free)
            return 0;
        rw_engine.malloc  = memfuncs->malloc;
        rw_engine.realloc = memfuncs->realloc;
        rw_engine.free    = memfuncs->free;
    } else {
        rw_engine.malloc  = malloc;
        rw_engine.realloc = realloc;
        rw_engine.free    = free;
    }

    rw_linklist_init(&rw_engine.frame_dirty_list);
    memset(rw_engine.render_states, 0, sizeof(rw_engine.render_states));
    rw_engine.current_camera = NULL;
    rw_engine.current_world = NULL;

    engine_state = RW_ENGINE_INITIALIZED;
    return 1;
}

int
rw_engine_open(void)
{
    if (engine_state != RW_ENGINE_INITIALIZED)
        return 0;

    engine_state = RW_ENGINE_OPENED;
    return 1;
}

int
rw_engine_start(void)
{
    if (engine_state != RW_ENGINE_OPENED)
        return 0;

    rw_set_render_state(RW_STATE_ZTESTENABLE, 1);
    rw_set_render_state(RW_STATE_ZWRITEENABLE, 1);
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_CCW);
    rw_set_render_state(RW_STATE_SRCBLEND, RW_BLEND_SRCALPHA);
    rw_set_render_state(RW_STATE_DESTBLEND, RW_BLEND_INVSRCALPHA);
    rw_set_render_state(RW_STATE_ALPHATEST, 0);
    rw_set_render_state(RW_STATE_ALPHAREF, 0);
    rw_set_render_state(RW_STATE_FOGENABLE, 0);
    rw_set_render_state(RW_STATE_FOGCOLOR, 0);
    rw_set_render_state(RW_STATE_TEXTURERASTER, 0);
    rw_set_render_state(RW_STATE_VERTEXALPHA, 0);

    engine_state = RW_ENGINE_STARTED;
    return 1;
}

void
rw_engine_stop(void)
{
    if (engine_state != RW_ENGINE_STARTED)
        return;
    engine_state = RW_ENGINE_OPENED;
}

void
rw_engine_close(void)
{
    if (engine_state != RW_ENGINE_OPENED)
        return;
    engine_state = RW_ENGINE_INITIALIZED;
}

void
rw_engine_term(void)
{
    if (engine_state != RW_ENGINE_INITIALIZED)
        return;
    memset(&rw_engine, 0, sizeof(rw_engine));
    engine_state = RW_ENGINE_DEAD;
}

void
rw_set_render_state(int state, uint32_t value)
{
    if (state >= 0 && state < RW_STATE_NUM_STATES)
        rw_engine.render_states[state] = value;
}

uint32_t
rw_get_render_state(int state)
{
    if (state >= 0 && state < RW_STATE_NUM_STATES)
        return rw_engine.render_states[state];
    return 0;
}
