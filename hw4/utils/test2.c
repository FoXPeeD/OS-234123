#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/ioctl.h>

int fd;

int entropy_count() {
    int p, ret;
    ret = ioctl(fd, RNDGETENTCNT, &p);
    assert(ret == 0);
    return p;
}

void test_write() {
    int ret;
    assert(entropy_count() == 0);
    ret = write(fd, (void*)5, 10);
    assert(ret == -1);
    assert(errno == EFAULT);

    char buf[10];
    ret = write(fd, buf, 10);
    assert(ret == 10);
    assert(entropy_count() == 0);
}

void test_get_ent() {
    int ret, p;
    ret = ioctl(fd, RNDGETENTCNT, (void*)5);
    assert(ret == -1);
    assert(errno == EFAULT);
    ret = ioctl(fd, RNDGETENTCNT, &p);
    assert(ret == 0);
}

void test_add_ent() {
    int ret;
    assert(entropy_count() == 0);
    struct rand_pool_info *rp = malloc(sizeof(*rp) + 10);
    rp->buf_size = 10;
    rp->entropy_count = -5;
    ret = ioctl(fd, RNDADDENTROPY, (void*)5);
    assert(ret == -1);
    assert(errno == EFAULT);
    ret = ioctl(fd, RNDADDENTROPY, rp);
    assert(ret == -1);
    assert(errno == EINVAL);
    rp->entropy_count = 5;
    ret = ioctl(fd, RNDADDENTROPY, rp);
    assert(ret == 0);
    assert(entropy_count() == 5);
    rp->entropy_count = 5000;
    ret = ioctl(fd, RNDADDENTROPY, rp);
    assert(ret == 0);
    assert(entropy_count() == 4096);
}

void test_clr_pool() {
    int ret;
    assert(entropy_count() != 0);
    ret = ioctl(fd, RNDCLEARPOOL);
    assert(ret == 0);
    assert(entropy_count() == 0);
}

void test_read() {
    int ret;
    char buf[50];
    ret = read(fd, buf, 0);
    assert(ret == 0);
    ret = read(fd, (void*)5, 10);
    assert(ret == -1);
    assert(errno == EFAULT);
    ret = read(fd, buf, 50);
    assert(ret > 0 && ret <= 50);
}

int main() {
    fd = open("/dev/srandom", O_RDWR);
    if (fd < 0) {
        printf("open: error\n");
        return 1;
    }
    test_write();
    test_get_ent();
    test_add_ent();
    test_clr_pool();
    test_add_ent();
    test_read();
    close(fd);
    printf("seems ok\n");
    return 0;
}
