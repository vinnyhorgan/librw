#include "../rw.h"

#include <assert.h>
#include <math.h>

static int
near(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

static void
assert_v3d(RwV3d v, float x, float y, float z)
{
    assert(near(v.x, x));
    assert(near(v.y, y));
    assert(near(v.z, z));
}

static void
test_hierarchy_ltm(void)
{
    RwFrame *root = rw_frame_create();
    RwFrame *child = rw_frame_create();
    RwMatrix expected;

    assert(root);
    assert(child);

    rw_frame_rotate(root, (RwV3d){0, 0, 1}, 90.0f, RW_COMBINE_REPLACE);
    rw_frame_translate(root, (RwV3d){10, 0, 0}, RW_COMBINE_POSTCONCAT);
    rw_frame_translate(child, (RwV3d){2, 0, 0}, RW_COMBINE_REPLACE);
    rw_frame_add_child(root, child);

    rw_frame_sync_dirty();
    rw_matrix_multiply(&expected, &child->matrix, &root->ltm);
    assert_v3d(child->ltm.pos, expected.pos.x, expected.pos.y, expected.pos.z);
    assert_v3d(child->ltm.pos, 10.0f, 2.0f, 0.0f);

    rw_frame_destroy(root);
}

static void
test_get_ltm_syncs_dirty_root(void)
{
    RwFrame *root = rw_frame_create();
    RwFrame *child = rw_frame_create();
    RwMatrix *ltm;

    assert(root);
    assert(child);

    rw_frame_add_child(root, child);
    rw_frame_translate(root, (RwV3d){3, 0, 0}, RW_COMBINE_REPLACE);
    rw_frame_translate(child, (RwV3d){0, 4, 0}, RW_COMBINE_REPLACE);

    ltm = rw_frame_get_ltm(child);
    assert(ltm == &child->ltm);
    assert_v3d(ltm->pos, 3.0f, 4.0f, 0.0f);
    assert(rw_linklist_isempty(&rw_engine.frame_dirty_list));

    rw_frame_destroy(root);
}

static void
test_remove_child_makes_new_root(void)
{
    RwFrame *root = rw_frame_create();
    RwFrame *child = rw_frame_create();

    assert(root);
    assert(child);

    rw_frame_translate(root, (RwV3d){10, 0, 0}, RW_COMBINE_REPLACE);
    rw_frame_translate(child, (RwV3d){2, 0, 0}, RW_COMBINE_REPLACE);
    rw_frame_add_child(root, child);
    rw_frame_sync_dirty();
    assert_v3d(child->ltm.pos, 12.0f, 0.0f, 0.0f);

    rw_frame_remove_child(child);
    rw_frame_sync_dirty();
    assert(child->parent == NULL);
    assert(child->root == child);
    assert_v3d(child->ltm.pos, 2.0f, 0.0f, 0.0f);

    rw_frame_destroy(child);
    rw_frame_destroy(root);
}

int
main(void)
{
    assert(rw_engine_init(NULL));
    test_hierarchy_ltm();
    test_get_ltm_syncs_dirty_root();
    test_remove_child_makes_new_root();
    rw_engine_term();
    return 0;
}
