
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "../match_tree.h"

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
	now += tm.tv_usec/1000;
#endif
	return (unsigned int)now;
}



#define TEST_COUNT 10*1000
int main(int argc, char* argv[])
{
	unsigned int begin = 0;
	int matching_count = 0;	
	struct match_tree_head * head = NULL;
	const char * filename = "filter.txt";
//	const char * filename = "filterworlds.txt";
	FILE * fp = NULL;
	char * file_content1 = NULL;
	unsigned int file_content1_len = 0;
	char * file_content2 = NULL;
	unsigned int file_content2_len = 0;
	unsigned int i = 0;
	unsigned int j = 0;
#ifdef WIN32
	if (fopen_s(&fp, filename, "rb") != 0)
	{
		return -1;
	}
#else
	fp = fopen(filename, "rb");
	if (!fp)
	{
		return -1;
	}
#endif

	fseek(fp, 0, SEEK_END);
	file_content1_len = ftell(fp);
	if (file_content1_len == 0)
	{
		return -1;
	}
	file_content1 = (char*)malloc(file_content1_len + 1);
	file_content2 = (char*)malloc(file_content1_len + 1);
	file_content1[file_content1_len] = '\0';
	fseek(fp, 0, SEEK_SET);
	file_content1_len = (unsigned int)fread(file_content1, 1, file_content1_len, fp);
	fclose(fp);
	fp = NULL;
	memcpy(file_content2, file_content1, file_content1_len);
	file_content2_len = file_content1_len;


	head = match_tree_init_from_file(filename, ".....", 5);
	begin = current_millisecond();
	for (i = 0; i < TEST_COUNT; i++)
	{
		for (j = 0; j < file_content1_len; j++)
		{
			if (match_tree_matching(head, file_content1 + j, file_content1_len - j, 0) > 0)
			{
				matching_count++;
			}
		}
	}
	printf("matching tree used time = %d, matching count=%d\n", current_millisecond() - begin, matching_count);
	

	printf("string      will   translate  with  greedy[%s]\n\n", file_content1);
	match_tree_translate(head, file_content1, file_content1_len, 1, '*');
	printf("string    is already translate with greedy[%s]\n\n", file_content1);


	printf("string   will   translate  without  greedy[%s]\n\n", file_content2);
	match_tree_translate(head, file_content2, file_content2_len, 0, '*');
	printf("string is already translate without greedy[%s]\n\n", file_content2);

	match_tree_free(head);
	head = NULL;
	return 0;
}



