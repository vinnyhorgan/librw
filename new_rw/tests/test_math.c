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
test_matrix_multiply(void)
{
    RwMatrix parent, child, out;

    rw_matrix_set_identity(&parent);
    rw_matrix_rotate(&parent, (RwV3d){0, 0, 1}, 90.0f, RW_COMBINE_REPLACE);
    rw_matrix_translate(&parent, (RwV3d){10, 0, 0}, RW_COMBINE_POSTCONCAT);

    rw_matrix_set_identity(&child);
    rw_matrix_translate(&child, (RwV3d){2, 0, 0}, RW_COMBINE_REPLACE);

    rw_matrix_multiply(&out, &child, &parent);
    assert_v3d(out.pos, 10.0f, 2.0f, 0.0f);
}

static void
test_replace_ops(void)
{
    RwMatrix m;

    rw_matrix_set_identity(&m);
    rw_matrix_rotate(&m, (RwV3d){0, 0, 1}, 90.0f, RW_COMBINE_REPLACE);
    rw_matrix_translate(&m, (RwV3d){1, 2, 3}, RW_COMBINE_REPLACE);
    assert_v3d(m.right, 1.0f, 0.0f, 0.0f);
    assert_v3d(m.up, 0.0f, 1.0f, 0.0f);
    assert_v3d(m.at, 0.0f, 0.0f, 1.0f);
    assert_v3d(m.pos, 1.0f, 2.0f, 3.0f);

    rw_matrix_translate(&m, (RwV3d){5, 6, 7}, RW_COMBINE_REPLACE);
    rw_matrix_scale(&m, (RwV3d){2, 3, 4}, RW_COMBINE_REPLACE);
    assert_v3d(m.right, 2.0f, 0.0f, 0.0f);
    assert_v3d(m.up, 0.0f, 3.0f, 0.0f);
    assert_v3d(m.at, 0.0f, 0.0f, 4.0f);
    assert_v3d(m.pos, 0.0f, 0.0f, 0.0f);
}

static void
test_matrix_invert(void)
{
    RwMatrix m, inv, ident;

    rw_matrix_set_identity(&m);
    rw_matrix_rotate(&m, (RwV3d){0, 0, 1}, 45.0f, RW_COMBINE_REPLACE);
    rw_matrix_translate(&m, (RwV3d){3, 4, 5}, RW_COMBINE_POSTCONCAT);

    assert(rw_matrix_invert(&inv, &m));
    rw_matrix_multiply(&ident, &m, &inv);
    assert_v3d(ident.right, 1.0f, 0.0f, 0.0f);
    assert_v3d(ident.up, 0.0f, 1.0f, 0.0f);
    assert_v3d(ident.at, 0.0f, 0.0f, 1.0f);
    assert_v3d(ident.pos, 0.0f, 0.0f, 0.0f);
}

int
main(void)
{
    test_matrix_multiply();
    test_replace_ops();
    test_matrix_invert();
    return 0;
}
