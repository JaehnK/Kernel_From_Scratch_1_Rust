#include "string.h"

size_t	ft_strlen(const char *str)
{
	size_t	len;

	len = 0;
    // in kernel not defence NULL  
	// if (str == NULL)
	// 	return NULL;
	while (*(str + len))
		len++;
	return (len);
}

ssize_t	ft_atoi(const char *nptr)
{
	ssize_t	nb;
	ssize_t	flag;
	ssize_t	mi;

	nb = 0;
	flag = 0;
	mi = 1;
	while ((*nptr >= 9 && *nptr <= 13) || *nptr == 32)
		nptr++;
	while (*nptr == '+' || *nptr == '-')
	{
		if (*nptr == '-')
			mi *= -1;
		nptr++;
		flag++;
	}
	if (flag > 1)
		return (nb);
	while (*nptr >= '0' && *nptr <= '9')
	{
		nb = (*nptr - 48) + (nb * 10);
		nptr++;
	}
	return (nb * mi);
}

static int	ft_is_space(char c)
{
	return ((c >= 9 && c <= 13) || c == 32);
}

static int	ft_is_valid_base(const char *base)
{
	size_t	i;
	size_t	j;

	if (base == 0 || ft_strlen(base) < 2)
		return (0);
	i = 0;
	while (base[i])
	{
		if (ft_is_space(base[i]) || base[i] == '+' || base[i] == '-')
			return (0);
		j = i + 1;
		while (base[j])
		{
			if (base[i] == base[j])
				return (0);
			j++;
		}
		i++;
	}
	return (1);
}

static ssize_t	ft_base_pos(char c, const char *base)
{
	ssize_t	pos;

	pos = 0;
	while (base[pos])
	{
		if (base[pos] == c)
			return (pos);
		pos++;
	}
	return (-1);
}

ssize_t	ft_atoi_base(const char *str, const char *base)
{
	ssize_t	nb;
	ssize_t	pos;
	ssize_t	sign;
	ssize_t	base_len;

	if (str == 0 || !ft_is_valid_base(base))
		return (0);
	nb = 0;
	sign = 1;
	base_len = (ssize_t)ft_strlen(base);
	while (ft_is_space(*str))
		str++;
	if (*str == '-' || *str == '+')
	{
		if (*str == '-')
			sign = -1;
		str++;
	}
	pos = ft_base_pos(*str, base);
	while (pos >= 0)
	{
		nb = (nb * base_len) + pos;
		str++;
		pos = ft_base_pos(*str, base);
	}
	return (nb * sign);
}