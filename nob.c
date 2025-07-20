#define NOB_EXPERIMENTAL_DELETE_OLD
#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_DIR "build"
#define GRAY_C_O (BUILD_DIR "/gray.c.o")
#define TO_LOWER(str)\
    for (char *_str_ = (str); *_str_; _str_ += 1) { *_str_ = tolower(*_str_); }

#ifndef CC
#   define CC "cc"
#endif // CC
#ifndef CF
#   define CF "-Wall", "-Wextra", "-Wpedantic", "-ggdb"
#endif // CF

enum SubCommand {
    Help,
    SingleHeader,
    SharedLib,
    StaticLib,
    Examples,
    All,
};

static const char *subcmd_strs[All + 1] = {
    [Help] = "help",
    [SingleHeader] = "singleheader",
    [SharedLib] = "sharedlib",
    [StaticLib] = "staticlib",
    [Examples] = "examples",
    [All] = "all",
};

static const char *subcmd_desc[All + 1] = {
    [Help] = "Print this help info",
    [SingleHeader] = "Compile gray.h and gray.c into a single header file",
    [SharedLib] = "Compile to a shared library",
    [StaticLib] = "Compile to a static library",
    [Examples] = "Compile all examples",
    [All] = "Create single header file, build shared and static library and compile examples",
};

bool require_build_dir()
{
    if (!nob_mkdir_if_not_exists(BUILD_DIR))              { return false; }
    if (!nob_mkdir_if_not_exists(BUILD_DIR "/examples" )) { return false; }
    return true;
}

bool create_single_header(Nob_Cmd *cmd)
{
    bool result = true;
    static const char *target = BUILD_DIR "/gray.h";
    nob_log(NOB_INFO, "Creating single header file");

    Nob_String_Builder reader = { 0 };
    Nob_String_Builder sb = { 0 };

    if (!nob_read_entire_file("gray.h", &reader)) {
        result = false;
        goto cleanup;
    }
    nob_sb_append_buf(&sb, reader.items, reader.count);
    reader.count = 0;

    nob_sb_append_cstr(&sb, "\n"
                       "#ifndef _GRAY_ALREADY_IMPLEMENTED_\n"
                       "#    ifdef GRAY_IMPLEMENTATION\n"
                       "#        define _GRAY_ALREADY_IMPLEMENTED_\n\n");

    // comments out the `#include "gray.h"` in gray.c
    nob_sb_append_cstr(&sb, "// ");

    if (!nob_read_entire_file("gray.c", &reader)) {
        result = false;
        goto cleanup;
    }
    nob_sb_append_buf(&sb, reader.items, reader.count);

    nob_sb_append_cstr(&sb, "\n"
                       "#    endif // GRAY_IMPLEMENTATION\n"
                       "#endif // _GRAY_ALREADY_IMPLEMENTED_\n");

    if (!nob_write_entire_file(target, sb.items, sb.count)) {
        result = false;
        goto cleanup;
    }


cleanup:
    nob_sb_free(sb);
    nob_sb_free(reader);

    return result;
}

bool build_shared_lib(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building shared library");

    nob_cmd_append(cmd, CC, CF, "-c", "gray.c", "-o", GRAY_C_O);
    if (!nob_cmd_run_sync_and_reset(cmd)) { return false; }

    nob_cmd_append(cmd, "ar", "rcs", BUILD_DIR "/libgray.a", GRAY_C_O);
    if (!nob_cmd_run_sync_and_reset(cmd)) { return false; }

    return true;
}

bool build_static_lib(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Building static library");

    nob_cmd_append(cmd, CC, CF, "-fPIC", "-c", "gray.c", "-o", GRAY_C_O);
    if (!nob_cmd_run_sync_and_reset(cmd)) { return false; }

    nob_cmd_append(cmd, CC, "-shared", "-o", BUILD_DIR "/libgray.so", GRAY_C_O);
    if (!nob_cmd_run_sync_and_reset(cmd)) { return false; }

    return true;
}

bool build_examples(Nob_Cmd *cmd)
{
    nob_log(NOB_INFO, "Compiling examples");

    nob_cmd_append(cmd, CC, CF, "-c", "gray.c", "-o", GRAY_C_O);
    if (!nob_cmd_run_sync_and_reset(cmd)) { return false; }

    #define raylib_example(name) do {\
        nob_cmd_append(cmd, CC, CF, "-c", "examples/" name ".c", "-o",         \
                       BUILD_DIR "/examples/" name ".c.o");                    \
        if (!nob_cmd_run_sync_and_reset(cmd)) { return false; }                \
        nob_cmd_append(cmd, CC, GRAY_C_O, BUILD_DIR "/examples/" name ".c.o",  \
                       "-lm", "-lraylib", "-o", BUILD_DIR "/examples/" name);  \
        if (!nob_cmd_run_sync_and_reset(cmd)) { return false; }                \
    } while (0);

    // TODO: might want to iterate the examples directory
    raylib_example("01-raylib-animated");

    #undef raylib_example
    return true;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    char *prog_name = nob_shift(argv, argc);
    char *subcommand = "all";
    if (argc > 0) {
        subcommand = nob_shift(argv, argc);
        TO_LOWER(subcommand);
    }

    if (argc > 0) {
        nob_log(NOB_WARNING, "All arguments after the subcommand are ignored");
    }

    Nob_Cmd cmd = { 0 };

    if (strcmp(subcommand, subcmd_strs[Help]) == 0) {
        printf("All available subcommands:\n");
        int max_len = 0;
        for (size_t i = 0; i < NOB_ARRAY_LEN(subcmd_strs); i += 1) {
            if (strlen(subcmd_strs[i]) > max_len) {
                max_len = strlen(subcmd_strs[i]);
            }
        }
        for (size_t i = 0; i < NOB_ARRAY_LEN(subcmd_strs); i += 1) {
            printf(" - %-*s  %s\n", max_len, subcmd_strs[i], subcmd_desc[i]);
        }
    } else if (strcmp(subcommand, subcmd_strs[SingleHeader]) == 0) {
        if (!require_build_dir())    { return EXIT_FAILURE; }
        if (!create_single_header(&cmd)) { return EXIT_FAILURE; }
    } else if (strcmp(subcommand, subcmd_strs[SharedLib]) == 0) {
        if (!require_build_dir())    { return EXIT_FAILURE; }
        if (!build_shared_lib(&cmd))     { return EXIT_FAILURE; }
    } else if (strcmp(subcommand, subcmd_strs[StaticLib]) == 0) {
        if (!require_build_dir())    { return EXIT_FAILURE; }
        if (!build_static_lib(&cmd))     { return EXIT_FAILURE; }
    } else if (strcmp(subcommand, subcmd_strs[Examples]) == 0) {
        if (!require_build_dir())    { return EXIT_FAILURE; }
        if (!build_examples(&cmd))       { return EXIT_FAILURE; }
    } else if (strcmp(subcommand, subcmd_strs[All]) == 0) {
        if (!require_build_dir())    { return EXIT_FAILURE; }

        if (!create_single_header(&cmd)) { return EXIT_FAILURE; }
        if (!build_shared_lib(&cmd))     { return EXIT_FAILURE; }
        if (!build_static_lib(&cmd))     { return EXIT_FAILURE; }
        if (!build_examples(&cmd))       { return EXIT_FAILURE; }
    } else {
        nob_log(NOB_ERROR, "Unknown subcommand '%s', use '%s' to list all available",
                subcommand, subcmd_strs[Help]);
        return EXIT_FAILURE;
    }

    nob_log(NOB_INFO, "Ok");
    return EXIT_SUCCESS;
}
