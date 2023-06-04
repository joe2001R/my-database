#include "btree.h"
#include "pager.h"
#include "row.h"
#include "table.def.h"
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

static uint32_t internal_node_get_node_key(void* internal_node,pager* pager);

static uint32_t node_get_node_key(void* node,pager* pager)
{
    if(*node_get_type(node) == LEAF_NODE)
    {
        return leaf_node_get_max_key(node);
    }
    else if(*node_get_type(node) == INTERNAL_NODE)
    {
        return internal_node_get_node_key(node,pager);
    }

    fprintf(stderr,"Error: invalid node type encountered in `node_get_node_key`");
    exit(EXIT_FAILURE);
}

static uint32_t internal_node_get_node_key(void* internal_node,pager* pager)
{
    return node_get_node_key(internal_node_pager_get_right_child(internal_node, pager), pager);
}

static uint32_t pager_get_page_id(pager* pager,void* page)
{
    int32_t id = pager_find_page_id(pager,page);
    ensure(id!=-1,"Error: could not find page id in pager\n");

    return (uint32_t)id;
}

static void internal_node_init(void* node,uint32_t parent_node_id)
{
    *node_get_type(node) = INTERNAL_NODE;
    *node_get_is_root(node) = false;
    *node_get_parent(node) = parent_node_id;
}

static void internal_node_init2(void* node,uint32_t parent_node_id,void* node_1,void* node_2,pager* pager)
{
    internal_node_init(node,parent_node_id);

    *internal_node_get_key(node, 0) = node_get_node_key(node_1, pager);
    *internal_node_get_child(node, 0) = pager_get_page_id(pager, node_1);
    *internal_node_get_child(node, 1) = pager_get_page_id(pager, node_2);
    *internal_node_get_num_keys(node) = 1;

    *node_get_parent(node_1) = pager_get_page_id(pager, node);
    *node_get_parent(node_2) = pager_get_page_id(pager, node);
}

static void internal_node_root_init(void *node, void *node_1, void *node_2, table* table)
{
    internal_node_init2(node,0,node_1,node_2,table->pager);

    *node_get_is_root(node) = true;
    table->root_page_index = pager_get_page_id(table->pager, node);

}

static void leaf_node_split_and_insert(void *old_leaf_node, uint32_t key, row *row_to_insert, table *table)
{
    ensure(*node_get_type(old_leaf_node) == LEAF_NODE, "Error: split and insert applies only for the root leaf node\n");

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
    
    if(*node_get_is_root(old_leaf_node))
    {
        internal_node_root_init(old_leaf_node, new_leaf_node_1, new_leaf_node_2, table);
    }
    else
    {   
        *node_get_parent(new_leaf_node_1) = *node_get_parent(old_leaf_node);

        uint32_t new_leaf_node_2_page_id = pager_get_page_id(table->pager,new_leaf_node_2);

        memcpy(old_leaf_node,new_leaf_node_1,PAGE_SIZE);
        memcpy(new_leaf_node_1,new_leaf_node_2,PAGE_SIZE);
        pager_destroy_page(table->pager, new_leaf_node_2_page_id);

        new_leaf_node_2 = new_leaf_node_1; //!! dangling pointer
        new_leaf_node_1 = old_leaf_node;

        void* internal_node = pager_get_valid_page_ensure(table->pager, *node_get_parent(new_leaf_node_1));
        
        uint32_t old_index = internal_node_child_index_lower_bound(internal_node,key);

        *internal_node_get_key(internal_node,old_index) = leaf_node_get_max_key(new_leaf_node_1);
        internal_node_insert_node(internal_node,new_leaf_node_2,table);
    }

    *leaf_node_get_right_child(new_leaf_node_1) = pager_get_page_id(table->pager, new_leaf_node_2);

    DESTROY(new_record);
}

static string_buffer get_cumulated_padding(const char* padding, size_t level)
{
    string_buffer buffer;

    string_buffer_init(&buffer);

    if(level == 0)
    {
        string_buffer_store(&buffer,"");
    }

    for(;level>0;level--)
    {
        string_buffer_append(&buffer,padding);
    }

    return buffer;
}

static void btree_print_leaf_node(void* leaf_node,string_buffer* buffer,const char* padding,size_t level)
{
    string_buffer cumulated_padding_buffer = get_cumulated_padding(padding,level);

    string_buffer_append2(buffer,"%sleaf node: num of records is %ld\n",cumulated_padding_buffer.string,*leaf_node_get_num_records(leaf_node));

    for(uint32_t i = 0;i<*leaf_node_get_num_records(leaf_node);i++)
    {
        row row_to_print;

        row_deserialize(&row_to_print,leaf_node_get_row(leaf_node,i));

        string_buffer_append2(buffer,"%s%skey:%d, value:%s\n",cumulated_padding_buffer.string,padding,row_to_print.id,row_to_print.name);
    }

    string_buffer_destroy(&cumulated_padding_buffer);
}

