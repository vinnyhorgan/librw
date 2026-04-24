#include "rw.h"
#include <glad/gles2.h>

#include <math.h>
#include <string.h>

static int
rw_link_is_linked(RwLink *link)
{
    return link->next != link;
}

static void
rw_scene_sphere_transform(RwSphere *out, const RwSphere *in, const RwMatrix *m)
{
    RwV3d c = in->center;

    out->center.x = c.x*m->right.x + c.y*m->up.x + c.z*m->at.x + m->pos.x;
    out->center.y = c.x*m->right.y + c.y*m->up.y + c.z*m->at.y + m->pos.y;
    out->center.z = c.x*m->right.z + c.y*m->up.z + c.z*m->at.z + m->pos.z;
    out->radius = in->radius;
}

static void
rw_camera_update_projection(RwCamera *c)
{
    memset(&c->dev_proj, 0, sizeof(c->dev_proj));
    c->dev_proj.right.x = 1.0f / c->view_window.x;
    c->dev_proj.up.y = 1.0f / c->view_window.y;
    c->dev_proj.at.z = c->far_plane / (c->far_plane - c->near_plane);
    c->dev_proj.atw = 1.0f;
    c->dev_proj.pos.z = -(c->near_plane * c->far_plane) / (c->far_plane - c->near_plane);
}

static void
rw_camera_set_frustum_plane(RwCamera *c, int index, RwV3d normal, RwV3d point)
{
    normal = rw_v3d_normalize(normal);
    c->frustum[index].normal = normal;
    c->frustum[index].dist = -rw_v3d_dot(normal, point);
    c->frustum[index].cx = normal.x >= 0.0f;
    c->frustum[index].cy = normal.y >= 0.0f;
    c->frustum[index].cz = normal.z >= 0.0f;
}

static RwV3d
rw_camera_vector_to_world(const RwMatrix *ltm, RwV3d v)
{
    RwV3d out;

    out.x = v.x*ltm->right.x + v.y*ltm->up.x + v.z*ltm->at.x;
    out.y = v.x*ltm->right.y + v.y*ltm->up.y + v.z*ltm->at.y;
    out.z = v.x*ltm->right.z + v.y*ltm->up.z + v.z*ltm->at.z;
    return out;
}

static void
rw_camera_update_frustum(RwCamera *c)
{
    RwMatrix ltm;
    RwV3d near_point, far_point;

    if (c->frame)
        ltm = *rw_frame_get_ltm(c->frame);
    else
        rw_matrix_set_identity(&ltm);

    near_point = rw_v3d_add(ltm.pos, rw_v3d_scale(ltm.at, c->near_plane));
    far_point = rw_v3d_add(ltm.pos, rw_v3d_scale(ltm.at, c->far_plane));

    rw_camera_set_frustum_plane(c, 0, ltm.at, near_point);
    rw_camera_set_frustum_plane(c, 1, rw_v3d_scale(ltm.at, -1.0f), far_point);
    rw_camera_set_frustum_plane(c, 2, rw_camera_vector_to_world(&ltm, (RwV3d){1.0f, 0.0f, c->view_window.x}), ltm.pos);
    rw_camera_set_frustum_plane(c, 3, rw_camera_vector_to_world(&ltm, (RwV3d){-1.0f, 0.0f, c->view_window.x}), ltm.pos);
    rw_camera_set_frustum_plane(c, 4, rw_camera_vector_to_world(&ltm, (RwV3d){0.0f, 1.0f, c->view_window.y}), ltm.pos);
    rw_camera_set_frustum_plane(c, 5, rw_camera_vector_to_world(&ltm, (RwV3d){0.0f, -1.0f, c->view_window.y}), ltm.pos);
}

RwAtomic *
rw_atomic_create(void)
{
    RwAtomic *a = rw_malloc(sizeof(*a));

    if (!a)
        return NULL;

    memset(a, 0, sizeof(*a));
    rw_link_init(&a->in_clump);
    return a;
}

void
rw_atomic_destroy(RwAtomic *a)
{
    if (!a)
        return;

    if (a->clump)
        rw_clump_remove_atomic(a->clump, a);
    if (a->geometry)
        rw_geometry_destroy(a->geometry);
    rw_free(a);
}

