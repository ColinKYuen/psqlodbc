/* **********************************************************************************
 * AUTHOR: vaneeden
 * Copyright (C) 2024 Amazon.com All Rights Reserved
 * **********************************************************************************/

/** 
* NAME 
*    dsts_secure_sscanf - secure replacement for sscanf()
* 
* SYNOPSIS 
*    #include "dsts_secure_sscanf.h"
*
*    int 
*    dsts_secure_sscanf(const char *pInputString, int *pStatus, const char *pFmt, ... ) 
* 
* DESCRIPTION 
*    The dsts_secure_sscanf() function is a secure replacement for sscanf(). It
*    scans formatted input from the string str according to the format string
*    format, as described in the FORMATS section below. dsts_secure_sscanf()
*    differs from sscanf() in the following ways:
*
*    1. Types are passed using DSTS_ARG_TYPE() macros.
*    2. Output values are initialized even on errors.
*    3. Numeric conversion errors will be propagated.
*    4. The '%s' format specifier takes an additional argument, a size_t of the
*       destination buffer.
*    5. Output is guaranteed to be \0 terminated.
*    6. Max string output is capped to DSTS_MAX_STRING_OUTPUT (4 * PAGE_SIZE).
* 
* FORMATS
*    The format string is composed of zero or more directives: one or more
*    white-space characters, an ordinary character (neither '%' nor a white-
*    space character), or a conversion specification.  Each conversion
*    specification is introduced by the character '%' after which the following
*    appear in sequence:
* 
*    %c - matches a character or a sequence of characters 
*    %s - matches a sequence of non-whitespace characters (a string) 
*    %d - Matches an optionally signed decimal integer 
*    %u - Matches an unsigned decimal integer 
*    %x - Matches an unsigned hexadecimal integer 
*    %f - Matches an optionally signed floating-point number 
* 
*    The arguments for the format string specifiers are passed through the following macros:
*
*    DSTS_ARG_STR(&buf, sizeof(buf)) for strings (%s)
*    DSTS_ARG_INT(&int1) for integers (%d, %u, %x)
*    DSTS_ARG_FLOAT(&float1) for floats (%f)
*    DSTS_ARG_CHAR(&char1) for char (%c)

* RETURN VALUE
*    The dsts_secure_sscanf() function returns the number of input items
*    successfully matched and assigned; this can be fewer than provided for,
*    or even zero, in the event of an early matching failure between the input
*    string and a directive.
*
*    The pStatus argument will be set to one of the following values:
*
*    DSTS_ERROR_SUCCESS
*    DSTS_ERROR_NUMERIC_CONVERSION
*    DSTS_ERROR_BUFFER_TOO_SMALL
*    DSTS_ERROR_INVALID_FMT
*    DSTS_ERROR_INVALID_TYPE 
*
* EXAMPLES
*
*    ret = dsts_secure_sscanf(pInput, &status, "%s %f %c", 
*                                              DSTS_ARG_STR(&string1, sizeof(string1)),
*                                              DSTS_ARG_FLOAT(&float1),
*                                              DSTS_ARG_CHAR(&char1));
*
*    if (ret != 3 || status != DSTS_ERROR_SUCCESS)
*        // handle error
*
* SEE ALSO 
*    sscanf(3) 
* 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include "dsts_secure_sscanf.h" 

ALWAYS_INLINE char *
skip_spaces(const char *pInputString)
{
    while (isspace(*pInputString)) {
        ++pInputString;
    }

    return (char *)pInputString;
}

const char *
parse_arg(int *pError, const char *pFmt, const char *pInputString, va_list *args)
{
    const char *pCurrent                    = pInputString;
    const char *pParse                      = pInputString;
    char trimCurr[DSTS_MAX_STRING_OUTPUT]   = { 0 };

    char *pEnd                              = NULL;
    char *pFmtEnd                           = NULL;

    void *pOutput                           = NULL;

    float fRet                              = 0.0f;
    long lRet                               = 0;
    long long llRet                         = 0;
    unsigned long ulRet                     = 0;
    unsigned long long ullRet               = 0;

    int iBase                               = 10;
    int iType                               = 0;

    size_t cbSize                           = 0;

    unsigned int uWidth                     = 0;
    unsigned char useTrim                   = 0;

    DSTS_PUT_VALUE(pError, DSTS_ERROR_SUCCESS);

    lRet = strtol(pFmt, &pFmtEnd, 10);

    if (((lRet == HUGE_VALF || lRet == -HUGE_VALF) && (errno == ERANGE)) || (lRet > UINT_MAX)) {
        DSTS_PUT_VALUE(pError, DSTS_ERROR_INVALID_FMT);
        return pCurrent;
    }

    uWidth = lRet;
    if (uWidth > 0) {
        useTrim = 1;
        strncpy(trimCurr, pCurrent, uWidth);
        trimCurr[uWidth] = '\0';
        pParse = trimCurr;
    } else {
        pParse = pCurrent;
    }

    pFmt = pFmtEnd;

    iType = va_arg(*args, int);

    long spType = DSTS_TYPE_DEFAULT;
    switch (*pFmt) {
        case 'h':
            pFmt++;
            if (*pFmt == 'h') {
                pFmt++;
                spType = DSTS_TYPE_CHAR;
            } else {
                spType = DSTS_TYPE_SHORT;
            }
            break;
        case 'l':
            pFmt++;
            if (*pFmt == 'l') {
                pFmt++;
                spType = DSTS_TYPE_LLONG;
            } else {
                spType = DSTS_TYPE_LONG;
            }
            break;
        default:
            break;
    }

    switch (*pFmt) {
        case 'f':

            DSTS_VALIDATE_FMT_TYPE(DSTS_TYPE_FLOAT);
            pOutput = (void *)va_arg(*args, float *);
            DSTS_PUT_VALUE((float *)pOutput, 0.0f);

            fRet = strtof(pParse, &pEnd);
            if (pEnd == pParse) {
                DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                return skip_spaces(pCurrent);
            }

            if (useTrim) {
                while (uWidth-- > 0 && *pParse != *pEnd) {
                    pCurrent++;
                    pParse++;
                } 
            } else {
                pCurrent = pEnd;
            }

            if (((fRet == HUGE_VALF || fRet == -HUGE_VALF)) && (errno == ERANGE)) {
                DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                return skip_spaces(pCurrent);
            }

            DSTS_PUT_VALUE((float *)pOutput, fRet);
            break;

        case 'i':   
            if (*pCurrent == '0') {
                if (*(pCurrent + 1) == 'x') {
                    iBase = 16;
                } else {
                    iBase = 8;
                }
            }
            DSTS_FALL_THROUGH;

        case 'd':
            DSTS_VALIDATE_FMT_TYPE(spType == DSTS_TYPE_DEFAULT ? DSTS_TYPE_INT : spType);
            switch (spType) {
                case DSTS_TYPE_CHAR:
                    pOutput = (void *)va_arg(*args, char *);
                    DSTS_PUT_VALUE((char *)pOutput, 0);
                    lRet = strtol(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_SHORT:
                    pOutput = (void *)va_arg(*args, short *);
                    DSTS_PUT_VALUE((short *)pOutput, 0);
                    lRet = strtol(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_LONG:
                    pOutput = (void *)va_arg(*args, long *);
                    DSTS_PUT_VALUE((long *)pOutput, 0);
                    lRet = strtol(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_LLONG:
                    pOutput = (void *)va_arg(*args, long long *);
                    DSTS_PUT_VALUE((long long *)pOutput, 0);
                    llRet = strtoll(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_DEFAULT:
                    DSTS_FALL_THROUGH;
                default:
                    pOutput = (void *)va_arg(*args, unsigned int *);
                    DSTS_PUT_VALUE((int *)pOutput, 0);
                    lRet = strtol(pParse, &pEnd, iBase);
            }

            if (pEnd == pParse) {
                DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                return skip_spaces(pCurrent);
            }

            if (useTrim) {
                while (uWidth-- > 0 && *pParse != *pEnd) {
                    pCurrent++;
                    pParse++;
                } 
            } else {
                pCurrent = pEnd;
            }

            if ((lRet == HUGE_VAL) && (errno == ERANGE)) {
                DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                return skip_spaces(pCurrent);
            }

            switch (spType) {
                    case DSTS_TYPE_CHAR:
                        if (lRet > CHAR_MAX) {
                            DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                            return skip_spaces(pCurrent);
                        }
                        DSTS_PUT_VALUE((char *)pOutput, lRet);
                        break;

                    case DSTS_TYPE_SHORT:
                        if (lRet > SHRT_MAX) {
                            DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                            return skip_spaces(pCurrent);
                        }
                        DSTS_PUT_VALUE((short *)pOutput, lRet);
                        break;

                    case DSTS_TYPE_LONG:
                        if (lRet > LONG_MAX) {
                            DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                            return skip_spaces(pCurrent);
                        }
                        DSTS_PUT_VALUE((long *)pOutput, lRet);
                        break;

                    case DSTS_TYPE_LLONG:
                        if (llRet > LLONG_MAX) {
                            DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                            return skip_spaces(pCurrent);
                        }
                        DSTS_PUT_VALUE((long long *)pOutput, llRet);
                        break;

                    case DSTS_TYPE_DEFAULT:
                        DSTS_FALL_THROUGH;
                    default:
                        if (lRet > INT_MAX) {
                            DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                            return skip_spaces(pCurrent);
                        }
                        DSTS_PUT_VALUE((int *)pOutput, lRet);
            }
            break;

        case 'X':
            DSTS_FALL_THROUGH;

        case 'x':
            iBase = 16;
            DSTS_FALL_THROUGH;

        case 'u':
            // Up by 1 to "unsigned" type
            unsigned long type = spType == DSTS_TYPE_DEFAULT ? DSTS_TYPE_UINT : spType + 1;
            DSTS_VALIDATE_FMT_TYPE(type);
            switch (type) {
                case DSTS_TYPE_UCHAR:
                    pOutput = (void *)va_arg(*args, unsigned char *);
                    DSTS_PUT_VALUE((unsigned char *)pOutput, 0);
                    ulRet = strtoul(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_USHORT:
                    pOutput = (void *)va_arg(*args, unsigned short *);
                    DSTS_PUT_VALUE((unsigned short *)pOutput, 0);
                    ulRet = strtoul(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_ULONG:
                    pOutput = (void *)va_arg(*args, unsigned long *);
                    DSTS_PUT_VALUE((unsigned long *)pOutput, 0);
                    ulRet = strtoul(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_ULLONG:
                    pOutput = (void *)va_arg(*args, unsigned long long *);
                    DSTS_PUT_VALUE((unsigned long long *)pOutput, 0);
                    ullRet = strtoull(pParse, &pEnd, iBase);
                    break;

                case DSTS_TYPE_DEFAULT:
                    DSTS_FALL_THROUGH;
                default:
                    pOutput = (void *)va_arg(*args, unsigned int *);
                    DSTS_PUT_VALUE((unsigned int *)pOutput, 0);
                    ulRet = strtoul(pParse, &pEnd, iBase);
            }

            if (pEnd == pCurrent) {
                DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                return skip_spaces(pCurrent);
            }

            if (useTrim) {
                while (uWidth-- > 0 && *pParse != *pEnd) {
                    pCurrent++;
                    pParse++;
                }
            } else {
                pCurrent = pEnd;
            }

            if ((ulRet == HUGE_VAL) && (errno == ERANGE)) {
                DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                return skip_spaces(pCurrent);
            }

            switch (type) {
                case DSTS_TYPE_UCHAR:
                    if (ulRet > UCHAR_MAX) {
                        DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                        return skip_spaces(pCurrent);
                    }
                    DSTS_PUT_VALUE((unsigned char *)pOutput, ulRet);
                    break;

                case DSTS_TYPE_USHORT:
                    if (ulRet > USHRT_MAX) {
                        DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                        return skip_spaces(pCurrent);
                    }
                    DSTS_PUT_VALUE((unsigned short *)pOutput, ulRet);
                    break;

                case DSTS_TYPE_ULONG:
                    if (ulRet > ULONG_MAX) {
                        DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                        return skip_spaces(pCurrent);
                    }
                    DSTS_PUT_VALUE((unsigned long *)pOutput, ulRet);
                    break;

                case DSTS_TYPE_ULLONG:
                    if (ullRet > ULLONG_MAX) {
                        DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                        return skip_spaces(pCurrent);
                    }
                    DSTS_PUT_VALUE((unsigned long long *)pOutput, ullRet);
                    break;

                case DSTS_TYPE_DEFAULT:
                    DSTS_FALL_THROUGH;
                default:
                    if (lRet > UINT_MAX) {
                        DSTS_PUT_VALUE(pError, DSTS_ERROR_NUMERIC_CONVERSION);
                        return skip_spaces(pCurrent);
                    }
                    DSTS_PUT_VALUE((unsigned int *)pOutput, ulRet);
            }
            break;

        case 'c':
            DSTS_VALIDATE_FMT_TYPE(DSTS_TYPE_CHAR);
            pOutput = (void *)va_arg(*args, unsigned char *);
            DSTS_PUT_VALUE((char *)pOutput, *pInputString);
            break;

        case 's':
            DSTS_VALIDATE_FMT_TYPE(DSTS_TYPE_STRING);
            pOutput = (void *)va_arg(*args, char *);
            cbSize  = va_arg(*args, size_t);

            if (!pOutput)
                return pCurrent;

            if (cbSize > DSTS_MAX_STRING_OUTPUT) {
                DSTS_PUT_VALUE(pError, DSTS_ERROR_BUFFER_TOO_SMALL);
                return pCurrent;
            }

            if (cbSize) {
                memset(pOutput, '\0', cbSize);
                cbSize--;
                while (pCurrent[0] != '\0' && !isspace(pCurrent[0]) && cbSize != 0 && (useTrim ? uWidth-- > 0 : 1)) {
                    #ifdef _MSC_VER
                        *((char*)pOutput)++ = *pCurrent++;
                    #else
                        *(char *)pOutput++ = *pCurrent++;
                    #endif
                    cbSize--;
                }

                if (cbSize == 0) {
                    /* Truncated ... Should this condition be propogated ? */
                    while (pCurrent[0] != '\0' && !isspace(pCurrent[0]))
                        pCurrent++;
                }
            }

            break;

        default:
            DSTS_PUT_VALUE(pError, DSTS_ERROR_INVALID_FMT);
            return pCurrent;
    }

    return pCurrent;
}

