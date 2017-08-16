#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
	int fd = 0;
	char dat = 0;

	if(argc != 2)
		printf("usage : ledtest <on | off>\n");

	fd = open("/dev/led1", O_RDWR);

	if(-1 == fd)
	{
		perror("open");
		return -1;
	}

	if(!strcmp(*(argv + 1), "on"))
		dat = 1;

	write(fd, &dat, 1);

	return 0;
}


