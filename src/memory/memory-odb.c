#include <assert.h>
#include <stdio.h>
#include <git2.h>
#include <git2/sys/odb_backend.h>

#include "khash.h"

static kh_inline khint_t kh_oid_hash_func(const git_oid *oid) {
    khint_t h = (khint_t)oid->id[0];
    for (int i = 1; i < GIT_OID_RAWSZ; i++) {
        h = (h << 5) - h + (khint_t)oid->id[i];
    }
    return h;
}

#define KHASH_MAP_INIT_OID(name, khval_t) \
    KHASH_INIT(name, const git_oid *, khval_t, 1, kh_oid_hash_func, git_oid_equal)

typedef struct {
    git_otype type;
    size_t len;
    void *data;
} memory_item;

KHASH_MAP_INIT_OID(mem, memory_item)

typedef struct {
    git_odb_backend parent;
    khash_t(mem) *store;
} memory_odb_backend;

static inline memory_odb_backend *check_backend(git_odb_backend *_backend) {
    assert(_backend);
    return (memory_odb_backend *)_backend;
}

// begin library functions

static int memory_odb_backend__exists(git_odb_backend *_backend, const git_oid *oid) {
    memory_odb_backend *backend = check_backend(_backend);
    khint_t k = kh_get(mem, backend->store, oid);
    return (k != kh_end(backend->store));
}

static int memory_odb_backend__read(void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *oid) {
    memory_odb_backend *backend = check_backend(_backend);
    khint_t k = kh_get(mem, backend->store, oid);
    if (k == kh_end(backend->store)) {
        return GIT_ENOTFOUND;
    }
    memory_item *m = &kh_value(backend->store, k);
    *type_p = m->type;
    *len_p = m->len;
    void *data = malloc(m->len);
    if (data == NULL) {
        return GITERR_NOMEMORY;
    }
    memcpy(data, m->data, m->len);
    *data_p = data;
    return GIT_OK;
}

static int memory_odb_backend__read_header(size_t *len_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *oid) {
    memory_odb_backend *backend = check_backend(_backend);
    khint_t k = kh_get(mem, backend->store, oid);
    if (k == kh_end(backend->store)) {
        return GIT_ENOTFOUND;
    }
    memory_item *m = &kh_value(backend->store, k);
    *type_p = m->type;
    *len_p = m->len;
    return GIT_OK;
}

static int memory_odb_backend__read_prefix(git_oid *out_oid, void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *short_oid, size_t oid_len) {
    memory_odb_backend *backend = check_backend(_backend);
    for (khint_t k = kh_begin(backend->store); k != kh_end(backend->store); ++k) {
        if (kh_exist(backend->store, k)) {
            const git_oid *key = kh_key(backend->store, k);
            if (git_oid_ncmp(short_oid, key, oid_len)) {
                git_oid_cpy(out_oid, key);
                return memory_odb_backend__read(data_p, len_p, type_p, _backend, key);
            }
        }
    }
    return GIT_ENOTFOUND;
}

static int memory_odb_backend__write(git_odb_backend *_backend, const git_oid *oid, const void *data, size_t len, git_otype type) {
    memory_odb_backend *backend = check_backend(_backend);
    assert(oid && data);
    int status;
    if ((status = git_odb_hash((git_oid *)oid, data, len, type)) < 0) {
        return status;
    }
    int absent;
    khint_t k = kh_put(mem, backend->store, oid, &absent);
    git_oid *oid_copy = malloc(sizeof(git_oid));
    git_oid_cpy(oid_copy, oid);

    memory_item *m = &kh_value(backend->store, k);
    if (absent) {
        kh_key(backend->store, k) = oid_copy;
    } else {
        free(m->data);
    }
    m->type = type;
    m->len = len;
    m->data = git_odb_backend_malloc(_backend, len);
    memcpy(m->data, data, len);
    return GIT_OK;
}

static void memory_odb_backend__free(git_odb_backend *_backend) {
    memory_odb_backend *backend = check_backend(_backend);
    // free copied oid keys
    for (khint_t k = kh_begin(backend->store); k != kh_end(backend->store); ++k) {
        if (kh_exist(backend->store, k)) {
            free((void *)kh_key(backend->store, k));
        }
    }
    kh_destroy(mem, backend->store);
    free(backend);
}

int git_odb_backend_memory(git_odb_backend **backend_out) {
    memory_odb_backend *backend;
    backend = calloc(1, sizeof(memory_odb_backend));
    if (backend == NULL) {
        return GITERR_NOMEMORY;
    }
    backend->store = kh_init(mem);
    git_odb_backend *parent = &backend->parent;
    parent->version = GIT_ODB_BACKEND_VERSION;

    parent->exists = &memory_odb_backend__exists;
    parent->read = &memory_odb_backend__read;
    parent->read_header = &memory_odb_backend__read_header;
    parent->read_prefix = &memory_odb_backend__read_prefix;
    parent->write = &memory_odb_backend__write;
    parent->free = &memory_odb_backend__free;
    *backend_out = (git_odb_backend *)backend;
    return GIT_OK;
}
