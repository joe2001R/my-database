#ifndef PAGER_H
#define PAGER_H

#include "pager.fwd.h"

#include <stdbool.h>
#include <stdint.h>

pager *pager_open(const char *filename);
void* pager_get_page(pager *pager, uint32_t id);
void *pager_get_valid_page(pager *pager, uint32_t id);
void* pager_get_valid_page_ensure(pager* pager,uint32_t id);
void* pager_get_free_page(pager* pager);
int32_t pager_find_page_id(pager* pager, void* page);
void pager_flush(pager *pager, uint32_t id);
void pager_destroy_page(pager* pager, uint32_t id);
bool pager_is_empty(pager *pager);

#endif