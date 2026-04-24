#include "rw.h"
#include "rw_gl_internal.h"

#include <string.h>

RwSkin *
rw_skin_create(int num_bones, int num_verts, int num_weights)
{
    RwSkin *skin;
    size_t matrix_count;
    size_t weight_count;

    if (num_bones <= 0 || num_verts <= 0 || num_weights <= 0)
        return NULL;

    skin = rw_malloc(sizeof(*skin));
    if (!skin)
        return NULL;
    memset(skin, 0, sizeof(*skin));

    skin->num_bones = num_bones;
    skin->num_weights = num_weights;

    matrix_count = (size_t)num_bones * 16u;
    weight_count = (size_t)num_verts * (size_t)num_weights;

    skin->inverse_matrices = rw_malloc(matrix_count * sizeof(*skin->inverse_matrices));
    skin->bone_indices = rw_malloc(weight_count * sizeof(*skin->bone_indices));
    skin->weights = rw_malloc(weight_count * sizeof(*skin->weights));
    if (!skin->inverse_matrices || !skin->bone_indices || !skin->weights) {
        rw_skin_destroy(skin);
        return NULL;
    }

    memset(skin->inverse_matrices, 0, matrix_count * sizeof(*skin->inverse_matrices));
    memset(skin->bone_indices, 0, weight_count * sizeof(*skin->bone_indices));
    memset(skin->weights, 0, weight_count * sizeof(*skin->weights));
    return skin;
}

void
rw_skin_destroy(RwSkin *skin)
{
    if (!skin)
        return;

    rw_free(skin->inverse_matrices);
    rw_free(skin->bone_indices);
    rw_free(skin->weights);
    rw_free(skin);
}

void
rw_skin_set_data(RwSkin *skin, uint8_t *indices, float *weights, float *inv_matrices)
{
    if (!skin)
        return;

    if (indices && indices != skin->bone_indices) {
        rw_free(skin->bone_indices);
        skin->bone_indices = indices;
    }
    if (weights && weights != skin->weights) {
        rw_free(skin->weights);
        skin->weights = weights;
    }
    if (inv_matrices && inv_matrices != skin->inverse_matrices) {
        rw_free(skin->inverse_matrices);
        skin->inverse_matrices = inv_matrices;
    }
}

void
rw_skin_set_pipeline(RwAtomic *a)
{
    if (!a || !a->geometry)
        return;
    rw_atomic_set_pipeline(a, rw_gl_skin_pipeline());
}

RwHAnimHier *
rw_hanim_create(int num_nodes, RwHAnimNodeInfo *nodes)
{
    RwHAnimHier *h;
    int i;

    if (num_nodes <= 0 || !nodes)
        return NULL;

    h = rw_malloc(sizeof(*h));
    if (!h)
        return NULL;
    memset(h, 0, sizeof(*h));

    h->num_nodes = num_nodes;
    h->nodes = rw_malloc((size_t)num_nodes * sizeof(*h->nodes));
    h->matrices = rw_malloc((size_t)num_nodes * sizeof(*h->matrices));
    h->keyframes = rw_malloc((size_t)num_nodes * sizeof(*h->keyframes));
    if (!h->nodes || !h->matrices || !h->keyframes) {
        rw_hanim_destroy(h);
        return NULL;
    }

    memcpy(h->nodes, nodes, (size_t)num_nodes * sizeof(*h->nodes));
    for (i = 0; i < num_nodes; i++) {
        rw_matrix_set_identity(&h->matrices[i]);
        h->keyframes[i].q = (RwQuat){0, 0, 0, 1};
        h->keyframes[i].t = (RwV3d){0, 0, 0};
        h->keyframes[i].time = 0.0f;
    }
    h->num_keyframes = num_nodes;
    return h;
}

void
rw_hanim_destroy(RwHAnimHier *h)
{
    if (!h)
        return;

    rw_free(h->nodes);
    rw_free(h->matrices);
    rw_free(h->keyframes);
    rw_free(h);
}

void
rw_hanim_attach(RwHAnimHier *h, RwFrame *root)
{
    RwFrame *frame;
    RwFrame *stack[64];
    int depth = 0;
    int i;

    if (!h || !root)
        return;

    frame = root;
    for (i = 0; i < h->num_nodes && frame; i++) {
        h->nodes[i].frame = frame;

        if ((h->nodes[i].flags & RW_HANIM_PUSH) && frame->child && depth < 63) {
            stack[depth++] = frame;
            frame = frame->child;
        } else {
            while (frame && !frame->next && depth > 0)
                frame = stack[--depth];
            frame = frame ? frame->next : NULL;
        }

        if ((h->nodes[i].flags & RW_HANIM_POP) && depth > 0 && (!frame || !frame->next)) {
            frame = stack[--depth];
            frame = frame->next;
        }
    }
}

void
rw_hanim_update_matrices(RwHAnimHier *h)
{
    int i;
    int depth = 0;
    RwMatrix stack[64];

    if (!h || !h->nodes || !h->matrices || !h->keyframes)
        return;

    rw_matrix_set_identity(&stack[0]);
    for (i = 0; i < h->num_nodes; i++) {
        RwMatrix local;
        RwMatrix global;
        int index = h->nodes[i].index;

        if (index < 0 || index >= h->num_nodes)
            index = i;

        rw_quat_to_matrix(&local, h->keyframes[index].q);
        local.pos = h->keyframes[index].t;
        rw_matrix_multiply(&global, &local, &stack[depth]);
        h->matrices[index] = global;

        if (h->nodes[i].frame)
            rw_frame_set_matrix(h->nodes[i].frame, &local);

        if ((h->nodes[i].flags & RW_HANIM_PUSH) && depth < 63)
            stack[++depth] = global;
        else
            stack[depth] = global;

        if ((h->nodes[i].flags & RW_HANIM_POP) && depth > 0)
            depth--;
    }
}

void
rw_hanim_interpolate(RwHAnimHier *h, float time)
{
    int frame0;
    int frame1;
    int node;
    float frame_pos;
    float t;

    if (!h || !h->anim_data || !h->keyframes || h->num_nodes <= 0 || h->num_frames <= 0)
        return;

    h->current_time = time;
    if (h->frame_rate <= 0.0f || h->num_frames == 1) {
        memcpy(h->keyframes, h->anim_data, (size_t)h->num_nodes * sizeof(*h->keyframes));
        return;
    }

    frame_pos = time * h->frame_rate;
    if (frame_pos < 0.0f)
        frame_pos = 0.0f;
    if (frame_pos > (float)(h->num_frames - 1))
        frame_pos = (float)(h->num_frames - 1);

    frame0 = (int)frame_pos;
    frame1 = frame0 + 1;
    if (frame1 >= h->num_frames)
        frame1 = frame0;
    t = frame_pos - (float)frame0;

    for (node = 0; node < h->num_nodes; node++) {
        RwHAnimKeyFrame *a = &h->anim_data[frame0 * h->num_nodes + node];
        RwHAnimKeyFrame *b = &h->anim_data[frame1 * h->num_nodes + node];

        h->keyframes[node].q = rw_quat_slerp(a->q, b->q, t);
        h->keyframes[node].t = rw_v3d_lerp(a->t, b->t, t);
        h->keyframes[node].time = time;
    }
}
