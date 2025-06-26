#include "url_encode.h"

#define HEX_CHARS "0123456789abcdef"

int url_encode(char *dest, const size_t destlen, const char *src, const size_t srclen)
{
    int c;
    int ri = 0, wi = 0;
    int max_wi = destlen - 1; // allow space for NULL terminating character

    while ((c = src[ri++]) != '\0')
    {
        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || ('-' == c) || ('.' == c) || ('_' == c) || ('~' == c))
        {
            if (wi < max_wi && dest != NULL)
                dest[wi] = c;
            wi++;
        }
        else
        {
            if (wi < max_wi && dest != NULL)
                dest[wi] = '%';
            wi++;
            if (wi < max_wi && dest != NULL)
                dest[wi] = HEX_CHARS[c >> 4];
            wi++;
            if (wi < max_wi && dest != NULL)
                dest[wi] = HEX_CHARS[c & 15];
            wi++;
        }
    }

    // always write the null (terminating) character
    if (dest != NULL && destlen > 0)
        dest[destlen - 1] = '\0';

    return wi;
}