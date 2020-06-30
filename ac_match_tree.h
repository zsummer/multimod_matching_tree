
/*

multimod_matching_tree License
-----------

multimod_matching_tree is licensed under the terms of the MIT license reproduced below.
This means that multimod_matching_tree is free software and can be used for both academic
and commercial purposes at absolutely no cost.


===============================================================================

Copyright (C) 2014-2016 YaweiZhang <yawei.zhang@foxmail.com>.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

===============================================================================

(end of COPYRIGHT)


*/
#ifndef _MULTI_MOD_MATCHING_TREE_H_
#define _MULTI_MOD_MATCHING_TREE_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct match_tree_node 
{
    unsigned char is_used_;
    unsigned char is_boundary_;
    struct match_tree_node * child_tree_; //match_tree_node[256]
};

struct match_tree_head
{
    struct match_tree_node * tree_;
    unsigned int tree_memory_used_;
    unsigned int tree_node_total_count_;
    unsigned int tree_node_used_count_;
    unsigned int tree_pattern_count_;
    unsigned int tree_pattern_maximum_len_;
    unsigned int tree_pattern_minimum_len_;
};

#if __GNUC__
#define MMT_API static __attribute((unused))
#else
#define MMT_API static
#endif

//build one multi-pattern match tree
MMT_API struct match_tree_head * match_tree_init();
//add one pattern to the tree.
MMT_API int match_tree_add_pattern(struct match_tree_head * head, const char * pattern, unsigned int pattern_len);
//build and add patterns from file.
MMT_API struct match_tree_head * match_tree_init_from_file(const char * file_name, const char* delimiter, unsigned int delimiter_len);
//matching pattern from text or biniry stream.
MMT_API unsigned int match_tree_matching(const struct match_tree_head *head, const char * text, unsigned int text_len, unsigned char is_greedy);
//translate character when the character is matched.
MMT_API void match_tree_translate(const struct match_tree_head *head, char * text, unsigned int text_len, unsigned char is_greedy, char escape);
//free one multi-pattern match tree
MMT_API void match_tree_free(struct match_tree_head *head);


//--------------------------------------
//impliment
//--------------------------------------

MMT_API struct match_tree_head * match_tree_init()
{
    struct match_tree_head * head = (struct match_tree_head *)malloc(sizeof(struct match_tree_head));
    memset(head, 0, sizeof(struct match_tree_head));
    return head;
}

MMT_API int match_tree_add_pattern(struct match_tree_head * head, const char * pattern, unsigned int pattern_len)
{
    struct match_tree_node **tree = NULL;
    unsigned int i = 0;
    unsigned char node = 0;

    if (head == NULL)
    {
        return -1;
    }
    tree = &head->tree_;
    if (pattern_len == 0)
    {
        return 0;
    }
    for (i = 0; i < pattern_len; i++)
    {
        //check  is current node had all childs. if not then malloc it.
        if (*tree == NULL)
        {
            *tree = (struct match_tree_node *)malloc(sizeof(struct match_tree_node) * 256);
            memset(*tree, 0, sizeof(struct match_tree_node) * 256);
            head->tree_node_total_count_ += 256;
            head->tree_memory_used_ += sizeof(struct match_tree_node) * 256;
        }
        
        node = (unsigned char)pattern[i];

        //set node valid, default is invalid
        if (!(*tree)[node].is_used_)
        {
            (*tree)[node].is_used_ = 1;
            head->tree_node_used_count_++;
        }

        //check is the boundary, if true then set bound.
        if (i == pattern_len - 1 && !(*tree)[node].is_boundary_)
        {
            (*tree)[node].is_boundary_ = 1;
            head->tree_pattern_count_++;

            if (pattern_len > head->tree_pattern_maximum_len_)
            {
                head->tree_pattern_maximum_len_ = pattern_len;
            }
            if (pattern_len < head->tree_pattern_minimum_len_ || head->tree_pattern_minimum_len_ == 0)
            {
                head->tree_pattern_minimum_len_ = pattern_len;
            }
        }
        tree = &(*tree)[node].child_tree_;
    }
    return 0;
}

