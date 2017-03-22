#include "os_string.h"
#include "vtos.h"
void *os_mem_set(void *s, int8 ch, os_size_t n)
{
	int8 *ps = (int8 *) s;
	os_size_t i;
	for (i = 0; i < n; i++)
	{
		ps[i] = ch;
	}
	return s;
}

void *os_str_cpy(char *dest, const char *src, os_size_t n)
{
	os_size_t i = 0;
	while (*src != '\0' && n > 0 && i < n - 1)
	{
		*dest = *src;
		dest++;
		src++;
		i++;
	}
	*dest = '\0';
	return dest;
}

void os_utoa(char *buff, os_size_t num)
{
	os_size_t k = 10;
	os_size_t i = 0;
	for (i = 0; num / k > 0;i++)
	{
		k *= 10;
	}
	k /= 10;
	for (; i > 0; i--)
	{
		*buff = (char)(num / k);
		num -= (os_size_t)*buff * k;
		k /= 10;
		*buff += 48;
		buff++;
	}
	*buff = (char)num + 48;
	buff++;
	*buff = '\0';
}

os_size_t os_str_len(const char *str)
{
	os_size_t i = 0;
	for (;str != NULL && *str != '\0';str++)
	{
		i++;
	}
	return i;
}

static os_size_t *next_array(const char *pattern, os_size_t *next)
{
	os_size_t i = 0;
	os_size_t j = 0; 
	next[0] = 0;
	next[1] = 0;
	for(i = 1; pattern[i] != '\0'; i++)
	{
		while(j > 0 && pattern[i] != pattern[j])
		{
			j = next[j];  
		}
		if(pattern[i] == pattern[j])
		{
			j++;  
		}
		next[i + 1] = j;  
	}
	return next;
}

os_size_t os_str_find(const char *str, const char *pattern)
{
	if (str != NULL && pattern != NULL)
	{
		os_size_t *next = os_kmalloc(sizeof(os_size_t) * (os_str_len(pattern) + 1));
		if (next != NULL)
		{
			os_size_t i = 0;
			os_size_t j = 0;
			next = next_array(pattern, next);
			for (i = 0; str[i] != '\0'; i++) 
			{  
				while (j > 0 && str[i] != pattern[j])
				{
					j = next[j];
				}

				if (str[i] == pattern[j])
				{
					j++;
				}
				if ('\0' == pattern[j]) 
				{  
					return i - j + 1;
				}  
			} 
			os_kfree(next);
		} 
	}
	return -1;
}
