#ifdef _WIN32

#include "win32_compat.h"
#include <windows.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <glib.h>

// Windows implementation of flock
int win32_flock(int fd, int operation) {
    HANDLE handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    OVERLAPPED overlapped = {0};
    DWORD flags = 0;
    
    if (operation & LOCK_EX) {
        flags = LOCKFILE_EXCLUSIVE_LOCK;
    }
    if (operation & LOCK_NB) {
        flags |= LOCKFILE_FAIL_IMMEDIATELY;
    }
    
    if (LockFileEx(handle, flags, 0, MAXDWORD, MAXDWORD, &overlapped)) {
        return 0;
    }
    return -1;
}

// Windows implementation of strcasestr
char *win32_strcasestr(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        return NULL;
    }
    
    size_t needle_len = strlen(needle);
    if (needle_len == 0) {
        return (char *)haystack;
    }
    
    for (const char *h = haystack; *h; h++) {
        if (g_ascii_strncasecmp(h, needle, needle_len) == 0) {
            return (char *)h;
        }
    }
    return NULL;
}

// Windows implementation of lstat (just use stat since Windows doesn't have symlinks in the same way)
int win32_lstat(const char *path, struct stat *buf) {
    return stat(path, buf);
}

// Simple fnmatch implementation for Windows
static int match_pattern(const char *pattern, const char *string, int flags) {
    const char *p = pattern;
    const char *s = string;
    
    while (*p && *s) {
        if (*p == '*') {
            // Handle wildcard
            p++;
            if (*p == '\0') {
                return 0; // Match
            }
            
            while (*s) {
                if (match_pattern(p, s, flags) == 0) {
                    return 0;
                }
                s++;
            }
            return FNM_NOMATCH;
        } else if (*p == '?') {
            // Match any single character
            p++;
            s++;
        } else {
            // Literal character match
            int match;
            if (flags & FNM_PATHNAME && (*p == '/' || *p == '\\')) {
                // Handle path separators
                match = (*s == '/' || *s == '\\');
            } else {
                // Case-insensitive comparison on Windows
                match = (tolower(*p) == tolower(*s));
            }
            
            if (!match) {
                return FNM_NOMATCH;
            }
            p++;
            s++;
        }
    }
    
    // Handle remaining pattern characters
    while (*p == '*') {
        p++;
    }
    
    return (*p == '\0' && *s == '\0') ? 0 : FNM_NOMATCH;
}

int win32_fnmatch(const char *pattern, const char *string, int flags) {
    if (!pattern || !string) {
        return FNM_NOMATCH;
    }
    
    return match_pattern(pattern, string, flags);
}

// Simple strptime implementation for Windows
char *win32_strptime(const char *s, const char *format, struct tm *tm) {
    // Very basic implementation - only handles common formats
    // This is a simplified version that handles basic ISO date formats
    
    if (!s || !format || !tm) {
        return NULL;
    }
    
    // Clear the tm structure
    memset(tm, 0, sizeof(struct tm));
    
    // Handle common ISO date formats
    if (strcmp(format, "%Y-%m-%d") == 0) {
        int year, month, day;
        if (sscanf(s, "%d-%d-%d", &year, &month, &day) == 3) {
            tm->tm_year = year - 1900;
            tm->tm_mon = month - 1;
            tm->tm_mday = day;
            return (char*)(s + 10); // Return pointer after parsed part
        }
    }
    else if (strcmp(format, "%Y-%m-%d %H:%M:%S") == 0) {
        int year, month, day, hour, min, sec;
        if (sscanf(s, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
            tm->tm_year = year - 1900;
            tm->tm_mon = month - 1;
            tm->tm_mday = day;
            tm->tm_hour = hour;
            tm->tm_min = min;
            tm->tm_sec = sec;
            return (char*)(s + 19); // Return pointer after parsed part
        }
    }
    
    return NULL; // Format not supported or parsing failed
}

// Windows directory reading implementations
DIR *win32_opendir(const char *dirname) {
    DIR *dirp = malloc(sizeof(DIR));
    if (!dirp) {
        return NULL;
    }
    
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", dirname);
    
    dirp->handle = FindFirstFileA(search_path, &dirp->find_data);
    if (dirp->handle == INVALID_HANDLE_VALUE) {
        free(dirp);
        return NULL;
    }
    
    dirp->first = 1;
    return dirp;
}

struct dirent *win32_readdir(DIR *dirp) {
    if (!dirp || dirp->handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    
    if (dirp->first) {
        dirp->first = 0;
    } else {
        if (!FindNextFileA(dirp->handle, &dirp->find_data)) {
            return NULL;
        }
    }
    
    strncpy(dirp->entry.d_name, dirp->find_data.cFileName, sizeof(dirp->entry.d_name) - 1);
    dirp->entry.d_name[sizeof(dirp->entry.d_name) - 1] = '\0';
    
    return &dirp->entry;
}

int win32_closedir(DIR *dirp) {
    if (!dirp) {
        return -1;
    }
    
    if (dirp->handle != INVALID_HANDLE_VALUE) {
        FindClose(dirp->handle);
    }
    free(dirp);
    return 0;
}

int win32_dirfd(DIR *dirp) {
    // Windows doesn't have file descriptors for directories like Unix
    // Return a dummy value - this might need adjustment based on usage
    return -1;
}

int win32_fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) {
    // Since Windows doesn't have directory file descriptors like Unix,
    // we need to get the current directory path and construct the full path.
    // This is a limitation of our Windows compatibility layer.
    
    // If pathname is already absolute, use it directly
    if (pathname && (pathname[0] == '/' || pathname[0] == '\\' ||
                    (pathname[0] && pathname[1] == ':'))) {
        return stat(pathname, buf);
    }
    
    // For relative paths, we need to get the current working directory
    // and construct the full path
    char full_path[MAX_PATH];
    if (GetCurrentDirectoryA(sizeof(full_path), full_path) == 0) {
        return -1;
    }
    
    // Append path separator if needed
    size_t len = strlen(full_path);
    if (len > 0 && full_path[len-1] != '\\' && full_path[len-1] != '/') {
        strcat(full_path, "\\");
    }
    
    // Append the filename
    if (strlen(full_path) + strlen(pathname) >= MAX_PATH) {
        return -1; // Path too long
    }
    strcat(full_path, pathname);
    
    // printf("[DEBUG] fstatat: pathname='%s', full_path='%s'\n", pathname, full_path);
    
    return stat(full_path, buf);
}

#endif // _WIN32