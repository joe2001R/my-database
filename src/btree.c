#include "btree.h"
#include "pager.h"
#include "row.h"
#include "table.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

uint8_t* node_get_is_root(void* node)
{
    return (uint8_t*)(node + IS_ROOT_OFFSET);
}

uint32_t *node_get_parent(void *node)
{
    return (uint32_t *)(node + PARENT_NODE_OFFSET);
}

NodeType *node_get_type(void *node)
{
    return (NodeType*)(node + NODE_TYPE_OFFSET);
}

//private functions

static void leaf_node_init(void* node)
{
    *node_get_type(node) = LEAF_NODE;
    *node_get_is_root(node) = false;
    *leaf_node_get_num_records(node) = 0;
    *leaf_node_get_right_child(node) = 0;
}

static void* internal_node_pager_get_child(void* node,uint32_t child_index,pager* pager)
{
    return pager_get_valid_page(pager,*internal_node_get_child(node,child_index));
}

static void* internal_node_pager_get_right_child(void* node,pager* pager)
{
    return internal_node_pager_get_child(node,*internal_node_get_num_keys(node),pager);
}

static void* leaf_node_get_record(void* node,uint32_t index)
{
    return (node + LEAF_NODE_BODY_OFFSET + index*LEAF_NODE_RECORD_SIZE);
}

static uint32_t* leaf_node_get_key(void* node,uint32_t index)
{
    return (uint32_t*)(leaf_node_get_record(node,index) + LEAF_NODE_KEY_REL_OFFSET);
}

static uint32_t leaf_node_lower_bound(void* node,uint32_t key)
{
    uint32_t num_records = *leaf_node_get_num_records(node);

    uint32_t upper = num_records;
    uint32_t lower = 0;

    while(upper > lower)
    {
        uint32_t mid = (upper + lower) / 2;
        uint32_t key_at_mid = *leaf_node_get_key(node,mid);
        
        if(key_at_mid >= key)
        {
            upper = mid;
        }
        else
        {
            lower = mid + 1;
        }
    }

    return lower;
}

static uint32_t internal_node_child_index_lower_bound(void *node, uint32_t key)
{
    uint32_t num_keys = *internal_node_get_num_keys(node);

    uint32_t upper = num_keys;
    uint32_t lower = 0;

    while (upper > lower)
    {
        uint32_t mid = (upper + lower) / 2;
        uint32_t key_at_mid = *internal_node_get_key(node, mid);

        if (key_at_mid >= key)
        {
            upper = mid;
        }
        else
        {
            lower = mid + 1;
        }
    }

    return lower;
}

static uint32_t leaf_node_get_max_key(void* node)
{
    return *leaf_node_get_key(node,*leaf_node_get_num_records(node)-1);
}

static uint32_t pager_get_page_id(pager* pager,void* page)
{
    int32_t id = pager_find_page_id(pager,page);
    ensure(id!=-1,"Error: could not find page id in pager\n");

    return (uint32_t)id;
}

static void internal_node_root_init(void *node, void *leaf_node_1, void *leaf_node_2, table *table)
{
    *node_get_type(node) = INTERNAL_NODE;
    *node_get_is_root(node) = true;
    *internal_node_get_key(node, 0) = leaf_node_get_max_key(leaf_node_1);
    *internal_node_get_child(node, 0) = pager_get_page_id(table->pager, leaf_node_1);
    *internal_node_get_child(node, 1) = pager_get_page_id(table->pager, leaf_node_2);
    *internal_node_get_num_keys(node) = 1;

    table->root_page_index = pager_get_page_id(table->pager,node);
}

