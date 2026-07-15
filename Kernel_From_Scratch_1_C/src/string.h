#ifndef STRING_H
#define STRING_H

#include <stddef.h>

#ifndef KFS_SSIZE_T_DEFINED
# define KFS_SSIZE_T_DEFINED
typedef long	ssize_t;
#endif

size_t	ft_strlen(const char *str);
ssize_t	ft_atoi(const char *nptr);
ssize_t	ft_atoi_base(const char *str, const char *base);

#endif
