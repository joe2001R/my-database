#include "pager.h"
#include "utilities.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// private functions

static void pager_init(pager* pager)
{
    for (int i = 0; i < MAX_PAGE_NO; i++)
    {
        pager->pages[i] = NULL;
    }
}

pager* pager_open(const char *filename)
{
    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    ensure(fd != -1, "unable %s to open file in `pager_open`\n",filename);

    pager *return_value = Malloc(sizeof(pager));
    
    off_t file_length = lseek(fd,0,SEEK_END);

    ensure(file_length%PAGE_SIZE == 0, "invalid file length : PAGE_SIZE (%d) does not evenly divide the opened file's length (%d)\n",PAGE_SIZE,file_length);

    return_value->fd=fd;
    return_value->file_length = file_length;
    return_value->num_pages = file_length / PAGE_SIZE;
    
    pager_init(return_value);

    return return_value;
}

void *pager_get_page(pager *pager, uint32_t id)
{
    ensure(id < MAX_PAGE_NO, "invalid page id : page id (%d) is too big\n",id);

    if(pager->pages[id] == NULL)
    {
        uint32_t file_num_pages = pager->file_length / PAGE_SIZE;
        pager->pages[id] = Malloc(PAGE_SIZE);

        if( id < file_num_pages)
        {
            ensure(lseek(pager->fd,id*PAGE_SIZE,SEEK_SET) != -1,"invalid file seek in `pager_get_page`\n");
            ensure(read(pager->fd,pager->pages[id],PAGE_SIZE) != -1,"could not read from file in `pager_get_page`\n");
        }

        if(id >= pager->num_pages)
        {
            pager->num_pages = id + 1;
        }
    }

    return pager->pages[id];
}

void *pager_get_valid_page(pager *pager, uint32_t id)
{
    uint32_t file_num_pages = pager->file_length /PAGE_SIZE;
    
    if (id >= file_num_pages && pager->pages[id] == NULL)
    {
        return NULL;
    }
    
    return pager_get_page(pager,id);
}

void *pager_get_valid_page_ensure(pager *pager, uint32_t id)
{
    void* page = pager_get_valid_page(pager,id);
    ensure(page!=NULL,"Error: valid page is NULL in `pager_get_valid_page_ensure` call\n");
    
    return page;
}

void* pager_get_free_page(pager *pager)
{
    return pager_get_page(pager,pager->num_pages);
}

int32_t pager_find_page_id(pager *pager, void *page)
{
    for(int i = 0; i < pager->num_pages;i++)
    {
        if(pager->pages[i] == page)
        {
            return i;
        }
    }

    return -1;
}

void pager_flush(pager *pager, uint32_t id)
{
    ensure(pager->pages[id] != NULL, "flushing a null page\n");
    ensure(lseek(pager->fd, id * PAGE_SIZE, SEEK_SET) != -1, "invalid file seek in `pager_flush`\n");
    ensure(write(pager->fd, pager->pages[id], PAGE_SIZE) != -1, "could not write to file in `pager_flush`\n");
}