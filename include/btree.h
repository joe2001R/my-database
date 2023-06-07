#ifndef BTREE_H
#define BTREE_H

#include "constants.h"
#include "pager.fwd.h"
#include "row.h"
#include "table.fwd.h"

#include <stdint.h>

typedef enum
{
    LEAF_NODE,
    INTERNAL_NODE
} NodeType;

typedef struct _LeafNodeRowPair
{
    void *node;
    void *row;
} LeafNodeRowPair;

string_buffer btree_get_diagnostics();
string_buffer btree_print_tree(void* root_node,pager* pager);

uint32_t leaf_node_read_num_records(void* node);
uint32_t leaf_node_read_right_child(void* node);

void leaf_node_root_init(void* node);

bool node_is_root(void *node);

const void* leaf_node_read_row(void *node, uint32_t index);

void leaf_node_insert_row(void* node,uint32_t key,void* row_to_insert,table* table);

LeafNodeRowPair find_row(void *node, uint32_t key, uint32_t *row_index, pager *pager);

#endif