
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#endif

#include "../multimod_matching_tree.h"

unsigned int current_millisecond()
{
	unsigned long long now = 0;
#ifdef WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	now = ft.dwHighDateTime;
	now <<= 32;
	now |= ft.dwLowDateTime;
	now /= 10;
	now -= 11644473600000000ULL;
	now /= 1000;
#else
	struct timeval tm;
	gettimeofday(&tm, NULL);
	now = tm.tv_sec * 1000;
	now += tm.tv_usec;
#endif
	return (unsigned int)now;
}



#define TEST_COUNT 100*1000
int main(int argc, char* argv[])
{
	unsigned int begin = current_millisecond();
	int matching_count = 0;	
	struct pattern_tree * head = NULL;
#ifdef WIN32
	char chatmsg1[100] = "abcdefg chat msg 周瑜笑了一声";
	char chatmsg2[100] = "abcdefg chat msg 周瑜笑了一声";
#else
	char chatmsg1[100] = "abcdefg chat msg 周瑜笑了一声";
	char chatmsg2[100] = "abcdefg chat msg 周瑜笑了一声";
#endif

	unsigned int msg_len = strlen(chatmsg1);
	int i = 0;
	int j = 0;

#ifdef WIN32
	const char filename[] = "sanguo.txt";
#else
	const char filename[] = "sanguo_utf8.txt";
#endif
	

	multi_mod_pattern_tree_from_file(&head, filename, ',');
	
	begin = current_millisecond();
	for (i = 0; i < TEST_COUNT; i++)
	{
		for (j = 0; j < msg_len; j++)
		{
			if (multi_mod_pattern_tree_matching(head, chatmsg1, msg_len - j, 1) > 0)
			{
				matching_count++;
			}
		}
	}
	printf("matching tree used time = %d, matching count=%d\n", current_millisecond() - begin, matching_count);
	

	printf("string      will   translate  with  greedy[%s]\n", chatmsg1);
	multi_mod_pattern_tree_translate(head, chatmsg1, strlen(chatmsg1), 1, '*');
	printf("string    is already translate with greedy[%s]\n", chatmsg1);

	printf("string   will   translate  without  greedy[%s]\n", chatmsg2);
	multi_mod_pattern_tree_translate(head, chatmsg2, strlen(chatmsg2), 0, '*');
	printf("string is already translate without greedy[%s]\n", chatmsg2);

	multi_mod_pattern_tree_free(&head);
	return 0;
}



