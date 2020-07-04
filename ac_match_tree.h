
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
            s32 depth_; //root == 0  
            s32 childs_;
            MatchNode* parent_;
            MatchNode* next_alloc_;
            MatchNode* up_matched_; //最大匹配
            MatchNode* goto_node_;  //失败后跳转到的目标node   
            s32 goto_content_forward_; //失败后跳转到目标node后, content开始指针滑动字符个数   
            s32 goto_locked_path_;      //发生失配后锁定第一个匹配位
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
            using Node = MatchNode<Ty>;
            using Offset = MatchOffset<Ty>;
            using State = MatchState<Ty>;
            Node root_;
            s32 use_heap_size_;
            s32 node_count_;
            s32 pattern_count_;
            s32 pattern_max_len_;
            s32 pattern_min_len_;
        public:
            MatchTree()
            {
                memset(this, 0, sizeof(MatchTree));
            }
            s32 AddPattern(const char* pattern, s32 pattern_len, Ty val);

            s32 DestroyPatternTree();


            s32 MatchContentImpl(State& state, std::function<void(Offset& offset)> callback, bool enable_goto);
            s32 MatchContent(State& state){return MatchContentImpl(state, NULL, false);}
            s32 AcMatchContent(State& state){return MatchContentImpl(state, NULL, true);}

            s32 MatchPath(State& state);

            Node* MatchPath(const char* content, size_t len);

            s32 BuildUpMatchedRecursive(Node*);

            s32 BuildGotoStateRecursive(Node*, std::string& path);
            s32 BuildGotoStateRecursive();


            static std::string ReadFile(const std::string& file_name);

            s32 AddPatternFromString(const char* content, char ch);


        };

        template<class Ty>
        s32 MatchTree<Ty>::AddPattern(const char* pattern, s32 pattern_len, Ty val)
        {
            //LogDebug() << "pattern<" << pattern_len << ">:[" << std::string(pattern, pattern_len) << "]";
            Node* node = &root_;
            const u8* offset = (u8*)pattern;
            const u8* end = offset + pattern_len;
            while (offset != end)
            {
                Node*& child = node->child_tree_[*offset];
                if (child == NULL)
                {
                    child = new Node();
                    memset(child, 0, sizeof(Node));
                    node->childs_++;
                    child->next_alloc_ = root_.next_alloc_;
                    root_.next_alloc_ = child;
                    child->parent_ = node;
                    child->depth_ = node->depth_ + 1;
                    child->node_char_ = (char)*offset;
                    use_heap_size_ += sizeof(Node);
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
        template<class Ty>
        s32 MatchTree<Ty>::DestroyPatternTree()
        {
            Node* node = root_.next_alloc_;
            s32 count = 0;
            while (node != NULL)
            {
                Node* front = node;
                node = node->next_alloc_;
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

        template<class Ty>
        s32 MatchTree<Ty>::MatchContentImpl(State& state, std::function<void(Offset& offset)> callback, bool enable_goto)
        {
            Offset& offset = state.offset_;
            offset.node_ = &root_;
            Offset best_match;
            best_match.node_ = NULL;
            do
            {
                if (offset.node_->has_val_)
                {
                    best_match = offset;
                }
                if (offset.offset_ < offset.end_)
                {
                    if (offset.node_->child_tree_[*(u8*)offset.offset_] != NULL)
                    {
                        offset.node_ = offset.node_->child_tree_[*(u8*)offset.offset_];
                        offset.offset_++;
                        continue;
                    }
                }
                if (offset.node_->up_matched_ != NULL)
                {
                    if (best_match.node_ != NULL)
                    {
                        if (offset.node_->up_matched_ != best_match.node_)
                        {
                            LogError() << "error";
                        }
                    }
                    best_match = offset;
                    best_match.node_ = offset.node_->up_matched_;
                    best_match.offset_ = best_match.begin_+ best_match.node_->depth_;
                }
                
                if (best_match.node_ != NULL)
                {
                    if (callback)
                    {
                        callback(best_match);
                    }
                    else
                    {
                        state.results_.push_back(best_match);
                    }
                    offset.begin_ = best_match.offset_;
                    offset.offset_ = offset.begin_;
                    offset.node_ = &root_;
                    best_match.node_ = NULL;
                    continue;
                }

                if (enable_goto)
                {
                    offset.begin_ += offset.node_->goto_content_forward_;
                    offset.offset_ = offset.begin_ + offset.node_->goto_node_->depth_;
                    offset.node_ = offset.node_->goto_node_;
                }
                else
                {
                    offset.begin_ += 1;
                    offset.offset_ = offset.begin_;
                    offset.node_ = &root_;
                }

            } while (offset.offset_ != offset.begin_ || offset.offset_ < offset.end_);
            return 0;
        }

        template<class Ty>
        s32 MatchTree<Ty>::MatchPath(State& state)
        {
            state.offset_.node_ = &root_;
            while (state.offset_.offset_ != state.offset_.end_)
            {
                if (state.offset_.node_->child_tree_[*(u8*)state.offset_.offset_] == NULL)
                {
                    return 0;
                }
                state.offset_.node_ = state.offset_.node_->child_tree_[*(u8*)state.offset_.offset_];
                state.offset_.offset_++;
            }
            return 0;
        }
        template<class Ty>
        MatchNode<Ty>*  MatchTree<Ty>::MatchPath(const char* content, size_t len)
        {
            State state;
            state.offset_.begin_ = content;
            state.offset_.offset_ = content;
            state.offset_.end_ = content + len;
            state.offset_.node_ = &root_;
            MatchPath(state);
            if (state.offset_.node_ == &root_)
            {
                return NULL;
            }
            return state.offset_.node_;
        }

        template<class Ty>
        s32 MatchTree<Ty>::BuildUpMatchedRecursive(Node* node)
        {
            if (node->has_val_)
            {
                node->up_matched_ = node;
            }
            else
            {
                node->up_matched_ = (node->parent_) ? node->parent_->up_matched_ : NULL;
            }
            for (size_t i = 0; i < CHILD_SIZE; i++)
            {
                if (node->child_tree_[i] != NULL)
                {
                    s32 ret = BuildUpMatchedRecursive(node->child_tree_[i]);
                    if (ret != 0)
                    {
                        LogError() << "has error";
                        return ret;
                    }
                }
            }
            return 0;
        }

        template<class Ty>
        s32 MatchTree<Ty>::BuildGotoStateRecursive(Node*node, std::string& path)
        {
            if (node->depth_ > 0)
            {
                node->goto_content_forward_ = node->parent_->goto_content_forward_;
                node->goto_node_ = node->parent_->goto_node_;
                node->goto_locked_path_ = node->parent_->goto_locked_path_;
            }

            //root & root->child  is direct pointer to root_ and forward offset 1.  
            if (node->depth_ > 1)
            {
                if (!node->goto_locked_path_)
                {
                    //has match   
                    if (node->goto_node_->child_tree_[(u8)node->node_char_] != NULL)
                    {
                        node->goto_node_ = node->goto_node_->child_tree_[(u8)node->node_char_];
                    }
                    //first mis match
                    else if (node->goto_node_ == &root_) //向下滑动
                    {
                        node->goto_content_forward_++;
                    }
                    else //部分匹配后失配
                    {
                        //有完整匹配则锁定
                        if (node->goto_node_->up_matched_)
                        {
                            node->goto_locked_path_ = true;
                        }
                        //无完整匹配则滑动一次进行查找
                        else
                        {
                            while (++node->goto_content_forward_ != node->depth_)
                            {
                                State state;
                                state.offset_.begin_ = path.c_str() + node->goto_content_forward_;
                                state.offset_.offset_ = state.offset_.begin_;
                                state.offset_.end_ = path.c_str() + path.length();
                                state.offset_.node_ = &root_;
                                s32 ret = MatchPath(state);
                                if (ret != 0)
                                {
                                    LogError() << "match path error";
                                    node->goto_locked_path_ = true;
                                    node->goto_content_forward_ = 1;
                                    node->goto_node_ = &root_;
                                    break;
                                }
                                //出现超过当前节点的匹配则从该位置继续匹配
                                if (state.offset_.node_->depth_ == node->depth_ - node->goto_content_forward_)
                                {
                                    node->goto_node_ = state.offset_.node_;
                                    break;
                                }
                                //新位置仍然失配 但是存在完整匹配 锁定 
                                if (state.offset_.node_->up_matched_)
                                {
                                    node->goto_node_ = state.offset_.node_->up_matched_;
                                    node->goto_locked_path_ = true;
                                    break;
                                }
                                //继续下滑 
                            }
                            if (node->goto_content_forward_ == node->depth_)
                            {
                                node->goto_node_ = &root_;
                            }
                        }
                    }
                }
            }

            for (size_t i = 0; i < CHILD_SIZE; i++)
            {
                if (node->child_tree_[i] != NULL)
                {
                    path.push_back(node->child_tree_[i]->node_char_);
                    s32 ret = BuildGotoStateRecursive(node->child_tree_[i], path);
                    path.pop_back();
                    if (ret != 0)
                    {
                        LogError() << "has error";
                        return ret;
                    }
                }
            }
            return 0;
        }

        template<class Ty>
        s32 MatchTree<Ty>::BuildGotoStateRecursive()
        {
            root_.goto_content_forward_ = 1;
            root_.goto_locked_path_ = false;
            root_.goto_node_ = &root_;
            root_.goto_locked_path_ = false;
            root_.up_matched_ = NULL;
            root_.parent_ = NULL;
            s32 ret = BuildUpMatchedRecursive(&root_);
            std::string path;
            ret |= BuildGotoStateRecursive(&root_, path);
            return ret;
        }
        
        template<class Ty>
        std::string MatchTree<Ty>::ReadFile(const std::string& file_name)
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
        template<class Ty>
        s32 MatchTree<Ty>::AddPatternFromString(const char* content, char ch)
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
                s32 ret = AddPattern(begin, (s32)(offset - begin), 0);
                if (ret != 0)
                {
                    LogError() << "add pattern error:" << std::string(begin, offset - begin);
                    return -1;
                }
                begin = offset;
            }
            LogDebug() << "now patterns:" << pattern_count_;
            return 0;
        }



    }
}


#endif




