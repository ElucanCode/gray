#include "gray.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {            // only here so it is also used in the created single header file
#endif // __cplusplus

#define gr_abs(n)  ((n) < 0 ? -(n) : (n))
#define gr_sign(n) ((n) < 0 ? -1 : 1)
#define gr_sq(n)   ((n) * (n))
#define gr_not0f(n) ((n) == 0 ? 0.000000000001 : (n))

bool gr_graph_init(gr_Graph *graph, const size_t n_verticies)
{
    assert(graph != NULL);
    assert(n_verticies > 0);
    graph->edges = NULL;

    bool *edges_raw = calloc(n_verticies * n_verticies, sizeof(bool));
    if (edges_raw == NULL) {
        return false;
    }
    bool **edges = calloc(n_verticies, sizeof(bool*));
    if (edges == NULL) {
        free(edges_raw);
        return false;
    }
    for (size_t i = 0; i < n_verticies; i += 1) {
        edges[i] = edges_raw + (n_verticies * i);
    }

    graph->n_verticies = n_verticies;
    graph->edges = edges;
    return true;
}

void gr_graph_destroy(gr_Graph *graph)
{
    assert(graph != NULL);
    assert(graph->edges != NULL);
    free(*graph->edges);
    free(graph->edges);
    graph->edges = NULL;
    return;
}

bool gr_graph_add_edge(gr_Graph *graph, const gr_Edge edge)
{
    assert(graph != NULL);
    if (edge.start >= graph->n_verticies || edge.end >= graph->n_verticies) {
        return false;
    }
    graph->edges[edge.start][edge.end] = true;
    if (!edge.directed) {
        graph->edges[edge.end][edge.start] = true;
    }
    return true;
}

size_t gr_graph_add_edges(gr_Graph *graph, const size_t n_edges, const gr_Edge *edges)
{
    for (size_t i = 0; i < n_edges; i += 1) {
        if (!gr_graph_add_edge(graph, edges[i])) {
            return i;
        }
    }
    return 0;
}

gr_RenderMethod gr_method_create(const enum gr_RenderMethodKind kind)
{
    switch (kind) {
        case GR_RM_Eades:
            return (gr_RenderMethod) {
                .eades = (struct gr_RenderMethodEades) {
                    .kind = GR_RM_Eades,
                    .c1 = GR_DEFAULT_EADES_C1,
                    .c2 = GR_DEFAULT_EADES_C2,
                    .c3 = GR_DEFAULT_EADES_C3,
                    .c4 = GR_DEFAULT_EADES_C4,
                    .grav_cen = GR_DEFAULT_GRAV_CEN,
                },
            };
        case GR_RM_Fruchterman_and_Reingold:
            return (gr_RenderMethod) {
                .fruchtgold = (struct gr_RenderMethodFruchtgold) {
                    .kind = GR_RM_Fruchterman_and_Reingold,
                    .c = GR_DEFAULT_FRUCHTGOLD_C,
                    .area = GR_DEFAULT_FRUCHTGOLD_AREA,
                    .cur_temp = GR_DEFAULT_FRUCHTGOLD_TEMP,
                    .decay = GR_DEFAULT_FRUCHTGOLD_DECAY(
                                GR_DEFAULT_FRUCHTGOLD_TEMP,
                                GR_DEFAULT_ITERATIONS),
                    .grav_cen = GR_DEFAULT_GRAV_CEN,
                },
            };
    default:
        fprintf(stderr, "Unreachable render method kind: %d\n", kind);
        abort();
    }
}

gr_RenderContext gr_render_ctx_create_ex(const gr_Graph *graph,
                                         const gr_RenderMethod method,
                                         const bool init_positions)
{
    assert(graph != NULL);

    gr_RenderContext ctx = (gr_RenderContext) {
        .graph = graph,
        .method = method,
        .positions_initialized = init_positions,
        .vertex_pos = NULL,
        .cur_iteration = 0,
    };

    if (init_positions) {
        ctx.vertex_pos = calloc(graph->n_verticies, sizeof(gr_Vec2));
        for (size_t i = 0; i < graph->n_verticies; i += 1) {
            ctx.vertex_pos[i] = (gr_Vec2) {
                .x = gr_randf(0, 1),
                .y = gr_randf(0, 1),
            };
        }
    }

    return ctx;
}

void gr_render_ctx_destroy(gr_RenderContext *ctx)
{
    assert(ctx != NULL);
    if (ctx->positions_initialized && ctx->vertex_pos != NULL) {
        free(ctx->vertex_pos);
        ctx->vertex_pos = NULL;
    }
}

