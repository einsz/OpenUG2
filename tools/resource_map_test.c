/* Read-only file mapping contract, independent of SDL and graphics. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE
#endif
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#else
#include <unistd.h>
#endif

#include "resource.h"

#ifdef _WIN32
static void wide_path(const char *path, wchar_t wide[4096]) {
    assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                               wide, 4096) > 0);
}
static FILE *open_file(const char *path, const char *mode) {
    wchar_t wide[4096];
    wide_path(path, wide);
    return _wfopen(wide, !strcmp(mode, "wb") ? L"wb" : L"rb");
}
static void remove_file(const char *path) {
    wchar_t wide[4096]; wide_path(path, wide);
    assert(DeleteFileW(wide));
}
#else
#define open_file fopen
static void remove_file(const char *path) { assert(remove(path) == 0); }
#endif

int main(void) {
    char root[4096], path[4096], missing[4096];
#ifdef _WIN32
    wchar_t temp[4096], wide[4096];
    DWORD length = GetTempPathW(4096, temp);
    assert(length > 0 && length < 4096);
    assert(GetTempFileNameW(temp, L"ug2", 0, wide));
    assert(DeleteFileW(wide));
    assert(CreateDirectoryW(wide, NULL));
    assert(WideCharToMultiByte(CP_UTF8, 0, wide, -1, root, sizeof root,
                               NULL, NULL) > 0);
#else
    const char *temp = getenv("TMPDIR");
    if (!temp || !*temp) temp = "/tmp";
    assert(snprintf(root, sizeof root, "%s/openug2-map-XXXXXX", temp) < (int)sizeof root);
    assert(mkdtemp(root));
#endif
    /* UTF-8 name also exercises CreateFileW rather than the Windows codepage. */
    assert(snprintf(path, sizeof path, "%s/map-\xc4\xb1-\xe4\xb8\xad.bin", root) < (int)sizeof path);
    assert(snprintf(missing, sizeof missing, "%s/missing.bin", root) < (int)sizeof missing);
    long len = -1;
    assert(!res_map_file(missing, &len));
    assert(!res_map_file(NULL, &len));
    assert(!res_map_file(path, NULL));
    assert(!res_map_file(root, &len));
    FILE *file = open_file(path, "wb");
    assert(file && fclose(file) == 0);
    assert(!res_map_file(path, &len));
    assert(len == -1);

    unsigned char bytes[8193];
    for (size_t i = 0; i < sizeof bytes; i++) bytes[i] = (unsigned char)i;
    file = open_file(path, "wb");
    assert(file && fwrite(bytes, 1, sizeof bytes, file) == sizeof bytes);
    assert(fclose(file) == 0);
    for (int i = 0; i < 256; i++) {
        unsigned char *data = res_map_file(path, &len);
        assert(data && len == (long)sizeof bytes);
        assert(!memcmp(data, bytes, sizeof bytes));
        res_unmap_file(data, len);
    }
    res_unmap_file(NULL, 0);
    remove_file(path);

#ifdef _WIN32
    /* Use a sparse file so the LLP64 overflow check costs no 2 GiB allocation. */
    wide_path(path, wide);
    HANDLE large = CreateFileW(wide, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    assert(large != INVALID_HANDLE_VALUE);
    DWORD returned;
    if (DeviceIoControl(large, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &returned, NULL)) {
        LARGE_INTEGER size; size.QuadPart = (LONGLONG)LONG_MAX + 1;
        assert(SetFilePointerEx(large, size, NULL, FILE_BEGIN));
        assert(SetEndOfFile(large));
        assert(CloseHandle(large));
        len = -1;
        assert(!res_map_file(path, &len) && len == -1);
    } else {
        assert(CloseHandle(large));
        puts("resource_map_test: sparse overflow case unavailable on this filesystem");
    }
    remove_file(path);
    wide_path(root, wide);
    assert(RemoveDirectoryW(wide));
#else
    assert(rmdir(root) == 0);
#endif
    puts("resource_map_test: PASS");
    return 0;
}
