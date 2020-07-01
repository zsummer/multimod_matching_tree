
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
typedef uint32_t u32;
typedef uint8_t u8;








template<class Ty>
class MatchTree
{
public:
    constexpr static u32 NEXT_NODE = 0;
    constexpr static u32 PARENT_NODE = 1;
    constexpr static u32 BAD_JUMP_NODE = 2;
    constexpr static u32 BAD_JUMP_FORWARD_OFFSET = 3;
    constexpr static u32 BAD_JUMP_OFFSET_OFFSET = 4;
    constexpr static u32 CHILD_SIZE = 1U << (sizeof(u8) * 8);
    static_assert(CHILD_SIZE == 256, "");

    struct MatchNode
    {
        bool has_val_;
        Ty val_;
        struct MatchNode* child_tree_[CHILD_SIZE];
        static_assert(std::is_arithmetic<Ty>::value, "");
    };
    struct MatchState
    {
        const char* begin_;
        const char* offset_;
        const char* end_;
        MatchNode* cur_;
    };

public:
    MatchNode root_;
    unsigned int use_heap_size_;
    unsigned int node_count_;
    unsigned int pattern_count_;
    unsigned int pattern_max_len_;
    unsigned int pattern_min_len_;
public:
    MatchTree()
    {
        memset(this, 0, sizeof(MatchTree));
    }
    int AddPattern(const char* pattern, u32 pattern_len, Ty val)
    {
        MatchNode* node = &root_;
        const u8* offset = (u8*)pattern;
        const u8* end = offset + pattern_len;
        while (offset != end)
        {
            MatchNode*& child = node->child_tree_[*offset];
            if (child == NULL)
            {
                child = new MatchNode();
                memset(child, 0, sizeof(MatchNode));
                child->child_tree_[NEXT_NODE] = root_->child_tree_[NEXT_NODE];
                root_->child_tree_[NEXT_NODE] = child;
                child->child_tree_[PARENT_NODE] = &root_;
                if (child->child_tree_[NEXT_NODE] != NULL)
                {
                    child->child_tree_[NEXT_NODE]->child_tree_[PARENT_NODE] = child;
                }
                use_heap_size_ += sizeof(MatchNode);
                node_count_++;
            }
            offset++;
            node = child;
        }
        if (node != &root_)
        {
            node->has_val_ = true;
            node->val_ = val;
            pattern_count_++;
            pattern_min_len_ = pattern_min_len_ == 0 ? pattern_len : pattern_min_len_;
            pattern_min_len_ = pattern_min_len_ <= pattern_len ? pattern_min_len_ : pattern_len;
            pattern_max_len_ = pattern_max_len_ > pattern_len ? pattern_max_len_ : pattern_len;
        }
        return 0;
    }

    u32 DestroyPatternTree()
    {
        MatchNode* node = root_->child_tree_[0];
        u32 count = 0;
        while (node != NULL)
        {
            MatchNode* front = node;
            node = node->child_tree_[0];
            delete front;
            count++;
        }
        memset(this, 0, sizeof(MatchTree));
        return count;
    }

    u32 MatchMaxLen(MatchState& ms)
    {
        MatchNode* cur = &root_;
        ms.cur_ = cur;
        while (cur->child_tree_[*ms.offset_] && ms.offset_ != ms.end_)
        {
            ms.cur_ = cur;
            ms.offset_++;
        }
        return ms.offset_ - ms.begin_;
    }

    u32 BuildBadState(MatchState& ms)
    {
        for (size_t i = 0; i < CHILD_SIZE; i++)
        {
            if (ms.cur_->child_tree_[*ms.offset_] == NULL)
            {

            }
        }
        return 0;
    }

    u32 BuildBadState()
    {

    }
    




};






#endif




