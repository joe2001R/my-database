#include "btree.h"
#include "row.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void leaf_node_insert_row(void* node, uint32_t key, void* row_to_insert)
{
    ensure(leaf_node_find_row(node,key,NULL)==NULL,"Error: key %d already exists.\n",key);

    uint32_t row_to_insert_index = leaf_node_lower_bound(node,key);

    uint32_t* num_records = leaf_node_get_num_records(node);
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
    *node_get_type(node) = LEAF_NODE;
    *node_get_is_root(node) = true;
    *leaf_node_get_num_records(node) = 0;
}