#ifndef BTREE_H
#define BTREE_H

#include "constants.h"
#include "pager.fwd.h"
#include "row.h"
#include "table.fwd.h"

#include <assert.h>
#include <stdint.h>

typedef enum
{
    LEAF_NODE,
    INTERNAL_NODE
} NodeType;

//common nodes header

#define COMMON_HEADER_OFFSET 0

#define IS_ROOT_OFFSET (COMMON_HEADER_OFFSET + 0)
#define IS_ROOT_SIZE sizeof(uint8_t)
#define PARENT_NODE_OFFSET (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define PARENT_NODE_SIZE sizeof(uint32_t)
#define NODE_TYPE_OFFSET (PARENT_NODE_OFFSET + PARENT_NODE_SIZE)
#define NODE_TYPE_SIZE sizeof(NodeType)

#define COMMON_HEADER_SIZE (IS_ROOT_SIZE + PARENT_NODE_SIZE + NODE_TYPE_SIZE)

uint8_t *node_get_is_root(void *node);
uint32_t *node_get_parent(void *node);
NodeType *node_get_type(void* node);

//leaf nodes header

#define LEAF_NODE_HEADER_OFFSET (COMMON_HEADER_OFFSET + COMMON_HEADER_SIZE)

#define LEAF_NODE_NUM_RECORDS_OFFSET (LEAF_NODE_HEADER_OFFSET + 0)
#define LEAF_NODE_NUM_RECORDS_SIZE sizeof(uint32_t)
#define LEAF_NODE_RIGHT_CHILD_OFFSET (LEAF_NODE_NUM_RECORDS_OFFSET + LEAF_NODE_NUM_RECORDS_SIZE)
#define LEAF_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)

#define LEAF_NODE_HEADER_SIZE (LEAF_NODE_NUM_RECORDS_SIZE + LEAF_NODE_RIGHT_CHILD_SIZE)

//leaf nodes body

#define LEAF_NODE_KEY_REL_OFFSET (0)
#define LEAF_NODE_KEY_SIZE sizeof(uint32_t)
#define LEAF_NODE_ROW_REL_OFFSET (LEAF_NODE_KEY_REL_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_RECORD_SIZE (ROW_SIZE + LEAF_NODE_KEY_SIZE)

#define LEAF_NODE_BODY_OFFSET (LEAF_NODE_HEADER_OFFSET + LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_BODY_SIZE ( PAGE_SIZE - LEAF_NODE_HEADER_SIZE - COMMON_HEADER_SIZE)

#define LEAF_NODE_MAX_NUM_RECORDS (LEAF_NODE_BODY_SIZE / LEAF_NODE_RECORD_SIZE)

#define LEAF_NODE_LEFT_SPLIT_COUNT ((LEAF_NODE_MAX_NUM_RECORDS + 1 )/2)
#define LEAF_NODE_RIGHT_SPLIT_COUNT (LEAF_NODE_MAX_NUM_RECORDS + 1 - LEAF_NODE_LEFT_SPLIT_COUNT)

static_assert(LEAF_NODE_MAX_NUM_RECORDS > 0, "leaf's node maximum number of records is less than or equal to 0");

string_buffer btree_get_diagnostics();
string_buffer btree_print_tree(void* root_node,pager* pager);

uint32_t* leaf_node_get_num_records(void* node);
uint32_t* leaf_node_get_right_child(void* node);

void leaf_node_root_init(void* node);

void *leaf_node_get_row(void *node, uint32_t index);

void *leaf_node_find_row(void *node, uint32_t key, uint32_t *row_index);
void leaf_node_insert_row(void* node,uint32_t key,void* row_to_insert,table* table);

// internal node header

#define INTERNAL_NODE_HEADER_OFFSET (COMMON_HEADER_OFFSET + COMMON_HEADER_SIZE)

#define INTERNAL_NODE_NUM_KEYS_OFFSET (INTERNAL_NODE_HEADER_OFFSET)
#define INTERNAL_NODE_NUM_KEYS_SIZE (sizeof(uint32_t))

#define INTERNAL_NODE_HEADER_SIZE (INTERNAL_NODE_NUM_KEYS_SIZE)

// internal node body

#define INTERNAL_NODE_BODY_OFFSET (INTERNAL_NODE_HEADER_OFFSET + INTERNAL_NODE_HEADER_SIZE)

#define INTERNAL_NODE_BODY_SIZE (PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE - COMMON_HEADER_SIZE)
//child 0 key 0 child 1 key 1 child 2.... etc
#define INTERNAL_NODE_CHILD_REL_OFFSET (0)
#define INTERNAL_NODE_CHILD_SIZE (sizeof(uint32_t))
#define INTERNAL_NODE_KEY_REL_OFFSET (INTERNAL_NODE_CHILD_REL_OFFSET + INTERNAL_NODE_CHILD_SIZE)
#define INTERNAL_NODE_KEY_SIZE (sizeof(uint32_t))

#define INTERNAL_NODE_MAX_NUM_KEYS ((INTERNAL_NODE_BODY_SIZE - INTERNAL_NODE_CHILD_SIZE)/(INTERNAL_NODE_KEY_SIZE + INTERNAL_NODE_CHILD_SIZE))

static_assert(INTERNAL_NODE_MAX_NUM_KEYS > 0, "internal nodes 's maximum number of keys is less than or equal to 0");

uint32_t* internal_node_get_num_keys(void* node);
uint32_t* internal_node_get_key(void* node,uint32_t index);
uint32_t* internal_node_get_child(void* node,uint32_t index);

void internal_node_insert_node(void* internal_node,void* node_to_insert,pager* pager);
void* internal_node_find_node(void* internal_node,uint32_t key,pager* pager);

#endif