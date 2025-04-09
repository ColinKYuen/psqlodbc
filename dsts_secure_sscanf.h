/* **********************************************************************************
* AUTHOR: vaneeden
* Copyright (C) 2024 Amazon.com All Rights Reserved
* **********************************************************************************/
#define DSTS_MAX_STRING_OUTPUT                  4096 * 4
#define DSTS_ERROR_SUCCESS                      0
#define DSTS_ERROR_NUMERIC_CONVERSION           -1
#define DSTS_ERROR_BUFFER_TOO_SMALL             -2
#define DSTS_ERROR_INVALID_FMT                  -3
#define DSTS_ERROR_INVALID_TYPE                 -4

enum CheckTypes {
    DSTS_TYPE_DEFAULT,
    DSTS_TYPE_FLOAT,
    DSTS_TYPE_INT,
    DSTS_TYPE_UINT,
    DSTS_TYPE_SHORT,
    DSTS_TYPE_USHORT,
    DSTS_TYPE_LONG,
    DSTS_TYPE_ULONG,
    DSTS_TYPE_LLONG,
    DSTS_TYPE_ULLONG,
    DSTS_TYPE_CHAR,
    DSTS_TYPE_UCHAR,
    DSTS_TYPE_STRING
};

#define DSTS_PUT_VALUE(ptr, value) \
    do { if (ptr) *(ptr) = (value); } while (0)

#define DSTS_VALIDATE_FMT_TYPE(type) \
    do { if (iType != (type)) { DSTS_PUT_VALUE(pError, DSTS_ERROR_INVALID_TYPE); return pCurrent; } } while (0)

#define DSTS_ARG_FLOAT(arg)                     DSTS_TYPE_FLOAT, (arg)
#define DSTS_ARG_INT(arg)                       DSTS_TYPE_INT, (arg)
#define DSTS_ARG_UINT(arg)                      DSTS_TYPE_UINT, (arg)
#define DSTS_ARG_SHORT(arg)                     DSTS_TYPE_SHORT, (arg)
#define DSTS_ARG_USHORT(arg)                    DSTS_TYPE_USHORT, (arg)
#define DSTS_ARG_LONG(arg)                      DSTS_TYPE_LONG, (arg)
#define DSTS_ARG_ULONG(arg)                     DSTS_TYPE_ULONG, (arg)
#define DSTS_ARG_LLONG(arg)                     DSTS_TYPE_LLONG, (arg)
#define DSTS_ARG_ULLONG(arg)                    DSTS_TYPE_ULLONG, (arg)
#define DSTS_ARG_CHAR(arg)                      DSTS_TYPE_CHAR, (arg)
#define DSTS_ARG_UCHAR(arg)                     DSTS_TYPE_UCHAR, (arg)
#define DSTS_ARG_STR(arg, size)                 DSTS_TYPE_STRING, (arg), (size)

#if defined(__GNUC__) && __GNUC__ >= 7
    #define DSTS_FALL_THROUGH __attribute__ ((fallthrough))
#else
    #define DSTS_FALL_THROUGH ((void)0)
#endif /* __GNUC__ >= 7 */

#ifdef _MSC_VER
    #define ALWAYS_INLINE __forceinline
#elif defined(__GNUC__)
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
    #define ALWAYS_INLINE inline
#endif

int dsts_secure_sscanf(const char *pInputString, int *pStatus, const char *pFmt, ... );
