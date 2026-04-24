#include "rw.h"

#include <string.h>

static void
rw_frame_set_root(RwFrame *f, RwFrame *root)
{
    RwFrame *child;

    f->root = root;
    for (child = f->child; child; child = child->next)
        rw_frame_set_root(child, root);
}

static void
rw_frame_mark_dirty(RwFrame *f)
{
    RwFrame *root;

    if (!f)
        return;

    root = f->root ? f->root : f;
    if (!(root->dirty & RW_FRAME_HIERARCHYSYNC))
        rw_linklist_add(&rw_engine.frame_dirty_list, &root->in_dirty);
    root->dirty |= RW_FRAME_HIERARCHYSYNC;
    f->dirty |= RW_FRAME_SUBTREESYNC;
}

static void
rw_frame_sync_children(RwFrame *f, uint32_t dirty)
{
    for (; f; f = f->next) {
        dirty |= f->dirty;
        if (dirty & RW_FRAME_SUBTREESYNC) {
            rw_matrix_multiply(&f->ltm, &f->matrix, &f->parent->ltm);
            f->dirty &= ~RW_FRAME_SUBTREESYNC;
        }
        rw_frame_sync_children(f->child, dirty);
    }
}

static void
rw_frame_sync_root(RwFrame *root)
{
    if (root->dirty & RW_FRAME_SUBTREESYNC)
        root->ltm = root->matrix;
    rw_frame_sync_children(root->child, root->dirty);
    root->dirty &= ~(RW_FRAME_HIERARCHYSYNC | RW_FRAME_SUBTREESYNC);
}

RwFrame *
rw_frame_create(void)
{
    RwFrame *f = rw_malloc(sizeof(*f));

    if (!f)
        return NULL;

    memset(f, 0, sizeof(*f));
    rw_matrix_set_identity(&f->matrix);
    rw_matrix_set_identity(&f->ltm);
    f->root = f;
    rw_link_init(&f->in_dirty);
    return f;
}

void
rw_frame_destroy(RwFrame *f)
{
    RwFrame *child, *next;

    if (!f)
        return;

    if (f->parent)
        rw_frame_remove_child(f);
    if (f->dirty & RW_FRAME_HIERARCHYSYNC)
        rw_link_remove(&f->in_dirty);

    for (child = f->child; child; child = next) {
        next = child->next;
        child->parent = NULL;
        child->next = NULL;
        child->root = child;
        rw_frame_destroy(child);
    }

    rw_free(f);
}

void
rw_frame_add_child(RwFrame *parent, RwFrame *child)
{
    RwFrame *c;

    if (!parent || !child || parent == child)
        return;

    if (child->parent)
        rw_frame_remove_child(child);
    if (child->dirty & RW_FRAME_HIERARCHYSYNC) {
        rw_link_remove(&child->in_dirty);
        child->dirty &= ~RW_FRAME_HIERARCHYSYNC;
    }

    child->next = NULL;
    if (!parent->child) {
        parent->child = child;
    } else {
        for (c = parent->child; c->next; c = c->next)
            ;
        c->next = child;
    }

    child->parent = parent;
    rw_frame_set_root(child, parent->root ? parent->root : parent);
    rw_frame_mark_dirty(parent);
}

void
rw_frame_remove_child(RwFrame *child)
{
    RwFrame *parent, *c;

    if (!child || !child->parent)
        return;

    parent = child->parent;
    if (parent->child == child) {
        parent->child = child->next;
    } else {
        for (c = parent->child; c && c->next != child; c = c->next)
            ;
        if (c)
            c->next = child->next;
    }

    child->parent = NULL;
    child->next = NULL;
    rw_frame_set_root(child, child);
    rw_frame_mark_dirty(parent);
    rw_frame_mark_dirty(child);
}

RwMatrix *
rw_frame_get_ltm(RwFrame *f)
{
    RwFrame *root;

    if (!f)
        return NULL;

    root = f->root ? f->root : f;
    if (root->dirty & RW_FRAME_HIERARCHYSYNC) {
        rw_frame_sync_root(root);
        rw_link_remove(&root->in_dirty);
    }
    return &f->ltm;
}

void
rw_frame_set_matrix(RwFrame *f, const RwMatrix *m)
{
    if (!f || !m)
        return;
    f->matrix = *m;
    rw_frame_mark_dirty(f);
}

void
rw_frame_translate(RwFrame *f, RwV3d v, RwCombineOp op)
{
    if (!f)
        return;
    rw_matrix_translate(&f->matrix, v, op);
    rw_frame_mark_dirty(f);
}

void
rw_frame_rotate(RwFrame *f, RwV3d axis, float angle, RwCombineOp op)
{
    if (!f)
        return;
    rw_matrix_rotate(&f->matrix, axis, angle, op);
    rw_frame_mark_dirty(f);
}

void
rw_frame_scale(RwFrame *f, RwV3d s, RwCombineOp op)
{
    if (!f)
        return;
    rw_matrix_scale(&f->matrix, s, op);
    rw_frame_mark_dirty(f);
}

void
rw_frame_sync_dirty(void)
{
    RwLink *link;

    while (!rw_linklist_isempty(&rw_engine.frame_dirty_list)) {
        RwFrame *root;

        link = rw_engine.frame_dirty_list.link.next;
        rw_link_remove(link);
        root = RW_LINK_DATA(link, RwFrame, in_dirty);
        if (root->dirty & RW_FRAME_HIERARCHYSYNC)
            rw_frame_sync_root(root);
    }
}