MMT_API struct match_tree_head * match_tree_init_from_file(const char * file_name, const char * delimiter, unsigned int delimiter_len)
{
    FILE * fp = NULL;
    char * file_content = NULL;

    unsigned int file_content_len = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int is_delimiter = 0;
    char * pattern = NULL;
    unsigned int pattern_len = 0;

    struct match_tree_head *head = NULL;
    if (delimiter == NULL || delimiter_len == 0)
    {
        return NULL;
    }
    

#ifdef WIN32
    if (fopen_s(&fp, file_name, "rb") != 0)
    {
        return NULL;
    }
#else
    fp = fopen(file_name, "rb");
    if (!fp)
    {
        return NULL;
    }
#endif

    fseek(fp, 0, SEEK_END);
    file_content_len = ftell(fp);
    file_content = (char*)malloc(file_content_len + delimiter_len+ 1);
    fseek(fp, 0, SEEK_SET);
    file_content_len = (unsigned int)fread(file_content, 1, file_content_len, fp);
    fclose(fp);
    fp = NULL;
    memcpy(file_content + file_content_len, delimiter, delimiter_len);
    file_content[file_content_len + delimiter_len] = '\0';
    file_content_len += delimiter_len;
    head = match_tree_init();
    //printf("%s:%d", file_content, file_content_len);
    pattern = file_content;
    i = 0;
    while ( i + delimiter_len <= file_content_len)
    {
        is_delimiter = 1;
        for (j = 0; j < delimiter_len; j++)
        {
            if (file_content[i+j] != delimiter[j])
            {
                is_delimiter = 0;
                break;
            }
        }

        if (is_delimiter)
        {
            pattern_len = (unsigned int)(file_content + i - pattern);
            match_tree_add_pattern(head, pattern, pattern_len);
            i = i + delimiter_len;
            pattern = file_content + i;
        }
        else
        {
            i++;
        }
    }

    free(file_content);
    file_content = NULL;
    file_content_len = 0;
    return head;
}

MMT_API unsigned int match_tree_matching(const struct match_tree_head *head, const char * text, unsigned int text_len, unsigned char is_greedy)
{
    unsigned int ret_len = 0;
    unsigned int i = 0;
    unsigned char node = 0;
    const struct match_tree_node * tree = NULL;
    if (text == NULL || text_len == 0 || head == NULL)
    {
        return ret_len;
    }
    tree = head->tree_;
    for (i = 0; i < text_len; ++i)
    {
        node = (unsigned char)text[i];
        if (tree == NULL || !tree[node].is_used_)
        {
            break;
        }
        
        if (tree[node].is_boundary_)
        {
            ret_len = i+1;
            if (!is_greedy)
            {
                break;
            }
        }
        tree = tree[node].child_tree_;
    }
    return ret_len;
}

MMT_API void match_tree_translate(const struct match_tree_head *head, char * text, unsigned int text_len, unsigned char is_greedy, char escape)
{
    unsigned int matching_count = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    for (i = 0; i < text_len;)
    {
        matching_count = match_tree_matching(head, text + i, text_len - i, is_greedy);
        if (matching_count > 0)
        {
            for (j = i; j < i + matching_count; j++)
            {
                text[j] = escape;
            }
            i += matching_count;
        }
        else
        {
            i++;
        }
    }
}

MMT_API void match_tree_free_impl(struct match_tree_node *tree)
{
    unsigned int i = 0;
    if (tree->child_tree_)
    {
        for (i = 0; i < 256; i++)
        {
            if (tree->child_tree_[i].child_tree_)
            {
                match_tree_free_impl(&tree->child_tree_[i]);
            }
        }
        free(tree->child_tree_);
    }
}
MMT_API void match_tree_free(struct match_tree_head *head)
{ 
    if (head)
    { 
        if (head->tree_) 
        { 
            match_tree_free_impl(head->tree_); 
    	    free(head->tree_); 
    	} 
        free(head); 
    } 
} 





#endif




