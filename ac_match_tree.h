
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
typedef int32_t s32;
typedef uint32_t u32;
typedef uint8_t u8;


#ifdef USED_FN_LOG
#include "fn_log.h"
#else
#define LogDebug() std::cout
#define LogInfo() std::cout
#define LogError() std::cout
#endif // USED_FN_LOG



namespace zsummer
{
    namespace matching
    {

        constexpr static u32 CHILD_SIZE = 1U << (sizeof(u8) * 8);
        static_assert(CHILD_SIZE == 256, "");

        template<typename Ty>
        struct MatchNode
        {
            bool has_val_;
            char node_char_;
            s32 depth_;
            s32 childs_;
            MatchNode* parent_;
            MatchNode* next_;
            MatchNode* goto_node_; //失败后跳转到的目标node   
            s32 goto_content_forward_; //失败后跳转到目标node后, content开始指针滑动字符个数   
            s32 got_content_match_offset_;  //失败后跳转到目标node后, node所在的depth, 等同已匹配个数
            Ty val_;
            struct MatchNode* child_tree_[CHILD_SIZE];
            static_assert(std::is_arithmetic<Ty>::value, "");
        };

        template<typename Ty>
        struct MatchOffset
        {
            const char* begin_;
            const char* offset_;
            const char* end_;
            MatchNode<Ty>* node_;
        };

        template<typename Ty>
        struct MatchState
        {
            MatchOffset<Ty> offset_;
            std::vector<MatchOffset<Ty>> results_;
        };