static void leaf_node_split_and_insert(void *old_leaf_node, uint32_t key, row *row_to_insert, table *table)
{
    void *new_leaf_node_1 = pager_get_free_page(table->pager);
    void *new_leaf_node_2 = pager_get_free_page(table->pager);

    leaf_node_init(new_leaf_node_1);
    leaf_node_init(new_leaf_node_2);

    void *new_record = Malloc(LEAF_NODE_RECORD_SIZE);
    memcpy(new_record + LEAF_NODE_KEY_REL_OFFSET, &key, LEAF_NODE_KEY_SIZE);
    row_serialize(new_record + LEAF_NODE_ROW_REL_OFFSET, row_to_insert);

    uint32_t row_to_insert_index = leaf_node_lower_bound(old_leaf_node, key);

    for (int i = 0; i < LEAF_NODE_MAX_NUM_RECORDS + 1; i++)
    {
        void *record_to_insert = (i == row_to_insert_index ? new_record : (i < row_to_insert_index ? leaf_node_get_record(old_leaf_node, i) : leaf_node_get_record(old_leaf_node, i - 1)));
        void *destination_node = (i < LEAF_NODE_LEFT_SPLIT_COUNT ? new_leaf_node_1 : new_leaf_node_2);
        uint32_t j = (i < LEAF_NODE_LEFT_SPLIT_COUNT ? i : i - LEAF_NODE_LEFT_SPLIT_COUNT);

        memcpy(leaf_node_get_record(destination_node, j), record_to_insert, LEAF_NODE_RECORD_SIZE);
    }

    *leaf_node_get_num_records(new_leaf_node_1)=LEAF_NODE_LEFT_SPLIT_COUNT;
    *leaf_node_get_num_records(new_leaf_node_2)=LEAF_NODE_RIGHT_SPLIT_COUNT;
    
    *leaf_node_get_right_child(new_leaf_node_1)=pager_get_page_id(table->pager,new_leaf_node_2);

    ensure(*node_get_type(old_leaf_node)==LEAF_NODE,"Error: split and insert applies only for the root leaf node\n");

    internal_node_root_init(old_leaf_node,new_leaf_node_1,new_leaf_node_2,table);

    *node_get_parent(new_leaf_node_1) = pager_get_page_id(table->pager,old_leaf_node);
    *node_get_parent(new_leaf_node_2) = pager_get_page_id(table->pager,old_leaf_node);

    destroy(&new_record);
}

/*** ------------------------------------------------------------- ***/

string_buffer btree_get_diagnostics()
{
    string_buffer buf;
    
    char char_arr[512];
    char* str = char_arr;

    str = str + sprintf(str, "page size is : %d\n", PAGE_SIZE);
    str = str + sprintf(str, "maximum number of pages is : %d\n", MAX_PAGE_NO);

    str = str + sprintf(str, "----------\n");

    str = str + sprintf(str,"common header size is : %ld\n", COMMON_HEADER_SIZE);
    str = str + sprintf(str,"leaf node header size is : %ld\n", LEAF_NODE_HEADER_SIZE);
    str = str + sprintf(str,"leaf node body size is : %ld\n", LEAF_NODE_BODY_SIZE);
    str = str + sprintf(str,"leaf node record size is : %ld\n", LEAF_NODE_RECORD_SIZE);
    str = str + sprintf(str,"\tleaf node key size is : %ld\n", LEAF_NODE_KEY_SIZE);
    str = str + sprintf(str,"\tleaf node row size is : %ld\n", ROW_SIZE);
    str = str + sprintf(str,"leaf node body max number of record is : %ld\n", LEAF_NODE_MAX_NUM_RECORDS);

    str = str + sprintf(str,"----------\n");

    str = str + sprintf(str, "internal node header size is : %ld\n",INTERNAL_NODE_HEADER_SIZE);
    str = str + sprintf(str, "internal node body size is : %ld\n", INTERNAL_NODE_BODY_SIZE);
    str = str + sprintf(str, "internal node maximum number of keys is : %ld\n", INTERNAL_NODE_MAX_NUM_KEYS);

    string_buffer_init(&buf);
    string_buffer_store(&buf,char_arr);

    return buf;
}

void* leaf_node_get_row(void *node, uint32_t index)
{
    return leaf_node_get_record(node, index) + LEAF_NODE_ROW_REL_OFFSET;
}

