#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <asm/uaccess.h>
int main(void)
{
	int fd = 0;
	int err = 0;
	unsigned char key = 0;

	fd = open("/dev/buttons", O_RDWR);

	while(1)
	{
		err = read(fd, &key, 1);

		printf("key %d\n", key);
	}

	return 0;
}