static void btree_print_node(void* node,string_buffer* buffer,size_t level,pager* pager);

static void btree_print_internal_node(void* internal_node,string_buffer* buffer,const char* padding,size_t level,pager* pager)
{
    string_buffer cumulated_padding_buffer = get_cumulated_padding(padding, level);

    string_buffer_append2(buffer,"%sinternal node: num of nodes is %d\n",cumulated_padding_buffer.string,*internal_node_get_num_keys(internal_node)+1);

    for(uint32_t i = 0;i<*internal_node_get_num_keys(internal_node);i++)
    {
        string_buffer_append2(buffer,"%s%schild%d's key is %d\n",cumulated_padding_buffer.string,padding,i,*internal_node_get_key(internal_node,i));
        
        btree_print_node(internal_node_pager_get_child(internal_node, i, pager), buffer, level + 1, pager);
    }

    string_buffer_append2(buffer, "%s%sright child\n", cumulated_padding_buffer.string, padding);
    btree_print_node(internal_node_pager_get_right_child(internal_node, pager), buffer, level + 1, pager);

    string_buffer_destroy(&cumulated_padding_buffer);
}

static void btree_print_node(void* node,string_buffer* buffer,size_t level,pager* pager)
{
    static const char* padding = "    ";

    if (*node_get_type(node) == LEAF_NODE)
    {
        btree_print_leaf_node(node, buffer, padding, level);
    }
    else if (*node_get_type(node) == INTERNAL_NODE)
    {
        btree_print_internal_node(node, buffer, padding, level, pager);
    }
    else
    {
        fprintf(stderr, "Invalid node type encountered in `btree_print_node` call\n");
        exit(EXIT_FAILURE);
    }
}

static void internal_node_update_parent(void *internal_node,pager* pager)
{
    for (uint32_t i = 0; i < *internal_node_get_num_keys(internal_node) + 1; i++)
    {
        void *node = internal_node_pager_get_child(internal_node, i, pager);
        *node_get_parent(node) = pager_get_page_id(pager, internal_node);
    }
}

static void internal_node_split_and_insert_node(void *old_internal_node, void *node_to_insert,table* table)
{
    static const int64_t IS_NODE_TO_INSERT = -1;

    void* new_internal_node_1 = Malloc(PAGE_SIZE); // replace old_internal_node : imagine an assignment operation
    void* new_internal_node_2 = pager_get_free_page(table->pager);

    internal_node_init(new_internal_node_1,*node_get_parent(old_internal_node));
    internal_node_init(new_internal_node_2, *node_get_parent(old_internal_node));

    uint32_t new_child_index = internal_node_child_index_lower_bound(old_internal_node,node_get_node_key(node_to_insert,table->pager));

    //is root? : 2 new nodes added : 2nd split and root
    if(*node_get_is_root(old_internal_node))
    {
        *node_get_is_root(new_internal_node_1) = true;
        old_internal_node = pager_get_free_page(table->pager);
        
        memcpy(old_internal_node,pager_get_valid_page_ensure(table->pager,table->root_page_index),PAGE_SIZE);
        internal_node_update_parent(old_internal_node,table->pager);
    }
    
    //move children and keys
    for(uint32_t i = 0;i<INTERNAL_NODE_MAX_NUM_KEYS + 1;i++)
    {
        void* destination_node = (i<=INTERNAL_NODE_LEFT_SPLIT_COUNT?new_internal_node_1:new_internal_node_2);
        int64_t source_i = (i==new_child_index?IS_NODE_TO_INSERT:(i<new_child_index?i:i-1));
        int64_t dest_i = (i<=INTERNAL_NODE_LEFT_SPLIT_COUNT?i:i-INTERNAL_NODE_LEFT_SPLIT_COUNT-1);
        
        if(source_i != IS_NODE_TO_INSERT)
        {
            *internal_node_get_child(destination_node,dest_i) = *internal_node_get_child(old_internal_node,source_i);
            *internal_node_get_key(destination_node,dest_i) = *internal_node_get_key(old_internal_node,source_i);
        }
        else
        {
            if(i==INTERNAL_NODE_MAX_NUM_KEYS)
            {
                int64_t new_dest_i;
                void* right_child = internal_node_pager_get_right_child(old_internal_node,table->pager);

                if(node_get_node_key(node_to_insert,table->pager) < node_get_node_key(right_child,table->pager))
                {
                    new_dest_i = dest_i + 1;
                }
                else
                {
                    new_dest_i = dest_i;
                    dest_i = dest_i + 1;
                }

                *internal_node_get_child(new_internal_node_2,new_dest_i) = pager_get_page_id(table->pager,right_child);
                *internal_node_get_key(new_internal_node_2, new_dest_i) = node_get_node_key(right_child, table->pager);
                *node_get_parent(right_child) = pager_get_page_id(table->pager, new_internal_node_2);
            }
            *internal_node_get_child(destination_node,dest_i) = pager_get_page_id(table->pager,node_to_insert);
            *internal_node_get_key(destination_node,dest_i) = node_get_node_key(node_to_insert,table->pager);
            *node_get_parent(node_to_insert) = (destination_node == new_internal_node_1?pager_get_page_id(table->pager,old_internal_node):pager_get_page_id(table->pager,new_internal_node_2));        
        }
    }

    *internal_node_get_num_keys(new_internal_node_1) = INTERNAL_NODE_LEFT_SPLIT_COUNT;
    *internal_node_get_num_keys(new_internal_node_2) = INTERNAL_NODE_RIGHT_SPLIT_COUNT;

    //update parent of second split nodes
    internal_node_update_parent(new_internal_node_2,table->pager);

    memcpy(old_internal_node, new_internal_node_1, PAGE_SIZE);
    DESTROY(new_internal_node_1);

    //edge case: old_internal_node was the root node - we find a reference to the root node using the table
    if(*node_get_is_root(old_internal_node))
    {
        *node_get_is_root(old_internal_node) = false;
        internal_node_root_init(pager_get_valid_page_ensure(table->pager,table->root_page_index),old_internal_node,new_internal_node_2,table);
    }
    else
    {
        internal_node_insert_node(pager_get_valid_page_ensure(table->pager,*node_get_parent(new_internal_node_2)),new_internal_node_2,table);
    }

}

