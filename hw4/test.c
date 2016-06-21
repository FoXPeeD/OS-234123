#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/random.h>

static inline void print_hex_byte (char c)
{
	char l = c & 0x0f;
	char h = (c >> 4) & 0x0f;

	putchar (h < 10 ? h + '0' : h + 'a' - 10);
	putchar (l < 10 ? l + '0' : l + 'a' - 10);
}

static void print_buf (const char *buf, size_t size)
{
	size_t i = 0;

	if (!size)
		return;

	while (1) {
		print_hex_byte (buf [i]);

		i++;
		if (i >= size)
			break;

		if (i % 4 == 0)
			putchar (' ');
		if (i % 32 == 0)
			putchar ('\n');
	}

	putchar ('\n');
	putchar ('\n');
}

int main (int argc, char **argv)
{
	int fd;
	struct rand_pool_info *info;
	char buf [40];

	if (argc != 2) {
		fprintf (stderr, "Usage: %s <device>\n", argv [0]);
		return EXIT_FAILURE;
	}

	fd = open (argv [1], O_RDWR);
	if (fd < 0) {
		perror (argv [1]);
		return EXIT_FAILURE;
	}

	info = malloc (sizeof(*info) + 128);
	if (!info) {
		perror ("malloc");
		return EXIT_FAILURE;
	}

	info->entropy_count = 4096;
	info->buf_size = 128;
	memset (info->buf, 0, 128);
	if (ioctl (fd, RNDADDENTROPY, info) < 0) {
		perror ("ioctl(RNDADDENTROPY)");
		return EXIT_FAILURE;
	}

	free (info);

	if (read (fd, buf, sizeof(buf)) < 40) {
		perror ("read");
		return EXIT_FAILURE;
	}

	print_buf (buf, sizeof(buf));

	close (fd);

	return EXIT_SUCCESS;
}
