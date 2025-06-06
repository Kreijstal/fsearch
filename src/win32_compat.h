#pragma once

#ifdef _WIN32

#include <windows.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>

// Define POSIX types that are missing on Windows
#ifndef off_t
#ifdef _WIN64
typedef __int64 off_t;
#else
typedef long off_t;
#endif
#endif

// Define missing constants
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

// Windows doesn't have these POSIX file locking constants
#ifndef LOCK_EX
#define LOCK_EX 2
#endif

#ifndef LOCK_NB
#define LOCK_NB 4
#endif

// Directory separator compatibility
#ifndef G_DIR_SEPARATOR
#define G_DIR_SEPARATOR '\\'
#endif

#ifndef G_DIR_SEPARATOR_S
#define G_DIR_SEPARATOR_S "\\"
#endif

// Function declarations for Windows compatibility
int win32_flock(int fd, int operation);
char *win32_strcasestr(const char *haystack, const char *needle);
int win32_lstat(const char *path, struct stat *buf);
int win32_fnmatch(const char *pattern, const char *string, int flags);
char *win32_strptime(const char *s, const char *format, struct tm *tm);

// Map POSIX functions to Windows implementations
#define flock(fd, op) win32_flock(fd, op)
#define strcasestr(h, n) win32_strcasestr(h, n)
#define lstat(p, b) win32_lstat(p, b)
#define fnmatch(p, s, f) win32_fnmatch(p, s, f)
#define strptime(s, f, tm) win32_strptime(s, f, tm)

// Directory reading compatibility
#include <windows.h>

// POSIX dirent structure for Windows
struct dirent {
    char d_name[260]; // MAX_PATH
};

typedef struct {
    HANDLE handle;
    WIN32_FIND_DATAA find_data;
    struct dirent entry;
    int first;
} DIR;

DIR *win32_opendir(const char *dirname);
struct dirent *win32_readdir(DIR *dirp);
int win32_closedir(DIR *dirp);
int win32_dirfd(DIR *dirp);

#define opendir(d) win32_opendir(d)
#define readdir(d) win32_readdir(d)
#define dirfd(d) win32_dirfd(d)

// For g_clear_pointer compatibility, we need a function that matches the signature
static inline void win32_closedir_clear(DIR *dirp) {
    win32_closedir(dirp);
}
#define closedir win32_closedir_clear

// File status functions
#define AT_SYMLINK_NOFOLLOW 0x100
int win32_fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);
#define fstatat(d, p, b, f) win32_fstatat(d, p, b, f)

// fnmatch flags
#ifndef FNM_NOMATCH
#define FNM_NOMATCH 1
#endif

#ifndef FNM_PATHNAME
#define FNM_PATHNAME (1 << 0)
#endif

#ifndef FNM_NOESCAPE
#define FNM_NOESCAPE (1 << 1)
#endif

#ifndef FNM_PERIOD
#define FNM_PERIOD (1 << 2)
#endif

#endif // _WIN32