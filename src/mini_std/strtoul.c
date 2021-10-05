#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "stdio.h"

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
	const char *p = nptr, *q;
	char c = 0;
	unsigned long r = 0;
	if (base < 0 || base == 1 || base > 36)
	{
		if (endptr != NULL)
			*endptr = (char *)nptr;
		errno = EINVAL;
		return 0;
	}
	if (!(nptr && *nptr))
	{
		errno = EINVAL;
		return 0;
	}
	while (isspace((int)*p))
		p++;
	if (*p == '-' || *p == '+')
		c = *p++;
	if (base == 0)
	{
		if (p[0] == '0')
		{
			if (tolower((int)p[1]) == 'x' && isxdigit((int)p[2]))
			{
				p += 2;
				base = 16;
			}
			else
				base = 8;
		}
		else
			base = 10;
	}
	else
	{
		if (base == 16 && p[0] == '0' && p[1] == 'x' && isxdigit(p[2]))
			p += 2;
	}
	q = p;
	for (;;)
	{
		int a;
		if (!isalnum((int)*q))
			break;
		a = isdigit((int)*q) ? *q - '0' : tolower((int)*q) - ('a' - 10);
		if (a >= base)
			break;
		if (r > (ULONG_MAX - a) / base || r * base > ULONG_MAX - a)
		{
			errno = ERANGE; /* overflow */
			r = ULONG_MAX;
			c = 0;
		}
		else
			r = r * base + a;
		q++;
	}
	if (q == p) /* Not a single number read */
	{
		if (endptr != NULL)
			*endptr = (char *)nptr;
		return 0;
	}
	if (c == '-')
		r = -r;
	if (endptr != NULL)
		*endptr = (char *)q;
	return r;
}
