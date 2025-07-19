# gray
The name gray comes from "graph layout" as it is a library to compute vertex
positions for graph drawing / visualization. It does not come with its own
backend for rendering. Take a look at the examples folder for simple rendering
examples.

Algorithms are taken from the wonderful [Handbook of Graph Drawing and Visualization](https://web.archive.org/web/20130815181243/http://cs.brown.edu/~rt/gdhandbook/).
Currently implemented algorithms:
- [x] Eades' spring algorithm
- [x] Fruchterman and Reingold's spring algorithm
- [ ] more to come :)

## Building
```shell
$ cc -o nob nob.c
$ ./nob
```

By default nob will use `cc` to compile everything, if want to use a different
compiler build nob like this:
```shell
$ cc -o nob -DCC='"clang"' nob.c
```
Similarly you can pass compiler and linker flags using `CF` and `LF`
respectively. (These defines might be discarded when you change nob.c and do not
manually recompile it.)

For building options run:
```shell
$ ./nob help
```

## Usage
_Note_: If any of the functions or types supplied by **gray** create a collision
with one of your definitions simply define `GR_REQUIRE_PREFIX` before including
`gray.h`.

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
