#ifndef PAGER_H
#define PAGER_H

#include "constants.h"
#include "pager.fwd.h"

#include <stdint.h>

typedef struct _pager
{
    void* pages[MAX_PAGE_NO];
    uint32_t file_length;
    uint32_t num_pages;
    int fd;
} pager;

pager *pager_open(const char *filename);
void* pager_get_page(pager *pager, uint32_t id);
void *pager_get_valid_page(pager *pager, uint32_t id);
void* pager_get_valid_page_ensure(pager* pager,uint32_t id);
void* pager_get_free_page(pager* pager);
int32_t pager_find_page_id(pager* pager, void* page);
void pager_flush(pager *pager, uint32_t id);
void pager_destroy_page(pager* pager, uint32_t id);

#endif