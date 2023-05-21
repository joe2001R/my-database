#ifndef PAGER_H
#define PAGER_H

#define MAX_PAGE_NO (100)
#define PAGE_SIZE (4096)

#include <stdint.h>

typedef struct _pager
{
    void* pages[MAX_PAGE_NO];
    uint32_t file_length;
    uint32_t num_pages;
    int fd;
} pager;

pager* pager_open(const char* filename);
void* pager_get_page(pager *pager, uint32_t id);
void pager_flush(pager *pager, uint32_t id);

#endif