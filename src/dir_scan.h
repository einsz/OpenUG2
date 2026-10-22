/* Sorted directory enumeration for MinGW, which does not provide scandir. */
#ifndef OPENUG2_DIR_SCAN_H
#define OPENUG2_DIR_SCAN_H
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static int n2_dir_name_cmp(const void *a, const void *b) {
    const struct dirent *const *x = a, *const *y = b;
    return strcoll((*x)->d_name, (*y)->d_name);
}

/* Matches scandir(path, out, NULL, alphasort), including ownership on success. */
static int n2_dir_scan(const char *path, struct dirent ***out) {
    *out = NULL;
    DIR *dir = opendir(path);
    if (!dir) return -1;
    struct dirent **items = NULL, *entry;
    int n = 0, capacity = 0, error = 0;
    for (;;) {
        errno = 0;
        entry = readdir(dir);
        if (!entry) { error = errno; break; }
        if (n == capacity) {
            if (capacity > INT_MAX / 2) { error = ENOMEM; break; }
            int next = capacity ? capacity * 2 : 16;
            struct dirent **grown = realloc(items, (size_t)next * sizeof *items);
            if (!grown) { error = ENOMEM; break; }
            items = grown; capacity = next;
        }
        items[n] = malloc(sizeof *entry);
        if (!items[n]) { error = ENOMEM; break; }
        memcpy(items[n++], entry, sizeof *entry);
    }
    closedir(dir);
    if (error) {
        for (int i = 0; i < n; i++) free(items[i]);
        free(items); errno = error; return -1;
    }
    if (n > 1) qsort(items, (size_t)n, sizeof *items, n2_dir_name_cmp);
    *out = items;
    return n;
}
#endif
