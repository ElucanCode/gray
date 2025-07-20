# gray
The name **gray** comes from "graph layout" as it is a library to compute vertex
positions for graph drawing / visualization. It does not come with its own
backend for rendering. Take a look at the examples directory for this demo using
[raylib](https://github.com/raysan5/raylib):

![Raylib demo](https://raw.githubusercontent.com/ElucanCode/gray/refs/heads/main/demo.gif)

Algorithms are taken from the wonderful [Handbook of Graph Drawing and Visualization](https://web.archive.org/web/20130815181243/http://cs.brown.edu/~rt/gdhandbook/).
Currently implemented algorithms:
- [x] Eades' spring algorithm
- [x] Fruchterman and Reingold's spring algorithm
- [ ] more to come :)

## Building
**gray** uses [nob.h](https://github.com/tsoding/nob.h) for building:
```shell
$ cc -o nob nob.c && ./nob
```
For building options run:
```shell
$ ./nob help
```
(The used compiler and compile flags can be specified by defining `CC` and `CF`
when compiling nob.)

## Usage
The simplest usage example looks something like this:
```c
#include "gray.h"

void use_gray(const size_t num_vertecies) {
    Graph g = { 0 };
    graph_init(&g, num_vertecies);
    graph_add_edge(&g, (Edge) { 0, 1, false });
    // Add more edges

    RenderContext ctx = render_ctx_create(&g);
    render_run(&ctx); // Runs by default for 100 iterations
    render_ctx_normalize(&ctx);
    // Do something with ctx.vertex_pos

    render_ctx_destroy(&ctx);
    graph_destroy(&g);
}
```

_Note_: If any of the functions or types supplied by **gray** create a collision
with one of your definitions simply define `GR_REQUIRE_PREFIX` before including
`gray.h`.
