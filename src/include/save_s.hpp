#pragma once
#ifndef __SAVE_S_H__
#define __SAVE_S_H__

#ifndef WIN32

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <cstdarg>

#define sscanf_s sscanf
// #define localtime_s localtime


inline error_t memcpy_s(void *dest, size_t destSize, const void *src, size_t count)
{
    if (dest == NULL || src == NULL)
        return EINVAL;
    if (destSize < count)
        return ERANGE;
    memcpy(dest, src, count);
    return 0;
}

inline error_t strcpy_s(char *dest, size_t destSize, const char *src)
{
    if (dest == NULL || src == NULL)
        return EINVAL;
    if (destSize < strlen(src) + 1)
        return ERANGE;
    strcpy(dest, src);
    return 0;
}

inline error_t strcpy_s(char *dest, const char *src)
{
    strcpy(dest, src);
    return 0;
}

inline error_t strlen_s(const char *str, size_t strSize, size_t *len)
{
    if (str == NULL || len == NULL)
        return EINVAL;
    *len = strnlen(str, strSize);
    return 0;
}

inline size_t strnlen_s(const char *str, size_t strSize)
{
    if (str == NULL)
        return 0;
    return strnlen(str, strSize);
}

inline error_t localtime_s(struct tm *tm, const time_t *time)
{
    if (tm == NULL || time == NULL)
        return EINVAL;
    struct tm *result = localtime(time);
    if (result == NULL)
        return -1;
    *tm = *result;
    return 0;
}

inline int sprintf_s(char *__restrict __s, size_t __maxlen, const char *__restrict __format, ...)
{
    va_list va;
    va_start(va, __format);
    int ret =snprintf(__s, __maxlen, __format, va);
    va_end(va);
    return ret;
}

inline int sprintf_s(char *__restrict __s, const char *__restrict __format, ...)
{
    va_list va;
    va_start(va, __format);
    int ret = sprintf(__s, __format, va);
    return ret;
}

#endif

#endif
