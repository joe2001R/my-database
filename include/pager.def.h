#ifndef PAGER_DEF_H
#define PAGER_DEF_H

#include "constants.h"
#include "pager.fwd.h"

#include <stdint.h>

typedef struct _pager
{
    void *pages[MAX_PAGE_NO];
    uint32_t file_length;
    uint32_t num_pages;
    int fd;
} pager;

#endif