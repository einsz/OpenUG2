/* Sorted enumeration and error ownership without game assets. */
#include "dir_scan.h"
#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define test_mkdir(p) _mkdir(p)
#define test_rmdir(p) _rmdir(p)
#define test_pid() _getpid()
#else
#include <sys/stat.h>
#include <unistd.h>
#define test_mkdir(p) mkdir(p, 0700)
#define test_rmdir(p) rmdir(p)
#define test_pid() getpid()
#endif
int main(void) {
    char root[128], path[160];
    snprintf(root, sizeof root, "build/dir-scan-%ld", (long)test_pid());
    assert(test_mkdir(root) == 0);
    const char *names[] = {"ROUTESz", "ROUTESa", "other"};
    for (int i = 0; i < 3; i++) {
        snprintf(path, sizeof path, "%s/%s", root, names[i]);
        FILE *f = fopen(path, "wb"); assert(f && fclose(f) == 0);
    }
    for (int pass = 0; pass < 64; pass++) {
        struct dirent **items = NULL;
        int n = n2_dir_scan(root, &items), found = 0;
        assert(n >= 3);
        for (int i = 0; i < n; i++) {
            if (i) assert(strcoll(items[i-1]->d_name, items[i]->d_name) <= 0);
            for (int k = 0; k < 3; k++) if (!strcmp(items[i]->d_name, names[k])) found++;
        }
        assert(found == 3);
        for (int i = 0; i < n; i++) free(items[i]);
        free(items);
    }
    for (int i = 0; i < 3; i++) {
        snprintf(path, sizeof path, "%s/%s", root, names[i]); assert(remove(path) == 0);
    }
    assert(test_rmdir(root) == 0);
    struct dirent **items = (void *)1;
    assert(n2_dir_scan(root, &items) == -1 && !items);
    puts("dir_scan_test: PASS");
    return 0;
}