void
rw_atomic_set_geometry(RwAtomic *a, RwGeometry *geo)
{
    if (!a || a->geometry == geo)
        return;

    if (geo)
        geo->ref_count++;
    if (a->geometry)
        rw_geometry_destroy(a->geometry);
    a->geometry = geo;
    if (geo)
        a->bounding_sphere = geo->morph_target.bounding_sphere;
}

void
rw_atomic_set_frame(RwAtomic *a, RwFrame *f)
{
    if (a)
        a->frame = f;
}

void
rw_atomic_set_pipeline(RwAtomic *a, RwObjPipeline *p)
{
    if (a)
        a->pipeline = p;
}

void
rw_atomic_set_render_cb(RwAtomic *a, void (*cb)(RwAtomic *))
{
    if (a)
        a->render_cb = cb;
}

void
rw_atomic_render(RwAtomic *a)
{
    if (!a)
        return;

    if (a->render_cb)
        a->render_cb(a);
    if (a->pipeline && a->pipeline->render)
        a->pipeline->render(a);
}

RwClump *
rw_clump_create(void)
{
    RwClump *c = rw_malloc(sizeof(*c));

    if (!c)
        return NULL;

    memset(c, 0, sizeof(*c));
    rw_linklist_init(&c->atomics);
    rw_linklist_init(&c->lights);
    rw_linklist_init(&c->cameras);
    rw_link_init(&c->in_world);
    c->frame = rw_frame_create();
    if (!c->frame) {
        rw_free(c);
        return NULL;
    }
    return c;
}

void
rw_clump_destroy(RwClump *c)
{
    RwLink *link;

    if (!c)
        return;

    if (c->world)
        rw_world_remove_clump(c->world, c);

    while (!rw_linklist_isempty(&c->atomics)) {
        RwAtomic *a;

        link = c->atomics.link.next;
        a = RW_LINK_DATA(link, RwAtomic, in_clump);
        rw_atomic_destroy(a);
    }

    while (!rw_linklist_isempty(&c->lights)) {
        RwLight *l;

        link = c->lights.link.next;
        l = RW_LINK_DATA(link, RwLight, in_clump);
        rw_link_remove(link);
        l->clump = NULL;
    }

    rw_frame_destroy(c->frame);
    rw_free(c);
}

RwFrame *
rw_clump_get_frame(RwClump *c)
{
    return c ? c->frame : NULL;
}

void
rw_clump_set_frame(RwClump *c, RwFrame *f)
{
    if (!c || c->frame == f)
        return;

    rw_frame_destroy(c->frame);
    c->frame = f;
}

void
rw_clump_add_atomic(RwClump *c, RwAtomic *a)
{
    if (!c || !a)
        return;

    if (a->clump)
        rw_clump_remove_atomic(a->clump, a);
    a->clump = c;
    a->world = c->world;
    rw_linklist_append(&c->atomics, &a->in_clump);
}

void
rw_clump_remove_atomic(RwClump *c, RwAtomic *a)
{
    if (!c || !a || a->clump != c)
        return;

    rw_link_remove(&a->in_clump);
    a->clump = NULL;
    a->world = NULL;
}

void
rw_clump_add_light(RwClump *c, RwLight *l)
{
    if (!c || !l)
        return;

    if (l->clump)
        rw_link_remove(&l->in_clump);
    l->clump = c;
    rw_linklist_append(&c->lights, &l->in_clump);
}

void
rw_clump_render(RwClump *c)
{
    RwLink *link;

    if (!c)
        return;

    RW_FORLIST(link, c->atomics) {
        RwAtomic *a = RW_LINK_DATA(link, RwAtomic, in_clump);
        rw_atomic_render(a);
    }
}

RwWorld *
rw_world_create(void)
{
    RwWorld *w = rw_malloc(sizeof(*w));

    if (!w)
        return NULL;

    memset(w, 0, sizeof(*w));
    rw_linklist_init(&w->local_lights);
    rw_linklist_init(&w->global_lights);
    rw_linklist_init(&w->clumps);
    return w;
}

