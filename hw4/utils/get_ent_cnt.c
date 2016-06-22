#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <errno.h>
#include <sys/ioctl.h>

int main() {
    int p, ret, fd;
    fd = open("/dev/srandom", O_RDONLY);
    if (fd < 0) {
        printf("open: error\n");
        return 1;
    }
    ret = ioctl(fd, RNDGETENTCNT, &p);
    if (ret < 0) {
        printf("RNDGETENTCNT: %d\n", ret);
    } else {
        printf("RNDGETENTCNT: %d, %d\n", ret, p);
    }
    close(fd);
    return 0;
}
