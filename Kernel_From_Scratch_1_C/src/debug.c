#include "io.h"
#include "./vga.h"
#include "string.h"
#include <stdint.h>

static int	ft_is_base_valid(const char *base)
{
	size_t	i;
	size_t	j;

	if (base == 0 || ft_strlen(base) < 2)
		return (0);
	i = 0;
	while (base[i])
	{
		if ((base[i] >= 9 && base[i] <= 13) || base[i] == ' '
			|| base[i] == '+' || base[i] == '-')
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

static void	ft_putnbr_base(uint8_t n, const char *base)
{
	size_t	base_len;

	if (!ft_is_base_valid(base))
		return ;
	base_len = ft_strlen(base);
	if (n >= base_len)
		ft_putnbr_base(n / base_len, base);
	put_char(base[n % base_len]);
}

void        key_debug(uint8_t status){
    put_chars("0x");
    ft_putnbr_base(status, "0123456789ABCDEF");
    put_char('\n');
}

/*
jinseo code
void        key_debug_loop()
{
    uint8_t prev = 0xff;

    while (1)
    {
        uint8_t status = io_inb(0x64);
        uint8_t data = io_inb(0x60);
        if (status != prev){
            reset_screen();
            put_chars("status is : ");
            key_debug(status);
            put_chars("data is : ");
            key_debug(data);
            prev = status;
        }
    }
}
*/

void key_debug_loop(void)//AI CODE
{
    uint8_t prev;
    uint8_t status;
    uint8_t data;

    prev = io_inb(0x64);

    while (1)
    {
        status = io_inb(0x64);

        if (status != prev)
        {
            // reset_screen();

            put_chars("prev status : ");
            key_debug(prev);

            put_chars("new status  : ");
            key_debug(status);

            if (status & 1)
            {
                data = io_inb(0x60);
                put_chars("data        : ");
                key_debug(data);
            }

            prev = status;
        }
    }
}