void
rw_world_destroy(RwWorld *w)
{
    if (!w)
        return;

    while (!rw_linklist_isempty(&w->clumps)) {
        RwLink *link = w->clumps.link.next;
        RwClump *c = RW_LINK_DATA(link, RwClump, in_world);
        rw_world_remove_clump(w, c);
    }
    while (!rw_linklist_isempty(&w->global_lights)) {
        RwLink *link = w->global_lights.link.next;
        RwLight *l = RW_LINK_DATA(link, RwLight, in_world);
        rw_world_remove_light(w, l);
    }
    while (!rw_linklist_isempty(&w->local_lights)) {
        RwLink *link = w->local_lights.link.next;
        RwLight *l = RW_LINK_DATA(link, RwLight, in_world);
        rw_world_remove_light(w, l);
    }
    rw_free(w);
}

void
rw_world_add_clump(RwWorld *w, RwClump *c)
{
    RwLink *link;

    if (!w || !c)
        return;

    if (c->world)
        rw_world_remove_clump(c->world, c);
    c->world = w;
    rw_linklist_append(&w->clumps, &c->in_world);
    RW_FORLIST(link, c->atomics) {
        RwAtomic *a = RW_LINK_DATA(link, RwAtomic, in_clump);
        a->world = w;
    }
}

void
rw_world_remove_clump(RwWorld *w, RwClump *c)
{
    RwLink *link;

    if (!w || !c || c->world != w)
        return;

    rw_link_remove(&c->in_world);
    c->world = NULL;
    RW_FORLIST(link, c->atomics) {
        RwAtomic *a = RW_LINK_DATA(link, RwAtomic, in_clump);
        a->world = NULL;
    }
}

void
rw_world_add_light(RwWorld *w, RwLight *l)
{
    RwLinkList *list;

    if (!w || !l)
        return;

    if (l->world)
        rw_world_remove_light(l->world, l);
    l->world = w;
    list = (l->type == RW_LIGHT_POINT || l->type == RW_LIGHT_SPOT) ? &w->local_lights : &w->global_lights;
    rw_linklist_append(list, &l->in_world);
}

void
rw_world_remove_light(RwWorld *w, RwLight *l)
{
    if (!w || !l || l->world != w)
        return;

    rw_link_remove(&l->in_world);
    l->world = NULL;
}

void
rw_world_render(RwWorld *w)
{
    RwLink *link;

    if (!w)
        return;
    rw_engine.current_world = w;
    RW_FORLIST(link, w->clumps) {
        RwClump *c = RW_LINK_DATA(link, RwClump, in_world);
        rw_clump_render(c);
    }
}

void
rw_world_enumerate_lights(RwWorld *w, RwAtomic *a, RwWorldLights *out)
{
    RwLink *link;
    RwSphere sphere;

    if (!out)
        return;
    memset(out, 0, sizeof(*out));
    if (!w)
        return;

    if (a) {
        sphere = a->bounding_sphere;
        if (a->frame)
            rw_scene_sphere_transform(&sphere, &a->bounding_sphere, rw_frame_get_ltm(a->frame));
    }

    RW_FORLIST(link, w->global_lights) {
        RwLight *l = RW_LINK_DATA(link, RwLight, in_world);
        if (l->type == RW_LIGHT_AMBIENT) {
            out->num_ambients++;
            out->ambient.r += l->color.r;
            out->ambient.g += l->color.g;
            out->ambient.b += l->color.b;
            out->ambient.a += l->color.a;
        } else if (l->type == RW_LIGHT_DIRECTIONAL && out->num_directionals < 8) {
            out->directionals[out->num_directionals++] = l;
        }
    }

    RW_FORLIST(link, w->local_lights) {
        RwLight *l = RW_LINK_DATA(link, RwLight, in_world);

        if (out->num_locals >= 8)
            break;
        if (!a || !l->frame) {
            out->locals[out->num_locals++] = l;
        } else {
            RwV3d d = rw_v3d_sub(rw_frame_get_ltm(l->frame)->pos, sphere.center);
            if (rw_v3d_dot(d, d) <= (l->radius + sphere.radius) * (l->radius + sphere.radius))
                out->locals[out->num_locals++] = l;
        }
    }
}

