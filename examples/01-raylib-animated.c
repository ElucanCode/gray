#include <string.h>
#include <raylib.h>
#include <time.h>

#include "../gray.h"

#define WIN_WIDTH (640)
#define WIN_HEIGHT (480)
#define WIN_TITLE "gray Raylib animated example"

// Some random positions will lead to a lot of necessary iterations, others are
// done after a few dozens of iterations.
#define TARGET_ITERATION 200
#define TIMER 0.1
#define BG LIGHTGRAY

#define VERTEX_RAD 10
#define VERTEX_COLOR BLACK
#define EDGE_COLOR DARKGRAY

#define TEXT_COLOR RED
#define TEXT_SIZE 20
#define TXT_ARGS(row) 10, (10 + (TEXT_SIZE + 5) * row), TEXT_SIZE, TEXT_COLOR

static const Edge edges[] = {
    {  0,  1, false }, {  0,  2, false }, {  0, 14, false }, {  1,  3, false },
    {  1,  4, false }, {  2,  5, false }, {  2,  6, false }, {  3,  7, false },
    {  4,  7, false }, {  5,  8, false }, {  6,  7, false }, {  6,  9, false },
    {  7, 10, false }, {  8, 11, false }, {  9, 12, false }, { 10, 13, false },
    { 11, 14, false },
};

int main() {
    InitWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    time_t t = time(0);
    TraceLog(LOG_INFO, "Seed: %ld\n", t);
    srand(t); // Used when initializing the render context

    Graph g = { 0 };
    graph_init(&g, 15);
    for (size_t i = 0; i < sizeof(edges) / sizeof(Edge); i += 1) {
        graph_add_edge(&g, edges[i]);
    }
    RenderContext ctx = render_ctx_create(&g);
    Vec2 *backup = malloc(g.n_verticies * sizeof(Vec2));
    memcpy(backup, ctx.vertex_pos, g.n_verticies * sizeof(Vec2));

    bool show_id = false;
    bool step = false;
    bool run_till_end = false;
    float timer = 0;

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
    
        if (ctx.cur_iteration < TARGET_ITERATION) {
            if (IsKeyPressed(KEY_SPACE)) {
                run_till_end = true;
            }
            step |= IsKeyPressed(KEY_S) || run_till_end;

            if (step) {
                timer += dt;
                for (bool first = true; timer > TIMER; timer -= TIMER) {
                    if (first) {
                        memcpy(ctx.vertex_pos, backup, g.n_verticies * sizeof(Vec2));
                        first = false;
                    }
                    render_step(&ctx);
                    step = false;
                }

                if (!step) {
                    memcpy(backup, ctx.vertex_pos, g.n_verticies * sizeof(Vec2));
                    render_ctx_normalize(&ctx);
                }
            }
        }
        if (IsKeyPressed(KEY_N)) {
            show_id = !show_id;
        }

        BeginDrawing();
            ClearBackground(BG);

            const int w = GetRenderWidth();
            const int h = GetRenderHeight();

            // draw edges
            for (size_t n = 0; n < ctx.graph->n_verticies; n += 1) {
                for (size_t i = 0; i < ctx.graph->n_verticies; i += 1) {
                    if (ctx.graph->edges[n][i]) {
                        DrawLine(ctx.vertex_pos[n].x * w, ctx.vertex_pos[n].y * h,
                                 ctx.vertex_pos[i].x * w, ctx.vertex_pos[i].y * h,
                                 EDGE_COLOR);
                    }
                }
            }

            // draw verticies
            for (size_t i = 0; i < ctx.graph->n_verticies; i += 1) {
                Vector2 pos = {
                    .x = ctx.vertex_pos[i].x * w,
                    .y = ctx.vertex_pos[i].y * h,
                };
                if (show_id) {
                    DrawCircleV(pos, VERTEX_RAD + 2, VERTEX_COLOR);
                    DrawCircleV(pos, VERTEX_RAD, BG);
                    DrawText(TextFormat("%zu", i),
                             pos.x - VERTEX_RAD / 3.0, pos.y - VERTEX_RAD / 3.0,
                             VERTEX_RAD, VERTEX_COLOR);
                } else {
                    DrawCircleV(pos, VERTEX_RAD, VERTEX_COLOR);
                }
            }

            if (ctx.cur_iteration < TARGET_ITERATION) {
                DrawText(TextFormat("Itation %zu of %zu", ctx.cur_iteration,
                                    TARGET_ITERATION), TXT_ARGS(0));
                DrawText(TextFormat("<N>: to show vertex ids (%s)",
                                    show_id ? "true" : "false"), TXT_ARGS(1));
                if (!run_till_end) {
                    DrawText("<Space>: Run the simulation", TXT_ARGS(2));
                    DrawText("<S>: A single simulation step", TXT_ARGS(3));
                }
            } else {
                DrawText("Done", TXT_ARGS(0));
            }
        EndDrawing();
    }

    free(backup);
    render_ctx_destroy(&ctx);
    graph_destroy(&g);

    CloseWindow();
    return 0;
}
