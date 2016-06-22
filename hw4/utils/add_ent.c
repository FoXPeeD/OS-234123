#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <errno.h>
#include <sys/ioctl.h>

#define BUF_SIZE 512

int main(int argc, char **argv) {
    int ret, fd, ent_count;
    fd = open("/dev/srandom", O_RDONLY);
    if (fd < 0) {
        printf("open: error\n");
        return 1;
    }
    if (argc > 1) {
        ent_count = atoi(argv[1]);
    } else {
        ent_count = 10;
    }
    struct rand_pool_info *rp = malloc(sizeof(*rp) + BUF_SIZE);

    ssize_t s;
    size_t count = 0;
    while ((s=read(0, ((char*)(rp->buf))+count, BUF_SIZE-count))>0) {
        count += s;
    }
    rp->buf_size = count;
    rp->entropy_count = ent_count;

    ret = ioctl(fd, RNDADDENTROPY, rp);
    printf("RNDADDENTROPY: %d\n", ret);
    close(fd);
    free(rp);
    return 0;
}
