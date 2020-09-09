
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
	MatchState<int> ms3 = ms;
	ret = tree.MatchContent(ms);
	AssertMatch(ret, 0, "");
	ret = tree.AcMatchContent(ms2);
	tree.AcZipMatchContent(ms3);
	AssertMatch(ret, 0, "");
	AssertMatch(ms.results_.size(), ms2.results_.size(), "");
	AssertMatch(ms.results_.size(), 1, "");


	content = "18888";
	ms.offset_.begin_ = content.c_str();
	ms.offset_.end_ = ms.offset_.begin_ + content.length();
	ms.offset_.offset_ = ms.offset_.begin_;
	ms.results_.clear();

	ms2 = ms;
	ms3 = ms;

	ret = tree.MatchContent(ms);
	AssertMatch(ret, 0, "");
	ret = tree.AcMatchContent(ms2);
	tree.AcZipMatchContent(ms3);
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
	LogInfo() << "simple test success.";
	return 0;
}

s32 ReplaceTest()
{
	MatchTree<int> tree;
	tree.AddPattern("5555", 4, 504);
	tree.AddPattern("555", 3, 503);
	tree.AddPattern("55", 2, 502);
	tree.AddPattern("18888", 5, 188);
	tree.AddPattern("8888", 4, 888);
	tree.BuildGotoStateRecursive();
	std::string content = "房间号5555和房间号555发现了55, 从8188888到888888";
	std::string new_content = tree.ReplaceContent(content.c_str(), content.length(), 
		[](const int& n) {char buf[50]; sprintf(buf, "%d", n); return std::string(buf); });
	std::string new_ac_content = tree.AcReplaceContent(content.c_str(), content.length(),
		[](const int& n) {char buf[50]; sprintf(buf, "%d", n); return std::string(buf); });
	if (new_content != new_ac_content)
	{
		LogError() << "ac replace and general replace not equal";
		return -1;
	}
	if (new_content != "房间号504和房间号503发现了502, 从81888到88888")
	{
		LogError() << "not expect result content:" << new_ac_content;
		return -2;
	}

	s32 ret = tree.DestroyPatternTree();
	if (ret != 0)
	{
		LogError() << "destroy error";
		return -3;
	}
	tree.AddPattern("F", 1, 8);
	tree.AddPattern("NFF", 3, 88);
	content = "NFFF is NFF not FF and not F";
	tree.BuildGotoStateRecursive();
	new_content = tree.ReplaceContentImpl(content.c_str(), content.length(), true, false,
		[](const int& n) {char buf[50]; sprintf(buf, "%d", n); return std::string(buf); });
	new_ac_content = tree.ReplaceContentImpl(content.c_str(), content.length(), true, true,
		[](const int& n) {char buf[50]; sprintf(buf, "%d", n); return std::string(buf); });
	if (new_content != new_ac_content)
	{
		LogError() << "ac replace and general replace not equal";
		return -1;
	}
	if (new_content != "NFFF is 88 not FF and not 8")
	{
		LogError() << "not expect result content:" << new_ac_content;
		return -2;
	}
	LogInfo() << "replace success:" << new_ac_content;
	return 0;
}

s32 StressTest()
{

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
			return -1;
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
			return -3;
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
		if (content[i] == ',' || content[i] == ' ' || content[i] == '.')
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
			return -5;
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
		auto state2 = state;
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
			return -5;
		}

	}
	LogInfo() << "build & goto state & ac_match & destroy used:" << Now() - now;

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
		ret |= tree.AcZipMatchContent(state);
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
			ret |= tree.AcZipMatchContent(state);
		}

		ret |= tree.DestroyPatternTree();
		if (ret != 0)
		{
			LogError() << "has error";
			return -5;
		}

	}
	LogInfo() << "build & goto state & ac_zip_match & destroy used:" << Now() - now;
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
	
	if (ReplaceTest() != 0)
	{
		LogError() << "replace test error";
		return -2;
	}
	if (StressTest() != 0)
	{
		LogError() << "stress test error";
		return -3;
	}

	return 0;
}