        template<class Ty>
        class MatchTree
        {
        public:
            MatchNode<Ty> root_;
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
            s32 AddPattern(const char* pattern, u32 pattern_len, Ty val)
            {
                LogDebug() << "pattern<" << pattern_len << ">:[" << std::string(pattern, pattern_len) << "]";
                MatchNode<Ty>* node = &root_;
                const u8* offset = (u8*)pattern;
                const u8* end = offset + pattern_len;
                while (offset != end)
                {
                    MatchNode<Ty>*& child = node->child_tree_[*offset];
                    if (child == NULL)
                    {
                        child = new MatchNode<Ty>();
                        memset(child, 0, sizeof(MatchNode<Ty>));
                        node->childs_++;
                        child->next_ = root_.next_;
                        root_.next_ = child;
                        child->parent_ = node;
                        child->depth_ = node->depth_ + 1;
                        child->node_char_ = (char)*offset;
                        use_heap_size_ += sizeof(MatchNode<Ty>);
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

            s32 DestroyPatternTree()
            {
                MatchNode<Ty>* node = root_.next_;
                s32 count = 0;
                while (node != NULL)
                {
                    MatchNode<Ty>* front = node;
                    node = node->next_;
                    delete front;
                    count++;
                }
                if (count != node_count_)
                {
                    LogError() << "has memory leak. new node:" << node_count_ << ", destroy count:" << count;
                    return -1;
                }
                memset(this, 0, sizeof(MatchTree));
                return 0;
            }


            s32 MatchContentImpl(MatchState<Ty>& ms, bool enable_goto)
            {
                MatchOffset<Ty>& offset = ms.offset_;
                offset.node_ = &root_;
                MatchOffset<Ty> best_match;
                best_match.node_ = NULL;
                while (offset.offset_ < offset.end_)
                {
                    if (offset.node_->child_tree_[*(u8*)offset.offset_] != NULL)
                    {
                        offset.node_ = offset.node_->child_tree_[*(u8*)offset.offset_];
                        offset.offset_++;
                        if (offset.node_->has_val_)
                        {
                            best_match = offset;
                        }
                        //如果全文结束则进行后续处理, 按照下个字符匹配失败处理, 不能直接continue  
                        if (offset.offset_ != offset.end_)
                        {
                            continue;
                        }
                    }
                    if (best_match.node_ != NULL)
                    {
                        ms.results_.push_back(best_match);
                        offset.begin_ = best_match.offset_;
                        offset.offset_ = best_match.offset_;
                        offset.node_ = &root_;
                        best_match.node_ = NULL;
                        continue;
                    }
                    if (enable_goto)
                    {
                        offset.begin_ += offset.node_->goto_content_forward_;
                        offset.offset_ = offset.begin_ + offset.node_->got_content_match_offset_;
                        offset.node_ = offset.node_->goto_node_;
                        if (offset.offset_ == offset.end_ && offset.node_->has_val_ && offset.offset_ != offset.begin_)
                        {
                            ms.results_.push_back(offset);
                        }
                    }
                    else
                    {
                        offset.begin_ += 1;
                        offset.offset_ = offset.begin_;
                        offset.node_ = &root_;
                    }
                }
                return 0;
            }
            s32 MatchContent(MatchState<Ty>& ms)
            {
                return MatchContentImpl(ms, false);
            }
            s32 AcMatchContent(MatchState<Ty>& ms)
            {
                return MatchContentImpl(ms, true);
            }

            s32 MatchPath(MatchState<Ty>& ms)
            {
                ms.offset_.node_ = &root_;
                while (ms.offset_.offset_ != ms.offset_.end_)
                {
                    if (ms.offset_.node_->child_tree_[*(u8*)ms.offset_.offset_] == NULL)
                    {
                        return 0;
                    }
                    ms.offset_.node_ = ms.offset_.node_->child_tree_[*(u8*)ms.offset_.offset_];
                    ms.offset_.offset_++;
                }
                return 0;
            }

            MatchNode<Ty>* MatchPath(const char* content, size_t len)
            {
                MatchState<Ty> ms;
                ms.offset_.begin_ = content;
                ms.offset_.offset_ = content;
                ms.offset_.end_ = content + len;
                ms.offset_.node_ = &root_;
                MatchPath(ms);
                if (ms.offset_.node_ == &root_)
                {
                    return NULL;
                }
                return ms.offset_.node_;
            }


            s32 BuildBadState()
            {
                std::vector<MatchNode<Ty>*> nodes;
                nodes.push_back(&root_);
                for (size_t i = 0; i < nodes.size(); i++)
                {
                    for (size_t j = 0; j < CHILD_SIZE; j++)
                    {
                        if (nodes[i]->child_tree_[j] != NULL)
                        {
                            nodes.push_back(nodes[i]->child_tree_[j]);
                        }
                    }
                }

                std::string path;
                std::list<char> lpath;
                std::string revert_path;
                MatchState<Ty> state;
                root_.goto_node_ = &root_;
                root_.goto_content_forward_ = 1;
                root_.got_content_match_offset_ = 0;
                for (size_t i = 1; i < nodes.size(); i++)
                {
                    MatchNode<Ty>* node = nodes[i];
                    path.clear();
                    lpath.clear();
                    while (node != NULL)
                    {
                        lpath.push_front(node->node_char_);
                        node = node->parent_;
                    }
                    if (lpath.size() <= 1)
                    {
                        LogError() << "at least has root:'\0' and current node char val.";
                        return -1;
                    }
                    lpath.pop_front();  //pop root
                    node = nodes[i];
                    node->goto_content_forward_ = 1;
                    node->got_content_match_offset_ = 0;
                    node->goto_node_ = &root_;
                    lpath.pop_front();

                    while (!lpath.empty())
                    {
                        path.clear();
                        for (auto iter = lpath.begin(); iter != lpath.end(); ++iter )
                        {
                            path.push_back(*iter);
                        }
                        
                        state.offset_.begin_ = path.c_str();
                        state.offset_.offset_ = path.c_str();
                        state.offset_.node_ = &root_;
                        state.offset_.end_ = path.c_str() + path.length();
                        MatchPath(state);
                        if (state.offset_.begin_ != state.offset_.offset_)
                        {
                            node->goto_node_ = state.offset_.node_;
                            node->goto_content_forward_ += 0;
                            node->got_content_match_offset_ = (s32)(state.offset_.offset_ - state.offset_.begin_);
                            break;
                        }
                        lpath.pop_front();
                        node->goto_content_forward_ += 1;
                    }
                }
                return 0;
            }

            std::string ReadFile(const std::string& file_name)
            {
                FILE* fp = NULL;
                char buff[1024];

#ifdef WIN32
                if (fopen_s(&fp, file_name.c_str(), "rb") != 0)
                {
                    LogError() << "open file:[" << file_name << "] has error.";
                    return "";
                }
#else
                fp = fopen(file_name.c_str(), "rb");
                if (!fp)
                {
                    LogError() << "open file:[" << file_name << "] has error.";
                    return "";
                }
#endif
                fseek(fp, 0, SEEK_END);
                long content_len = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                size_t read_len = 0;
                std::string result;
                do
                {
                    read_len = fread(buff, 1, sizeof(buff), fp);
                    if (read_len > 0)
                    {
                        result.append(buff, read_len);
                    }

                } while (read_len == sizeof(buff));
                fclose(fp);
                if (result.size() != (size_t)content_len)
                {
                    LogError() << "check file size:" << content_len << ", but read size:" << result.size() << "  not equal.";
                    return result;
                }
                return result;
            }

            s32 AddPatternFromString(const char* content, char ch)
            {
                const char* begin = content;
                const char* offset = content;
                bool jump_table[256];
                memset(jump_table, 0, sizeof(jump_table));
                jump_table[(u8)' '] = true;
                jump_table[(u8)ch] = true;
                jump_table[(u8)'\r'] = true;
                jump_table[(u8)'\n'] = true;
                while (*begin != '\0')
                {
                    jump_table[0] = false;
                    while (jump_table[(u8)*begin])
                    {
                        begin++;
                    }
                    if (*begin == '\0')
                    {
                        break;
                    }
                    offset = begin;
                    jump_table[0] = true;
                    while (!jump_table[(u8)*offset])
                    {
                        offset++;
                    }
                    s32 ret = AddPattern(begin, (u32)(offset - begin), 0);
                    if (ret != 0)
                    {
                        LogError() << "add pattern error:" << std::string(begin, offset - begin);
                        return -1;
                    }
                    begin = offset;
                }
                return 0;

            }


        };





    }
}


#endif