RwCamera *
rw_camera_create(void)
{
    RwCamera *c = rw_malloc(sizeof(*c));

    if (!c)
        return NULL;

    memset(c, 0, sizeof(*c));
    c->view_window = (RwV2d){1.0f, 1.0f};
    c->near_plane = 0.1f;
    c->far_plane = 100.0f;
    c->fog_plane = 100.0f;
    rw_matrix_set_identity(&c->view_matrix);
    rw_matrix_to_raw(&c->dev_view, &c->view_matrix);
    rw_camera_update_projection(c);
    rw_camera_update_frustum(c);
    return c;
}

void
rw_camera_destroy(RwCamera *c)
{
    if (!c)
        return;
    if (rw_engine.current_camera == c)
        rw_engine.current_camera = NULL;
    rw_free(c);
}

void
rw_camera_set_frame(RwCamera *c, RwFrame *f)
{
    if (c)
        c->frame = f;
}

void
rw_camera_begin_update(RwCamera *c)
{
    if (!c)
        return;

    if (c->frame)
        rw_matrix_invert(&c->view_matrix, rw_frame_get_ltm(c->frame));
    else
        rw_matrix_set_identity(&c->view_matrix);
    rw_matrix_to_raw(&c->dev_view, &c->view_matrix);
    rw_camera_update_projection(c);
    rw_camera_update_frustum(c);
    rw_engine.current_camera = c;
}

void
rw_camera_end_update(RwCamera *c)
{
    (void)c;
}

void
rw_camera_clear(RwCamera *c, uint32_t flags)
{
    GLbitfield mask = 0;

    (void)c;
    if (flags & RW_CAMERA_CLEAR_IMAGE)
        mask |= GL_COLOR_BUFFER_BIT;
    if (flags & RW_CAMERA_CLEAR_Z)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (flags & RW_CAMERA_CLEAR_STENCIL)
        mask |= GL_STENCIL_BUFFER_BIT;
    if (mask)
        glClear(mask);
}

void
rw_camera_set_fov(RwCamera *c, float fov)
{
    float t;

    if (!c || fov <= 0.0f)
        return;
    t = tanf(fov * 0.5f * (3.14159265358979323846f / 180.0f));
    c->view_window.x = t;
    c->view_window.y = t;
    rw_camera_update_projection(c);
}

void
rw_camera_set_view_window(RwCamera *c, float vw)
{
    if (!c || vw <= 0.0f)
        return;
    c->view_window.x = vw;
    c->view_window.y = vw;
    rw_camera_update_projection(c);
}

void
rw_camera_set_near_far(RwCamera *c, float near, float far)
{
    if (!c || near <= 0.0f || far <= near)
        return;
    c->near_plane = near;
    c->far_plane = far;
    rw_camera_update_projection(c);
}

int
rw_camera_frustum_test_sphere(RwCamera *c, const RwSphere *s)
{
    int i;

    if (!c || !s)
        return 0;

    for (i = 0; i < 6; i++) {
        float d = rw_v3d_dot(c->frustum[i].normal, s->center) + c->frustum[i].dist;
        if (d < -s->radius)
            return 0;
    }
    return 1;
}

RwLight *
rw_light_create(RwLightType type)
{
    RwLight *l = rw_malloc(sizeof(*l));

    if (!l)
        return NULL;

    memset(l, 0, sizeof(*l));
    l->type = (uint8_t)type;
    l->radius = 1.0f;
    l->color = (RwRGBAf){1.0f, 1.0f, 1.0f, 1.0f};
    rw_link_init(&l->in_world);
    rw_link_init(&l->in_clump);
    return l;
}

void
rw_light_destroy(RwLight *l)
{
    if (!l)
        return;
    if (l->world)
        rw_world_remove_light(l->world, l);
    if (l->clump && rw_link_is_linked(&l->in_clump))
        rw_link_remove(&l->in_clump);
    rw_free(l);
}

void
rw_light_set_color(RwLight *l, RwRGBAf color)
{
    if (l)
        l->color = color;
}

void
rw_light_set_radius(RwLight *l, float radius)
{
    if (l)
        l->radius = radius;
}

void
rw_light_set_frame(RwLight *l, RwFrame *f)
{
    if (l)
        l->frame = f;
}
