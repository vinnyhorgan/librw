#include "../rw.h"

#include <assert.h>
#include <math.h>

static int
near(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

static void
test_skin_data(void)
{
    RwSkin *skin = rw_skin_create(2, 3, 4);

    assert(skin);
    assert(skin->num_bones == 2);
    assert(skin->num_weights == 4);
    assert(skin->inverse_matrices);
    assert(skin->bone_indices);
    assert(skin->weights);
    skin->bone_indices[11] = 1;
    skin->weights[11] = 0.5f;
    assert(skin->bone_indices[11] == 1);
    assert(skin->weights[11] == 0.5f);

    rw_skin_destroy(skin);
}

static void
test_hanim_interpolate_and_update(void)
{
    RwHAnimNodeInfo nodes[2] = {
        {0, 0, RW_HANIM_PUSH, NULL},
        {1, 1, RW_HANIM_POP, NULL},
    };
    RwHAnimHier *h = rw_hanim_create(2, nodes);
    RwFrame *root = rw_frame_create();
    RwFrame *child = rw_frame_create();
    RwHAnimKeyFrame anim[4];

    assert(h && root && child);
    rw_frame_add_child(root, child);
    rw_hanim_attach(h, root);
    assert(h->nodes[0].frame == root);
    assert(h->nodes[1].frame == child);

    anim[0].q = (RwQuat){0, 0, 0, 1}; anim[0].t = (RwV3d){0, 0, 0}; anim[0].time = 0.0f;
    anim[1].q = (RwQuat){0, 0, 0, 1}; anim[1].t = (RwV3d){0, 1, 0}; anim[1].time = 0.0f;
    anim[2].q = (RwQuat){0, 0, 0, 1}; anim[2].t = (RwV3d){10, 0, 0}; anim[2].time = 1.0f;
    anim[3].q = (RwQuat){0, 0, 0, 1}; anim[3].t = (RwV3d){0, 3, 0}; anim[3].time = 1.0f;

    h->anim_data = anim;
    h->num_frames = 2;
    h->frame_rate = 1.0f;
    rw_hanim_interpolate(h, 0.5f);
    assert(near(h->keyframes[0].t.x, 5.0f));
    assert(near(h->keyframes[1].t.y, 2.0f));

    rw_hanim_update_matrices(h);
    assert(near(h->matrices[0].pos.x, 5.0f));
    assert(near(h->matrices[1].pos.x, 5.0f));
    assert(near(h->matrices[1].pos.y, 2.0f));
    assert(near(rw_frame_get_ltm(child)->pos.x, 5.0f));
    assert(near(rw_frame_get_ltm(child)->pos.y, 2.0f));

    rw_frame_destroy(root);
    rw_hanim_destroy(h);
}

int
main(void)
{
    assert(rw_engine_init(NULL));
    test_skin_data();
    test_hanim_interpolate_and_update();
    rw_engine_term();
    return 0;
}
