#ifndef __URL_ENCODE_H
#define __URL_ENCODE_H

#include <stddef.h>

extern int url_encode(char *dest, const size_t destlen, const char *src, const size_t srclen);

#endif /* __URL_ENCODE_H */
