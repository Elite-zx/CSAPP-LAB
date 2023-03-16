#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
/*
name: zhangxun
loginID: Elite-X
*/

typedef struct cache_line
{
	int valid_bit;
	int tag;
    int time_stamp;
}cache_line;

typedef struct cache
{
	int S;
	int E;
	int B;
	cache_line** Cache; 
}cache;

cache* construct_cache(int s, int E, int b);
void access_cache(cache* my_cache, int s, int b, char* trace_name, int* hit_count_ptr, int* miss_count_ptr, int* eviction_count_ptr);
void real_access_cache(cache* my_cache, int ad_set, int ad_tag, int* hit_count_ptr, int* miss_count_ptr, int* eviction_count_ptr);
void free_cache(cache* my_cache);
int get_line_index(cache* my_cache, int ad_set, int ad_tag);
void update_LRU(cache* my_cache, int ad_set, int ad_tag, int line_index);
int is_not_full(cache* my_cache,int ad_set);
int find_LRU(cache* my_cache,int ad_set);

int main(int argc, char* argv[])
{ 	
    int	hit_count = 0, miss_count = 0, eviction_count = 0;
    int s, E, b,opt;
    char* trace_name = (char*)malloc(sizeof(char)*30);
    cache* my_cache;
    while((opt = getopt(argc, argv, "s:E:b:t:"))!= -1){
		switch(opt){
		case 's':
		   s = atoi(optarg);
		   break;
		case 'E':
		   E = atoi(optarg);
		   break;
		case 'b':
		   b = atoi(optarg);
		   break;
		case 't':
		   strcpy(trace_name,optarg);
		   break;
		case '?':
		   printf("unknown option: %c\n",optopt);
		   break;
		   }
     }
     my_cache = construct_cache(s,E,b);
     access_cache(my_cache, s, b, trace_name, &hit_count, &miss_count, &eviction_count);
     free_cache(my_cache);
     printSummary(hit_count, miss_count, eviction_count);
     return 0;
 }

 cache* construct_cache(int s, int E, int b)
 {
     cache* my_cache =(cache*) malloc(sizeof(cache));  // construct Cache
	 my_cache->S = 1 << s;
	 my_cache->B = 1 << b;
	 my_cache->E = E;
	 my_cache->Cache = (cache_line**)malloc(my_cache->S * sizeof(cache_line*) );
	 for(int i=0; i<my_cache->S;++i)
	 {
		my_cache->Cache[i] = (cache_line*)malloc(my_cache->E * sizeof(cache_line));
		for(int j=0; j<my_cache->E; ++j) // initialize
		{
			my_cache->Cache[i][j].valid_bit = 0;
			my_cache->Cache[i][j].tag = -1;
			my_cache->Cache[i][j].time_stamp = 0;
		}

	}
	return my_cache;
 }
 void access_cache(cache* my_cache, int s, int b, char* trace_name, int* hit_count_ptr, int* miss_count_ptr, int* eviction_count_ptr)
 {
 	 FILE* pFile;   // receive access
     pFile = fopen(trace_name,"r");
     if(!pFile) exit(-1);
     char identifier;
     unsigned address;
     int size;
     while(fscanf(pFile," %c %x,%d",&identifier,&address,&size)>0)
     {     
		int mask =(unsigned)(-1)>>(64-s);
		int ad_set = (address >> b) & mask;
		int ad_tag = address >> (s+b);
		switch(identifier)
		{
		case 'M':
			real_access_cache(my_cache, ad_set, ad_tag, hit_count_ptr, miss_count_ptr, eviction_count_ptr);
			real_access_cache(my_cache, ad_set, ad_tag, hit_count_ptr, miss_count_ptr, eviction_count_ptr);
			break;
		case 'L':
			real_access_cache(my_cache, ad_set, ad_tag, hit_count_ptr, miss_count_ptr, eviction_count_ptr);
			break;
		case 'S':
			real_access_cache(my_cache, ad_set, ad_tag, hit_count_ptr, miss_count_ptr, eviction_count_ptr);
			break;
		}
	}
	fclose(pFile);
 }
 void real_access_cache(cache* my_cache, int ad_set, int ad_tag, int* hit_count_ptr, int* miss_count_ptr, int* eviction_count_ptr)
 {
    int line_index,free_line, evict_line;
	line_index = get_line_index(my_cache, ad_set, ad_tag);
	if(line_index != -1)
	{
		++(*hit_count_ptr);
		update_LRU(my_cache, ad_set, ad_tag, line_index);
	}

	else 
	{
		free_line = is_not_full(my_cache, ad_set);
		if(free_line != -1)
		{
			++(*miss_count_ptr);
			update_LRU(my_cache, ad_set, ad_tag, free_line);
		}

		else
		{
			++(*miss_count_ptr);
			++(*eviction_count_ptr);
			evict_line = find_LRU(my_cache,ad_set);
			update_LRU(my_cache, ad_set, ad_tag, evict_line);
		}
		
	}	
}


int get_line_index(cache* my_cache, int ad_set, int ad_tag)
{
	for (int i = 0; i < my_cache->E; ++i)
	{
		if(my_cache->Cache[ad_set][i].valid_bit && my_cache->Cache[ad_set][i].tag == ad_tag)
			return i;  // hit
	}
	return -1; // miss
}

void update_LRU(cache* my_cache, int ad_set, int ad_tag, int line_index)
{
	for (int i = 0; i < my_cache->E; ++i)
		if(my_cache->Cache[ad_set][i].valid_bit) ++(my_cache->Cache[ad_set][i].time_stamp);

	my_cache->Cache[ad_set][line_index].time_stamp = 0;
	my_cache->Cache[ad_set][line_index].valid_bit = 1;
	my_cache->Cache[ad_set][line_index].tag = ad_tag;
}

int is_not_full(cache* my_cache, int ad_set)
{
	for (int i = 0; i < my_cache->E; ++i)
		if(!my_cache->Cache[ad_set][i].valid_bit) return i;

	return -1;
}

int find_LRU(cache* my_cache, int ad_set)
{
	int max_stamp = 0;
	int evict_line = 0;
	int temp = 0;
	for (int i = 0; i < my_cache->E; ++i)
	{
		temp = my_cache->Cache[ad_set][i].time_stamp;
		if(temp > max_stamp)
			{
				max_stamp = temp;
				evict_line = i;
			}
	}
	return evict_line;
}

void free_cache(cache* my_cache)
{
	for (int i = 0; i < my_cache->E; ++i)
	{
		free(my_cache->Cache[i]);
	}
	free(my_cache->Cache);
}