void gr_render_ctx_normalize_ex(gr_RenderContext *ctx, const gr_Vec2 mins,
                                const gr_Vec2 maxs)
{
    assert(ctx != NULL);
    assert(mins.x < maxs.x && mins.y < maxs.y);

    gr_Vec2 min = { FLT_MAX, FLT_MAX };
    gr_Vec2 max = { FLT_MIN, FLT_MIN };

    for (size_t i = 0; i < ctx->graph->n_verticies; i += 1) {
        if (ctx->vertex_pos[i].x < min.x) { min.x = ctx->vertex_pos[i].x; }
        if (ctx->vertex_pos[i].y < min.y) { min.y = ctx->vertex_pos[i].y; }
        if (ctx->vertex_pos[i].x > max.x) { max.x = ctx->vertex_pos[i].x; }
        if (ctx->vertex_pos[i].y > max.y) { max.y = ctx->vertex_pos[i].y; }
    }

    for (size_t i = 0; i < ctx->graph->n_verticies; i += 1) {
        ctx->vertex_pos[i].x = gr_lerpf(ctx->vertex_pos[i].x, min.x, max.x, mins.x, maxs.x);
        ctx->vertex_pos[i].y = gr_lerpf(ctx->vertex_pos[i].y, min.y, max.y, mins.y, maxs.y);
    }
}

static void gr_step_method_eades(gr_RenderContext *ctx,
                                 struct gr_RenderMethodEades *eades)
{
    for (size_t n = 0; n < ctx->graph->n_verticies; n += 1) {
        gr_Vec2 force = gr_vec2zero();
        for (size_t i = 0; i < ctx->graph->n_verticies; i += 1) {
            // if `n == 1` we are already here, so might as well use it for
            // gravity calculations
            const gr_Vec2 other = (n == i) ? eades->grav_cen : ctx->vertex_pos[i];
            const float d_x     = other.x - ctx->vertex_pos[n].x;
            const float d_y     = other.y - ctx->vertex_pos[n].y;
            const float d       = gr_not0f(sqrtf(gr_sq(d_x) + gr_sq(d_y)));

            if (ctx->graph->edges[n][i] || n == i) {
                force.x += (eades->c1 * logf(d / eades->c2)) * (d_x / d);
                force.y += (eades->c1 * logf(d / eades->c2)) * (d_y / d);
            } else if (d != 0) {
                force.x -= eades->c3 / gr_sq(d) * (d_x / d);
                force.y -= eades->c3 / gr_sq(d) * (d_y / d);
            }
        }
        ctx->vertex_pos[n].x += eades->c4 * force.x;
        ctx->vertex_pos[n].y += eades->c4 * force.y;
    }
}

static void gr_step_method_fruchtman_and_reingold(gr_RenderContext *ctx,
                                                  struct gr_RenderMethodFruchtgold *fag)
{
    const float k = fag->c * sqrtf(fag->area / ctx->graph->n_verticies);
    for (size_t n = 0; n < ctx->graph->n_verticies; n += 1) {
        gr_Vec2 force = gr_vec2zero();
        for (size_t i = 0; i < ctx->graph->n_verticies; i += 1) {
            // if `n == 1` we are already here, so might as well use it for
            // gravity calculations
            const gr_Vec2 other = (n == i) ? fag->grav_cen : ctx->vertex_pos[i];
            const float d_x     = other.x - ctx->vertex_pos[n].x;
            const float d_y     = other.y - ctx->vertex_pos[n].y;
            const float d_sq    = gr_sq(d_x) + gr_sq(d_y);
            const float d       = gr_not0f(sqrtf(d_sq));

            if (ctx->graph->edges[n][i] || n == i) {
                force.x += (d_sq / k) * (d_x / d);
                force.y += (d_sq / k) * (d_y / d);
            } else {
                force.x += (-gr_sq(k) / d) * (d_x / d);
                force.y += (-gr_sq(k) / d) * (d_y / d);
            }
        }
        ctx->vertex_pos[n].x += fag->cur_temp * force.x;
        ctx->vertex_pos[n].y += fag->cur_temp * force.y;
    }
    fag->cur_temp -= fag->decay;
    if (fag->cur_temp < 0) {
        fag->cur_temp = 0;
    }
}

static void gr_render_step_unchecked(gr_RenderContext *ctx)
{
    switch(ctx->method.kind) {
        case GR_RM_Eades:
            gr_step_method_eades(ctx, &ctx->method.eades);
            break;
        case GR_RM_Fruchterman_and_Reingold:
            gr_step_method_fruchtman_and_reingold(ctx, &ctx->method.fruchtgold);
            break;
        default:
            fprintf(stderr, "Unreachable render method kind: %d\n", ctx->method.kind);
            abort();
    }
    ctx->cur_iteration += 1;
}

void gr_render_step(gr_RenderContext *ctx)
{
    assert(ctx != NULL);
    assert(ctx->graph != NULL);
    gr_render_step_unchecked(ctx);
}

void gr_render_step_for(gr_RenderContext *ctx, const size_t iterations)
{
    assert(ctx != NULL);
    assert(ctx->graph != NULL);
    for (size_t i = 0; i < iterations; i += 1) {
        gr_render_step_unchecked(ctx);
    }
}

void gr_render_step_until(gr_RenderContext *ctx, const size_t target_iterations)
{
    assert(ctx != NULL);
    assert(ctx->graph != NULL);
    while (ctx->cur_iteration < target_iterations) {
        gr_render_step_unchecked(ctx);
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
