#ifndef _GRAY_H_
#define _GRAY_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef GR_REQUIRE_PREFIX
#   define Vec2 gr_Vec2
#       define vec2 gr_vec2
#       define vec2zero gr_vec2zero
#       define vec2one gr_vec2one
#   define Graph gr_Graph
#       define Edge gr_Edge
#       define edge_u gr_edge_u
#       define edge_d gr_edge_d
#   define RenderMethodKind gr_RenderMethodKind
#   define RenderMethod gr_RenderMethod
#       define RM_Eades GR_RM_Eades
#       define RM_Fruchterman_and_Reingold GR_RM_Fruchterman_and_Reingold
#       define RM_SPRING_1 GR_RM_SPRING_1
#       define RM_SPRING_2 GR_RM_SPRING_2
#   define RenderContext gr_RenderContext
#   define graph_init gr_graph_init
#   define graph_destroy gr_graph_destroy
#   define graph_add_edge gr_graph_add_edge
#   define graph_add_edges gr_graph_add_edges

#   define render_ctx_create_ex gr_render_ctx_create_ex
#   define render_ctx_create gr_render_ctx_create
#   define render_ctx_destroy gr_render_ctx_destroy
#   define render_ctx_normalize_ex gr_render_ctx_normalize_ex
#   define render_ctx_normalize gr_render_ctx_normalize
#   define render_step gr_render_step
#   define render_step_for gr_render_step_for
#   define render_step_until gr_render_step_until
#   define render_run gr_render_run
#endif // GR_REQUIRE_PREFIX

#ifndef GR_DEFAULT_METHOD
#   define GR_DEFAULT_METHOD GR_RM_SPRING_1
#endif // GR_DEFAULT_METHOD

#ifndef GR_DEFAULT_ITERATIONS
#   define GR_DEFAULT_ITERATIONS 100
#endif // GR_DEFAULT_ITERATIONS

typedef struct {
    float x;
    float y;
} gr_Vec2;

#define gr_vec2(x_, y_) ((gr_Vec2) { .x = (x_), .y = (y_) })
#define gr_vec2zero()   gr_vec2(0, 0)
#define gr_vec2one()    gr_vec2(1, 1)

#define gr_randf(min, max)\
    ((min) + (float)rand() / (float)(RAND_MAX) * ((max) - (min)))
#define gr_lerpf(v, min1, max1, min2, max2)\
    (min2 + ((v - min1) / (max1 - min1)) * (max2 - min2))

typedef struct {
    size_t n_verticies;
    bool **edges;
} gr_Graph;

// Only used in the API for adding edges.
typedef struct {
    size_t start;
    size_t end;
    bool directed;
} gr_Edge;

#define gr_edge_u(a, b) ((gr_Edge) { .start = (a), .end = (b), .directed = false })
#define gr_edge_d(a, b) ((gr_Edge) { .start = (a), .end = (b), .directed = true })

enum gr_RenderMethodKind {
    GR_RM_Eades,
    GR_RM_Fruchterman_and_Reingold,
};

#define GR_RM_SPRING_1 GR_RM_Eades
#define GR_RM_SPRING_2 GR_RM_Fruchterman_and_Reingold

#define GR_DEFAULT_GRAV_CEN gr_vec2zero()

#define GR_DEFAULT_EADES_C1       2
#define GR_DEFAULT_EADES_C2       1
#define GR_DEFAULT_EADES_C3       1
#define GR_DEFAULT_EADES_C4       0.1

struct gr_RenderMethodEades {
    const enum gr_RenderMethodKind kind;
    // c1 and c2 are used to calculate the force of a spring
    float c1;
    float c2;
    // used to calculate the repellent force between unconnected verticies
    float c3;
    // multiplied with the force to move the vertex
    float c4;
    // Center of gravity
    gr_Vec2 grav_cen;
};

#define GR_DEFAULT_FRUCHTGOLD_C                     0.1f
#define GR_DEFAULT_FRUCHTGOLD_AREA                  100
#define GR_DEFAULT_FRUCHTGOLD_TEMP                  0.1f
#define GR_DEFAULT_FRUCHTGOLD_DECAY(temp, iters)    ((temp) / (iters))

// Fruchterman and Reingold implementation.
// To disable temperature set `cur_temp = 1` and `decay = 0`
struct gr_RenderMethodFruchtgold {
    const enum gr_RenderMethodKind kind;
    float c;
    float area;
    float cur_temp;
    float decay;
    // Center of gravity
    gr_Vec2 grav_cen;
};

typedef union {
    enum gr_RenderMethodKind kind;
    struct gr_RenderMethodEades eades;
    struct gr_RenderMethodFruchtgold fruchtgold;
} gr_RenderMethod;

typedef struct {
    const gr_Graph *graph;
    gr_RenderMethod method;
    // Just stored weather `initialize = true` was passed to `gr_render_ctx_create_ex`.
    // Only used in `gr_render_ctx_destroy`
    bool positions_initialized;

    gr_Vec2 *vertex_pos;
    size_t cur_iteration;
} gr_RenderContext;

// Returns only false if the internal structures could not be allocated
bool gr_graph_init(gr_Graph *graph, const size_t n_verticies);
void gr_graph_destroy(gr_Graph *graph);
bool gr_graph_add_edge(gr_Graph *graph, const gr_Edge edge);
// Returns: `0` on success (already existing edges are ignored and won't lead to failure)
// and `n` when the n-th edge could not be added, following edges are also not added yet.
size_t gr_graph_add_edges(gr_Graph *graph, const size_t n_edges, const gr_Edge *edges);

// Creates the default render method for the passed kind. Values in this method
// can be tweaked.
gr_RenderMethod gr_method_create(const enum gr_RenderMethodKind kind);
#define gr_method_default() (gr_method_create(GR_DEFAULT_METHOD))

// If `init_positions` is `false` you have to initialize it yourself, it won't even
// be allocated. Otherwise `rand` is used to randomly initialize the position vectors.
gr_RenderContext gr_render_ctx_create_ex(const gr_Graph *graph,
                                         const gr_RenderMethod method,
                                         const bool init_positions);
#define gr_render_ctx_create(graph) \
    (gr_render_ctx_create_ex((graph), gr_method_default(), true))
void gr_render_ctx_destroy(gr_RenderContext *ctx);
// Lerps all positions into the rectangle described by `mins` and `maxs`
void gr_render_ctx_normalize_ex(gr_RenderContext *ctx, const gr_Vec2 mins,
                                const gr_Vec2 maxs);
#define gr_render_ctx_normalize(ctx) \
    (gr_render_ctx_normalize_ex((ctx), gr_vec2(0.05, 0.05), gr_vec2(0.95, 0.95)))
void gr_render_step(gr_RenderContext *ctx);
void gr_render_step_for(gr_RenderContext *ctx, const size_t iterations);
void gr_render_step_until(gr_RenderContext *ctx, const size_t target_iterations);
#define gr_render_run(ctx) gr_render_step_until((ctx), GR_DEFAULT_ITERATIONS)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _GRAY_H_
