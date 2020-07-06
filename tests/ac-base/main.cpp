
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fn_log.h"
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#define USED_FN_LOG
#include "ac_match_tree.h"

using namespace zsummer::matching;

#define AssertMatch(val1, val2, desc)  \
{\
	auto v1 = val1;\
	auto v2 = val2;\
	if (v1 != (decltype(v1))v2)\
	{\
		LogError() << desc << " expr " << v1 << " not equal " << v2;\
		return -1;\
	}\
}

#define Now() std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count()                                

s32 SimpleTest()
{
	MatchTree<int> tree;
	s32 ret = tree.AddPatternFromString("1234, 12345, 155555, 23456, 188888, 8888", ',');
	if (ret != 0)
	{
		LogError() << "error";
		return -1;
	}
	if (tree.node_count_ != 24)
	{
		LogError() << " node count error";
		return -2;
	}
	ret = tree.BuildGotoStateRecursive();
	if (ret != 0)
	{
		LogError() << "error build goto";
		return -5;
	}
	MatchNode<int>* node = tree.MatchPath("1", 1);
	AssertMatch(node == NULL, false, "");
	AssertMatch(node->goto_content_forward_, 1, "");
	AssertMatch(node->goto_node_->depth_, 0, "");

	node = tree.MatchPath("1234", 4);
	MatchNode<int>* goto_node = tree.MatchPath("234", 3);
	AssertMatch(node == NULL, false, "");
	AssertMatch(goto_node == NULL, false, "");

	AssertMatch(node->goto_node_, goto_node, "");

	AssertMatch(tree.MatchPath("123", 3)->goto_node_, tree.MatchPath("23", 2), "");
	AssertMatch(tree.MatchPath("123", 3)->goto_node_->depth_, 2, "");
	AssertMatch(tree.MatchPath("123", 3)->goto_content_forward_, 1, "");

	AssertMatch(tree.MatchPath("1555", 4)->goto_node_, &tree.root_, "");
	AssertMatch(tree.MatchPath("1555", 4)->goto_node_->depth_, 0, "");
	AssertMatch(tree.MatchPath("1555", 4)->goto_content_forward_, 4, "");

	AssertMatch(tree.MatchPath("155", 3)->goto_node_, &tree.root_, "");
	AssertMatch(tree.MatchPath("155", 3)->goto_node_->depth_, 0, "");
	AssertMatch(tree.MatchPath("155", 3)->goto_content_forward_, 3, "");

	AssertMatch(tree.MatchPath("15", 2)->goto_node_, &tree.root_, "");
	AssertMatch(tree.MatchPath("15", 2)->goto_node_->depth_, 0, "");

	
	std::string content = "1234567";

	MatchState<int> ms;
	ms.offset_.begin_ = content.c_str();
	ms.offset_.end_ = ms.offset_.begin_ + content.length();
	ms.offset_.offset_ = ms.offset_.begin_;
	ms.results_.clear();

	MatchState<int> ms2 = ms;
	ret = tree.MatchContent(ms);
	AssertMatch(ret, 0, "");
	ret = tree.AcMatchContent(ms2);
	AssertMatch(ret, 0, "");
	AssertMatch(ms.results_.size(), ms2.results_.size(), "");
	AssertMatch(ms.results_.size(), 1, "");


	content = "18888";
	ms.offset_.begin_ = content.c_str();
	ms.offset_.end_ = ms.offset_.begin_ + content.length();
	ms.offset_.offset_ = ms.offset_.begin_;
	ms.results_.clear();

	ms2 = ms;
	ret = tree.MatchContent(ms);
	AssertMatch(ret, 0, "");
	ret = tree.AcMatchContent(ms2);
	AssertMatch(ret, 0, "");
	AssertMatch(ms.results_.size(), ms2.results_.size(), "");
	AssertMatch(ms.results_.size(), 1, "");
	AssertMatch(ms.results_[0].offset_ - ms.results_[0].begin_, 4, "");
	AssertMatch(std::string(ms.results_[0].begin_, ms.results_[0].offset_ - ms.results_[0].begin_), "8888", "");


	ret = tree.DestroyPatternTree();
	if (ret != 0)
	{
		LogError() << "error destroy";
		return -10;
	}
	return 0;
}

