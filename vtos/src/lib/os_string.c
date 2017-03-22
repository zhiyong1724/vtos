#include "os_string.h"
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