uint32_t *leaf_node_get_num_records(void *node)
{
    return (uint32_t*)(node + LEAF_NODE_NUM_RECORDS_OFFSET);
}

uint32_t *leaf_node_get_right_child(void *node)
{
    return (uint32_t *)(node + LEAF_NODE_RIGHT_CHILD_OFFSET);
}

void *leaf_node_find_row(void *node, uint32_t key, uint32_t *row_index)
{
    uint32_t num_records = *leaf_node_get_num_records(node);

    uint32_t lower_bound_index = leaf_node_lower_bound(node,key);
    uint32_t lower_bound_key = *leaf_node_get_key(node, lower_bound_index);

    if( lower_bound_index>=num_records || lower_bound_key != key)
    {
        return NULL;
    }

    if(row_index != NULL)
    {
        *row_index = lower_bound_index;
    }

    return leaf_node_get_row(node,lower_bound_index);
}

void leaf_node_insert_row(void* node, uint32_t key, void* row_to_insert,table* table)
{
    ensure(leaf_node_find_row(node,key,NULL)==NULL,"Error: key %d already exists.\n",key);

    uint32_t* num_records = leaf_node_get_num_records(node);

    if(*num_records == LEAF_NODE_MAX_NUM_RECORDS)
    {
        return leaf_node_split_and_insert(node,key,row_to_insert,table);
    }

    uint32_t row_to_insert_index = leaf_node_lower_bound(node, key);

    *num_records = *num_records + 1;

    for (int i = ((int)*num_records - 1); i > row_to_insert_index; i--)
    {
        memcpy(leaf_node_get_record(node,i),leaf_node_get_record(node,i-1),LEAF_NODE_RECORD_SIZE);
    }
    
    memcpy(leaf_node_get_key(node,row_to_insert_index),&key,sizeof(key));
    memcpy(leaf_node_get_row(node,row_to_insert_index),row_to_insert,ROW_SIZE);
}

void leaf_node_root_init(void *node)
{
    leaf_node_init(node);
    *node_get_is_root(node) = true;
}

uint32_t *internal_node_get_num_keys(void *node)
{
    return (uint32_t*)(node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t *internal_node_get_key(void *node, uint32_t index)
{
    return (uint32_t*)(node + INTERNAL_NODE_BODY_OFFSET + (INTERNAL_NODE_KEY_SIZE + INTERNAL_NODE_CHILD_SIZE)*index + INTERNAL_NODE_KEY_REL_OFFSET);
}

uint32_t *internal_node_get_child(void *node, uint32_t index)
{
    return (uint32_t*)(node + INTERNAL_NODE_BODY_OFFSET + (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE)*index +  INTERNAL_NODE_CHILD_REL_OFFSET );
}

void internal_node_insert_node(void *internal_node, void *node_to_insert, uint32_t node_page_index,pager* pager)
{
    // scan keys: lower bound
    uint32_t child_index = internal_node_child_index_lower_bound(internal_node,leaf_node_get_max_key(node_to_insert));

    //edge case: node_to_insert is to be ordered after internal_node's right child
    if (leaf_node_get_max_key(node_to_insert) > leaf_node_get_max_key(internal_node_pager_get_right_child(internal_node,pager)))
    {
        child_index +=1;
    }

    *internal_node_get_num_keys(internal_node) = *internal_node_get_num_keys(internal_node) + 1;

    // move children
    for (int i = *internal_node_get_num_keys(internal_node); i > child_index; i--)
    {
        memcpy(internal_node_get_child(internal_node, i), internal_node_get_child(internal_node, i - 1), INTERNAL_NODE_CHILD_SIZE);
    }

    memcpy(internal_node_get_child(internal_node, child_index), &node_page_index, INTERNAL_NODE_CHILD_SIZE);
 
    //update keys

    for (int i = MIN(child_index,*internal_node_get_num_keys(internal_node)-1);i<*internal_node_get_num_keys(internal_node);i++)
    {
        *internal_node_get_key(internal_node,i) = leaf_node_get_max_key(internal_node_pager_get_child(internal_node,i,pager));
    }

}