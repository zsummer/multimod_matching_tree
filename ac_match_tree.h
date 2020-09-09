
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
typedef uint16_t u16;

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


        /*
        * 前缀树的节点结构   
        */
        template<typename Ty>
        struct MatchNode
        {
            bool has_val_;
            u8 node_char_;
            s32 seq_id_;
            s32 depth_; 
            s32 childs_;
            MatchNode* parent_;
            MatchNode* next_alloc_;
            MatchNode* up_matched_; //最大匹配   
            MatchNode* goto_node_;  //失败后跳转到的目标node     
            s32 goto_content_forward_; //失败后跳转到目标node后, content开始指针滑动字符个数      
            s32 goto_locked_path_;      //发生失配后锁定第一个匹配位   
            Ty val_;
            struct MatchNode* child_tree_[CHILD_SIZE];
            void reset()
            {
                has_val_ = false;
                node_char_ = 0;
                depth_ = 0;
                childs_ = 0;
                parent_ = NULL;
                next_alloc_ = NULL;
                up_matched_ = NULL;
                goto_node_ = NULL;
                goto_content_forward_ = 0;
                goto_locked_path_ = 0;
                memset(child_tree_, 0, sizeof(child_tree_));
                static_assert(sizeof(child_tree_) / sizeof(void*) == CHILD_SIZE, "");
                static_assert((1ULL << (sizeof(node_char_)*8)) == CHILD_SIZE, "");
            }
        };

        template<typename Ty>
        struct CharState
        {
            u8 node_char_;
            u8 child_count_;
            u16 child_begin_id_;  
            u16 text_offset_;  
            u16 failed_goto_id_;
            u16 failed_text_offset_;
            u16 up_closure_id_;  
            u8 has_val_;
            Ty val_;
        };


        template<typename Ty>
        struct ZipStateMachine
        {
            u32 node_size_;
            CharState<Ty> nodes_[1]; //root
        };



        /*
        * 前缀树的节点游标, 记录当前匹配过程中对应的文本范围以及匹配到的节点  
        * 有效的节点游标中,  begin记录的是第一个匹配的字符, offset是匹配子串结束位置:  
        * * offset - begin  == node_.depth    
        * * *(begin -1) == node_.node_char  
        */
        template<typename Ty>
        struct MatchOffset
        {
            const char* begin_;
            const char* offset_;
            const char* end_;
            MatchNode<Ty>* node_;
        };

        //匹配状态信息   
        //对一个长串文本进行连续匹配,并返回所有匹配结果用   
        //如果用callback的形式则出现匹配时直接callback 并且不会填充results数组   
        template<typename Ty>
        struct MatchState
        {
            MatchOffset<Ty> offset_;
            std::vector<MatchOffset<Ty>> results_;
        };


        //前缀树提供朴素的前缀树以及AC自动机两套接口  
        //AC自动机算法需要再添加完所有pattern后build一次gostate信息    
        //普通自动机算法可以随时添加pattern不需要build  
        template<class Ty>
        class MatchTree
        {
        public:
            using Node = MatchNode<Ty>;
            using Offset = MatchOffset<Ty>;
            using State = MatchState<Ty>;
            using ToString = std::function<std::string(const Ty&)>;
            Node root_;
            s32 use_heap_size_;
            s32 node_count_;
            s32 seq_count_;
            s32 pattern_count_;
            s32 pattern_max_len_;
            s32 pattern_min_len_;
            ZipStateMachine<Ty> *zip_states_;
        public:
            void reset()
            {
                root_.reset();
                use_heap_size_ = 0;
                node_count_ = 0;
                pattern_count_ = 0;
                pattern_max_len_ = 0;
                pattern_min_len_ = 0;
                seq_count_ = 0;
                zip_states_ = NULL;
            }
            MatchTree()
            {
                reset();
            }
            s32 AddPattern(const char* pattern, s32 pattern_len, Ty val);

            s32 DestroyPatternTree();

            s32 MatchContent(State& state, std::function<void(Offset& offset)> callback =NULL);
            s32 AcMatchContent(State& state, std::function<void(Offset& offset)> callback = NULL);
            s32 AcZipMatchContent(State& state, std::function<void(Offset& offset)> callback = NULL);
            std::string ReplaceContentImpl(const char* content, size_t lenth, bool symbo_bound, bool use_ac, ToString to_string);
            std::string ReplaceContent(const char* content, size_t lenth, ToString to_string);
            std::string AcReplaceContent(const char* content, size_t lenth, ToString to_string);
            s32 MatchPath(State& state);

            Node* MatchPath(const char* content, size_t len);

            s32 BuildUpMatchedRecursive(Node*);

            s32 BuildGotoStateRecursive(Node*, std::string& path);
            s32 BuildGotoStateRecursive();

            s32 BuildZipState(s32& cur_index, Node* node);
            s32 BuildZipState();


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
                    child->reset();
                    node->childs_++;
                    child->next_alloc_ = root_.next_alloc_;
                    root_.next_alloc_ = child;
                    child->parent_ = node;
                    child->depth_ = node->depth_ + 1;
                    child->node_char_ = *offset;
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
            if (zip_states_ != NULL)
            {
                delete[](char*)zip_states_;
                zip_states_ = NULL;
            }
            reset();
            return 0;
        }

        template<class Ty>
        s32 MatchTree<Ty>::MatchContent(State& state, std::function<void(Offset& offset)> callback)
        {
            Offset& offset = state.offset_;
            offset.node_ = &root_;
            Offset best_match;
            best_match.node_ = NULL;
            do
            {
                if (offset.offset_ < offset.end_)
                {
                    if (offset.node_->child_tree_[*(u8*)offset.offset_] != NULL)
                    {
                        offset.node_ = offset.node_->child_tree_[*(u8*)offset.offset_];
                        offset.offset_++;
                        if (offset.node_->has_val_)
                        {
                            best_match = offset;
                        }
                        continue;
                    }
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

                offset.begin_ += 1;
                offset.offset_ = offset.begin_;
                offset.node_ = &root_;

            } while (offset.offset_ != offset.begin_ || offset.offset_ < offset.end_);
            return 0;
        }
        template<class Ty>
        s32 MatchTree<Ty>::AcMatchContent(State& state, std::function<void(Offset& offset)> callback)
        {
            Offset& offset = state.offset_;
            offset.node_ = &root_;
            Offset best_match;
            do
            {
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
                    best_match = offset;
                    best_match.node_ = offset.node_->up_matched_;
                    best_match.offset_ = best_match.begin_ + best_match.node_->depth_;
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

 
                offset.begin_ += offset.node_->goto_content_forward_;
                offset.offset_ = offset.begin_ + offset.node_->goto_node_->depth_;
                offset.node_ = offset.node_->goto_node_;

            } while (offset.offset_ != offset.begin_ || offset.offset_ < offset.end_);
            return 0;
        }

        template<class Ty>
        s32 MatchTree<Ty>::AcZipMatchContent(State& state, std::function<void(Offset& offset)> callback)
        {
            Offset& offset = state.offset_;
            s32 index = 0;
            while (offset.offset_ < offset.end_)
            {
                CharState<Ty>& node = zip_states_->nodes_[index];
                bool has_match = false;
                if (true)
                {
                    s32 count = node.child_count_;
                    s32 step;
                    s32 child_index = node.child_begin_id_;
                    while (count > 0)
                    {
                        step = count / 2;
                        if ((u8)*offset.offset_ < zip_states_->nodes_[child_index + step].node_char_)
                        {
                            count = step;
                        }
                        else 
                        {
                            child_index += step+1;
                            count -= step+1;
                        }
                    }
                    if (child_index < node.child_begin_id_ + node.child_count_)
                    {
                        index = child_index;
                        offset.offset_++;
                        has_match = true;
                    }
                }
                if (false)
                {
                    for (s32 i = node.child_begin_id_; i < node.child_begin_id_ + node.child_count_; i++)
                    {
                        CharState<Ty>& child = zip_states_->nodes_[i];
                        if (child.node_char_ == *offset.offset_)
                        {
                            index = i;
                            offset.offset_++;
                            has_match = true;
                            break;
                        }
                    }
                }

                if (!has_match)
                {
                    if (node.up_closure_id_ == 0)
                    {
                        offset.begin_ += node.failed_text_offset_;
                        offset.offset_ = offset.begin_ + node.failed_text_offset_;
                        index = node.failed_goto_id_;
                    }
                    else
                    {
                        //matched
                        index = node.up_closure_id_;
                        CharState<Ty>& node = zip_states_->nodes_[index];
                        offset.offset_ = offset.begin_ + node.text_offset_;


                        offset.begin_ += node.text_offset_;
                        offset.offset_ = offset.begin_;
                        index = 0;
                    }
                }

            }

            return 0;
        }

        template<class Ty>
        std::string MatchTree<Ty>::ReplaceContentImpl(const char* content, size_t lenth, bool symbo_bound, bool use_ac, ToString to_string)
        {
            static bool symbo_table[] =
            {
                1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 0-15
                1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 16-31
                1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, // 32-47
                0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 1, 1, 1, 1, 1, // 48-63   (0-9 :;<=>?)
                1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 64-79 
                0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 1, 1, 1, 1, // 80-95 
                1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 96-111 
                0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 1, 1, 1, 0, //  -127
            };



            State state;
            state.offset_.begin_ = content;
            state.offset_.offset_ = content;
            state.offset_.end_ = content + lenth;
            state.offset_.node_ = &root_;
            std::string result;
            const char* last_begin = content;

            auto bound_proc = [&result, &last_begin, &to_string, symbo_bound, content](Offset& offset)
            {
                if (symbo_bound)
                {
                    if (offset.begin_ > content)
                    {
                        char pre_c = *(offset.begin_ - 1);
                        if (pre_c < 0)
                        {
                            return;
                        }
                        if (!symbo_table[(int)pre_c])
                        {
                            return;
                        }
                    }
                    if (offset.offset_ < offset.end_)
                    {
                        char nex_c = *(offset.offset_);
                        if (nex_c < 0)
                        {
                            return;
                        }
                        if (!symbo_table[(int)nex_c])
                        {
                            return;
                        }
                    }
                }
                result += std::string(last_begin, offset.begin_ - last_begin);
                result += to_string(offset.node_->val_);
                last_begin = offset.offset_;
            };

            s32 ret = 0;  
            if (use_ac)
            {
                ret = AcMatchContent(state, bound_proc);
            }
            else
            {
                ret = MatchContent(state, bound_proc);
            }
            if (ret != 0)
            {
                LogError() << "ac match has error";
            }
            result += std::string(last_begin, state.offset_.end_ - last_begin);
            return result;
        }
        template<class Ty>
        std::string MatchTree<Ty>::ReplaceContent(const char* content, size_t lenth, ToString to_string)
        {
            return ReplaceContentImpl(content, lenth, false, false, to_string);
        }

        template<class Ty>
        std::string MatchTree<Ty>::AcReplaceContent(const char* content, size_t lenth, ToString to_string)
        {
            return ReplaceContentImpl(content, lenth, false, false, to_string);
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
                    node->child_tree_[i]->seq_id_ = seq_count_++;
                }
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
                    if (node->goto_node_->child_tree_[node->node_char_] != NULL)
                    {
                        node->goto_node_ = node->goto_node_->child_tree_[node->node_char_];
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
                                state.offset_.end_ = state.offset_.begin_ + path.size();
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
                    path.push_back((char)node->child_tree_[i]->node_char_);
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
            root_.seq_id_ = seq_count_++;
            s32 ret = BuildUpMatchedRecursive(&root_);
            std::string path;
            ret |= BuildGotoStateRecursive(&root_, path);
            if (ret == 0)
            {
                ret |= BuildZipState();
            }
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
            bool jump_table[256] = { 0 };
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

        template<class Ty>
        s32 MatchTree<Ty>::BuildZipState(s32& cur_index, Node* node)
        {
            if (cur_index > zip_states_->node_size_)
            {
                return -1;
            }
            if (cur_index + node->childs_ > zip_states_->node_size_)
            {
                return -2;
            }
            CharState<Ty>* state = &zip_states_->nodes_[node->seq_id_];
            state->child_count_ = node->childs_;
            state->child_begin_id_ = cur_index;
            cur_index += node->childs_;

            for (size_t i = 0; i < 256; i++)
            {
                Node* child = node->child_tree_[i];
                if (child)
                {
                    CharState<Ty>* state = &zip_states_->nodes_[child->seq_id_];
                    state->failed_goto_id_ = child->goto_node_->seq_id_;
                    state->failed_text_offset_ = child->goto_content_forward_;
                    state->node_char_ = child->node_char_;
                    state->text_offset_ = child->depth_;
                    state->up_closure_id_ = child->up_matched_ == NULL ? 0 : child->up_matched_->seq_id_;
                    state->val_ = child->val_;
                    state->has_val_ = child->has_val_;
                }
            }
            for (size_t i = 0; i < 256; i++)
            {
                Node* child = node->child_tree_[i];
                if (child)
                {
                    s32 ret = BuildZipState(cur_index, child);
                    if (ret != 0)
                    {
                        return ret;
                    }
                }
            }
            return 0;
        }

        template<class Ty>
        s32 MatchTree<Ty>::BuildZipState()
        {
            if (zip_states_ != NULL)
            {
                return -1;
            }
            if (seq_count_ <= 0 )
            {
                return -2;
            }
            u32 zip_bytes = seq_count_ * sizeof(CharState<Ty>) + sizeof(ZipStateMachine<Ty>);
            char* zip_begin = new char[zip_bytes];
            zip_states_ = (ZipStateMachine<Ty>*)zip_begin;
            memset(zip_begin, 0, zip_bytes);
            zip_states_->node_size_ = seq_count_;

            CharState<Ty>* first = &zip_states_->nodes_[0];
            first->failed_goto_id_ = 0;
            first->failed_text_offset_ = 1;
            first->text_offset_ = 0;
            first->node_char_ = root_.node_char_;
            first->up_closure_id_ =0;
            first->has_val_ = root_.has_val_;
            first->val_ = root_.val_;
 

            s32 index = 1;
            return BuildZipState(index, &root_);
        }

        


    }
}


#endif