/*** ------------------------------------------------------------- ***/

string_buffer btree_print_tree(void* root_node,pager* pager)
{
    string_buffer btree;

    string_buffer_init(&btree);

    btree_print_node(root_node,&btree,0,pager);

    return btree;
}

string_buffer btree_get_diagnostics()
{
    string_buffer buf;

    string_buffer_init(&buf);

    string_buffer_append2(&buf, "page size is : %d\n", PAGE_SIZE);
    string_buffer_append2(&buf, "maximum number of pages is : %d\n", MAX_PAGE_NO);

    string_buffer_append2(&buf, "----------\n");
    
    string_buffer_append2(&buf, "common header size is : %ld\n", COMMON_HEADER_SIZE);
    string_buffer_append2(&buf, "leaf node header size is : %ld\n", LEAF_NODE_HEADER_SIZE);
    string_buffer_append2(&buf, "leaf node body size is : %ld\n", LEAF_NODE_BODY_SIZE);
    string_buffer_append2(&buf, "leaf node record size is : %ld\n", LEAF_NODE_RECORD_SIZE);
    string_buffer_append2(&buf, "\tleaf node key size is : %ld\n", LEAF_NODE_KEY_SIZE);
    string_buffer_append2(&buf, "\tleaf node row size is : %ld\n", ROW_SIZE);
    string_buffer_append2(&buf, "leaf node body max number of record is : %ld\n", LEAF_NODE_MAX_NUM_RECORDS);

    string_buffer_append2(&buf, "----------\n");

    string_buffer_append2(&buf, "internal node header size is : %ld\n", INTERNAL_NODE_HEADER_SIZE);
    string_buffer_append2(&buf, "internal node body size is : %ld\n", INTERNAL_NODE_BODY_SIZE);
    string_buffer_append2(&buf, "internal node maximum number of keys is : %ld\n", INTERNAL_NODE_MAX_NUM_KEYS);

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

void internal_node_insert_node(void *internal_node, void *node_to_insert,table* table)
{
    // scan keys: lower bound
    uint32_t child_index = internal_node_child_index_lower_bound(internal_node, node_get_node_key(node_to_insert,table->pager));

    uint32_t node_page_index = pager_get_page_id(table->pager,node_to_insert);

    if(*internal_node_get_num_keys(internal_node)==INTERNAL_NODE_MAX_NUM_KEYS)
    {
        return internal_node_split_and_insert_node(internal_node,node_to_insert,table);
    }

    //edge case: node_to_insert is to be ordered after internal_node's right child
    if (node_get_node_key(node_to_insert, table->pager) > node_get_node_key(internal_node_pager_get_right_child(internal_node, table->pager),table->pager))
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
        *internal_node_get_key(internal_node,i) = node_get_node_key(internal_node_pager_get_child(internal_node,i,table->pager),table->pager);
    }

    *node_get_parent(node_to_insert) = pager_get_page_id(table->pager,internal_node);
}

void *internal_node_find_node(void *internal_node, uint32_t key, pager *pager)
{
    uint32_t child_index = internal_node_child_index_lower_bound(internal_node,key);

    return internal_node_pager_get_child(internal_node,child_index,pager);
}