int
dsts_secure_sscanf(const char *pInputString, int *pStatus, const char *pFmt, ... )
{
    const char *pFmtEnd = NULL;

    int error           = 0;
    int ret             = 0;

    if (!pInputString || !pFmt || !pStatus)
        return 0;

    *pStatus = DSTS_ERROR_SUCCESS;

    va_list args;
    va_start(args, pFmt);

    pFmtEnd = pFmt + strlen(pFmt);

    while (pFmt[0] != '\0' && pFmt < pFmtEnd && pInputString[0] != '\0') {
        if (pFmt[0] == '%') {
            const char *tmp = parse_arg(&error, &pFmt[1], pInputString, &args);

            if (error != DSTS_ERROR_SUCCESS) {
                *pStatus = error;
                return ret;
            }

            if (tmp == pInputString) {
                break;
            }

            if (pFmt[1] != '%') {
                ++ret;
            }

            ++pFmt;

            while ((pFmt[0] >= '0' && pFmt[0] <= '9') ||pFmt[0] == 'h' || pFmt[0] == 'l') {
                ++pFmt;
            }

            ++pFmt;
            pInputString = tmp;

        } else if (isspace(pFmt[0])) {
            ++pFmt;
            pInputString = skip_spaces(pInputString);
        } else if (pFmt[0] == pInputString[0]) {
            ++pFmt;
            ++pInputString;
        } else {
            break;
        }
    }

    va_end(args);

    return ret;
}