#define TEST_COUNT 10*1000
int main(int argc, char* argv[])
{
	FNLog::FastStartDebugLogger();
	LogDebug() << "start log";

	if (SimpleTest() != 0)
	{
		LogError() << "simple test error.";
		return -1;
	}
	LogInfo() << "simple test success.";

	double now = 0.0;
	std::string filterworlds = MatchTree<int>::ReadFile("filterworlds.txt");
	now = Now();
	if (true)
	{
		MatchTree<int> tree;
		s32 ret = tree.AddPatternFromString(filterworlds.c_str(), ',');
		ret |= tree.DestroyPatternTree();
		if (ret != 0)
		{
			LogError() << "has error";
		}
	}
	LogInfo() << "build & destroy used:" << Now() - now;
	now = Now();
	if (true)
	{
		MatchTree<int> tree;
		s32 ret = tree.AddPatternFromString(filterworlds.c_str(), ',');
		ret |= tree.BuildGotoStateRecursive();
		ret |= tree.DestroyPatternTree();
		if (ret != 0)
		{
			LogError() << "has error";
		}
	}
	LogInfo() << "build & goto state & destroy used:" << Now() - now;

	std::string content = filterworlds;
	for (size_t i = 0; i < content.size(); i++)
	{
		if (rand() % 3 == 0)
		{
			content[i] = 'a' + (rand() % 25);
		}
		if (content[i] == ',' ||content[i] ==' ' || content[i] == '.')
		{
			content[i] = 'a' + (rand() % 25);
		}
	}
	//LogDebug() << content;
	
	now = Now();
	if (true)
	{
		MatchTree<int> tree;
		s32 ret = tree.AddPatternFromString(filterworlds.c_str(), ',');
		LogDebug() << "content size:" << content.size() << ", pattern size:" << tree.node_count_;
		MatchTree<int>::State state;
		state.offset_.begin_ = content.c_str();
		state.offset_.end_ = content.c_str() + content.length();
		state.offset_.offset_ = state.offset_.begin_;
		state.offset_.node_ = &tree.root_;
		ret |= tree.MatchContent(state);
		LogInfo() << "build & match one:" << Now() - now;
		std::string str;
		for (auto& s : state.results_)
		{
			str += std::string(s.begin_, s.offset_ - s.begin_) + " ";
		}
		LogDebug() << "match results:" << state.results_.size();// << "\n" << str;
		for (size_t i = 0; i < 1000; i++)
		{
			state.offset_.begin_ = content.c_str();
			state.offset_.end_ = content.c_str() + content.length();
			state.offset_.offset_ = state.offset_.begin_;
			state.offset_.node_ = &tree.root_;
			state.results_.clear();
			ret |= tree.MatchContent(state);
		}
		ret |= tree.DestroyPatternTree();
		if (ret != 0)
		{
			LogError() << "has error";
		}
	}
	LogInfo() << "build & match & destroy used:" << Now() - now;
	now = Now();
	if (true)
	{
		MatchTree<int> tree;
		s32 ret = tree.AddPatternFromString(filterworlds.c_str(), ',');
		ret |= tree.BuildGotoStateRecursive();
		LogDebug() << "content size:" << content.size() << ", pattern size:" << tree.node_count_;
		MatchTree<int>::State state;
		state.offset_.begin_ = content.c_str();
		state.offset_.end_ = content.c_str() + content.length();
		state.offset_.offset_ = state.offset_.begin_;
		state.offset_.node_ = &tree.root_;
		ret |= tree.AcMatchContent(state);	
		LogInfo() << "build & match one:" << Now() - now;
		std::string str;
		for (auto& s : state.results_)
		{
			str += std::string(s.begin_, s.offset_ - s.begin_) + " ";
		}
		LogDebug() << "match results:" << state.results_.size(); //  << "\n" << str;
		for (size_t i = 0; i < 1000; i++)
		{
			state.offset_.begin_ = content.c_str();
			state.offset_.end_ = content.c_str() + content.length();
			state.offset_.offset_ = state.offset_.begin_;
			state.offset_.node_ = &tree.root_;
			state.results_.clear();
			ret |= tree.AcMatchContent(state);
		}

		ret |= tree.DestroyPatternTree();
		if (ret != 0)
		{
			LogError() << "has error";
		}

	}
	LogInfo() << "build & goto state & ac_match & destroy used:" << Now() - now;

	return 0;
}



