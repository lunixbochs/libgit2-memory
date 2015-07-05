#include <stdio.h>

#include <git2.h>
#include <git2/sys/mempack.h>
#include <git2/sys/odb_backend.h>
#include <git2/sys/refdb_backend.h>

#include "memory-odb.h"

int repository_create_memory_cb(git_repository **out, const char *path, int bare, void *payload) {
    git_odb *odb = NULL;
    git_odb_backend *odb_mem = NULL;
    int error;
    if (    (error = git_repository_init(out, path, bare)) ||
            (error = git_repository_odb(&odb, *out)) ||
            (error = git_odb_backend_memory(&odb_mem)) ||
            (error = git_odb_add_backend(odb, odb_mem, 999))) {
        // TODO: free?
        return error;
    }
    return GIT_OK;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <repo url>\n", argv[0]);
        return 1;
    }
    git_libgit2_init();

    git_repository *repo = NULL;
    git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
    opts.repository_cb = repository_create_memory_cb;
    const char *url = argv[1];
    const char *path = "repo";
    int error = git_clone(&repo, url, path, &opts);
    if (error) {
        fprintf(stderr, "%s\n", giterr_last()->message);
    }

    git_libgit2_shutdown();
    return 0;
}
