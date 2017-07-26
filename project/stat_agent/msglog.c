#include <assert.h>
#include <string.h>

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/stat.h>

#include "msglog.h"

enum {
	buf_size	= 4096
};

int msglog(const char* logfile, uint32_t type, uint32_t timestamp, const void* data, int len)
{
	char buf[buf_size];
    message_header_t *h;
    int fd, s;

	s = sizeof(message_header_t)+len;

	assert((len >= 0) && (s >= (int)sizeof(message_header_t)) && (s <= buf_size));

	h = (void*)buf;

	memset(h, 0, sizeof(*h));

    h->len = s;
    h->hlen = sizeof(message_header_t);
    h->type = type;
    h->timestamp = timestamp;

    if(len>0) memcpy((char *)(h+1), data, len);

    signal(SIGXFSZ, SIG_IGN);
    fd = open(logfile, O_WRONLY|O_APPEND, 0666);
    if(fd<0) 
    {
        fd = open(logfile, O_WRONLY|O_APPEND|O_CREAT, 0666);
        int ret=fchmod(fd,0777);
        if ((ret!=0)||(fd<0))
        {
            return -1;
        }
    }

    write(fd, (char *)h, s);
    close(fd);

    signal(SIGXFSZ, SIG_DFL);
    return 0;
}
