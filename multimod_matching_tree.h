
/*

multimod_matching_tree License
-----------

multimod_matching_tree is licensed under the terms of the MIT license reproduced below.
This means that Log4z is free software and can be used for both academic
and commercial purposes at absolutely no cost.


===============================================================================

Copyright (C) 2014 YaweiZhang <yawei_zhang@foxmail.com>.

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


struct pattern_tree 
{
	unsigned char _is_valid_node;
	unsigned char _is_boundary;
	unsigned char _level;
	struct pattern_tree * _child; //PatternTree[256]
};

static int multi_mod_pattern_tree_from_file(struct pattern_tree **head, const char * file_name, char delimiter);
static void multi_mod_pattern_tree(struct pattern_tree **pt, const char * pattern, unsigned int pattern_len);
static unsigned int multi_mod_pattern_tree_matching(const struct pattern_tree *pt, const char * text, unsigned int text_len, unsigned char is_greedy);
static void multi_mod_pattern_tree_translate(const struct pattern_tree *pt, char * text, unsigned int text_len, unsigned char is_greedy, char escape);
static void multi_mod_pattern_tree_free(struct pattern_tree **pt);



static void multi_mod_pattern_tree(struct pattern_tree **pt, const char * pattern, unsigned int pattern_len)
{
	struct pattern_tree ** ppnode = pt;
	unsigned int i = 0;
	unsigned char ch_index = 0;
	for (i = 0; i < pattern_len; i++)
	{
		ch_index = (unsigned char)pattern[i];
		if (*ppnode == NULL)
		{
			*ppnode = (struct pattern_tree *)malloc(sizeof(struct pattern_tree)* 256);
			memset(*ppnode, 0, sizeof(struct pattern_tree)* 256);
		}
		(*ppnode)[ch_index]._is_valid_node = 1;
		if (i == pattern_len - 1)
		{
			(*ppnode)[ch_index]._is_boundary = 1;
		}
		ppnode = &((*ppnode)[ch_index]._child);
	}
}

static int multi_mod_pattern_tree_from_file(struct pattern_tree **head, const char * file_name, char delimiter)
{
	FILE * fp = NULL;
	char * file_content = NULL;
	unsigned int file_content_len = 0;

	unsigned int i = 0;



	char * pattern = NULL;
	unsigned int pattern_len = 0;

#ifdef WIN32
	if (fopen_s(&fp, file_name, "r") != 0)
	{
		return 1;
	}
#else
	fp = fopen(file_name, "r");
	if (!fp)
	{
		return 1;
	}
#endif

	fseek(fp, 0, SEEK_END);
	file_content_len = ftell(fp);
	file_content = (char*)malloc(file_content_len + 1);
	file_content[file_content_len] = '\0';
	fseek(fp, 0, SEEK_SET);
	file_content_len = fread(file_content, 1, file_content_len, fp);
	fclose(fp);
	fp = NULL;

	//delete file BOM head
	{
		if (file_content_len >= 3 
			&& (unsigned char)file_content[0] == 0xef 
			&& (unsigned char)file_content[1] == 0xbb 
			&& (unsigned char)file_content[2] == 0xbf)
		{
			file_content[0] = delimiter;
			file_content[1] = delimiter;
			file_content[2] = delimiter;
		}
		
	}
	
	

	pattern = file_content;
	for (i = 0; i < file_content_len; i++)
	{
		if (file_content[i] == delimiter || file_content[i] == '\0')
		{
			file_content[i] = '\0';
			pattern_len = strlen(pattern);
			if (pattern_len > 0)
			{
				multi_mod_pattern_tree(head, pattern, pattern_len);
			}
			pattern = file_content + i + 1;
		}
	}
	free(file_content);
	file_content = NULL;
	file_content_len = 0;
	return 0;
}

static unsigned int multi_mod_pattern_tree_matching(const struct pattern_tree *pt, const char * text, unsigned int text_len, unsigned char is_greedy)
{
	unsigned int ret_len = 0;
	unsigned int i = 0;
	for (i = 0; i < text_len; ++i)
	{
		unsigned char ch_index = (unsigned char)text[i];
		if (pt == NULL || !pt[ch_index]._is_valid_node)
		{
			return ret_len;
		}
		if (pt[ch_index]._is_boundary)
		{
			ret_len = i + 1;
			if (!is_greedy)
			{
				return ret_len;
			}
		}
		pt = pt[ch_index]._child;
	}
	return ret_len;
}

static void multi_mod_pattern_tree_translate(const struct pattern_tree *pt, char * text, unsigned int text_len, unsigned char is_greedy, char escape)
{
	int matching_count = 0;
	int i = 0;
	int j = 0;
	for (i = 0; i < text_len; i++)
	{
		matching_count = multi_mod_pattern_tree_matching(pt, text + i, text_len - i, is_greedy);
		if (matching_count > 0)
		{
			for (j = i; j < i + matching_count; j++)
			{
				text[j] = escape;
			}
		}
	}
}

static void multi_mod_pattern_tree_free(struct pattern_tree **pt)
{
	int i = 0;
	if (*pt == NULL)
	{
		return;
	}
	for (i = 0; i < 256; i++)
	{
		multi_mod_pattern_tree_free(&(*pt)[i]._child);
	}
	free(*pt);
	*pt = NULL;
}





#endif




