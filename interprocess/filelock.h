/**
 *============================================================
 *  @file      filelock.h
 *  @brief     用于操作文件锁。注意事项：\n
 *             1. 关闭文件会导致该文件的所有文件锁都被解锁。假设fd1和fd2
 *             对应的都是同一个文件，那么无论关闭了fd1或者fd2，都会导致
 *             该文件的所有文件锁被解锁。\n
 *             2. 子进程不能继承父进程获得的文件锁。\n
 *             3. 通过exec来执行新的程序的话，这个新的程序可以继承原来的
 *             程序的文件锁。但是，如果原来的文件对应的fd被设置了close-on-exec
 *             位的话，则不能继承原来程序的文件锁。\n
 *             Advanced Programming in the UNIX Environment - 14.3. Record
 *             Locking 中有更详细的说明，建议认真阅读一下。
 *
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_FILELOCK_H_
#define LIBTAOMEE_FILELOCK_H_

#include <fcntl.h>

// set a filelock
static inline int
filelock_lock(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;

	lock.l_type   = type;    /* F_RDLCK, F_WRLCK, F_UNLCK */
	lock.l_start  = offset;  /* byte offset, relative to l_whence */
	lock.l_whence = whence;  /* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len    = len;     /* #bytes (0 means to EOF) */

	return fcntl(fd, cmd, &lock);
}

/**
 * @brief 获取读锁。如果获取不到，则一直阻塞，直到可以成功获取，或者发生错误为止。\n
 *        int   fd_       文件描述符，需要上读锁的文件。\n
 *        off_t offset_   相对whence_的偏移量（byte）。offset_和whence_共同决定了上锁的起始位置。\n
 *        int   whence_   可以是SEEK_SET（文件起始位置）、SEEK_CUR（文件当前位置）或SEEK_END（文件末尾）。\n
 *        off_t len_      从起始位置开始上锁的长度（byte）。如果len_为0，则把起始位置到文件可能的最大偏移量这个区间都上锁。\n
 *        用法示例：假设当前位于文件的第10个字节，则filelock_rlock(fd, 2, SEEK_CUR, 4);将会把该文件的12到15个字节上锁。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_rlock(fd_, offset_, whence_, len_) \
		filelock_lock((fd_), F_SETLKW, F_RDLCK, (offset_), (whence_), (len_))
/**
 * @brief 获取读锁。如果获取不到，则马上返回-1。\n
 *        int   fd_       文件描述符，需要上读锁的文件。\n
 *        off_t offset_   相对whence_的偏移量（byte）。offset_和whence_共同决定了上锁的起始位置。\n
 *        int   whence_   可以是SEEK_SET（文件起始位置）、SEEK_CUR（文件当前位置）或SEEK_END（文件末尾）。\n
 *        off_t len_      从起始位置开始上锁的长度（byte）。如果len_为0，则把起始位置到文件可能的最大偏移量这个区间都上锁。\n
 *        用法示例：假设当前位于文件的第10个字节，则filelock_try_rlock(fd, 2, SEEK_SET, 0);将会把该文件的第2个字节开始，
 *                  一直到到文件可能的最大偏移量这段区间都上锁。这样，无论以后往该文件的末尾添加了多少字节，这些字节都
 *                  属于被上锁的区间。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_try_rlock(fd_, offset_, whence_, len_) \
		filelock_lock((fd_), F_SETLK, F_RDLCK, (offset_), (whence_), (len_))
/**
 * @brief 给整个文件上读锁。如果上不了，则一直阻塞，直到成功，或者发生错误为止。\n
 *        int fd_       文件描述符，需要上读锁的文件。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_rlockfile(fd_) \
		filelock_lock((fd_), F_SETLKW, F_RDLCK, 0, SEEK_SET, 0)
/**
 * @brief 给整个文件上读锁。如果上不了，则马上返回-1。\n
 *        int fd_       文件描述符，需要上读锁的文件。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_try_rlockfile(fd_) \
		filelock_lock((fd_), F_SETLK, F_RDLCK, 0, SEEK_SET, 0)
/**
 * @brief 获取写锁。如果获取不到，则一直阻塞，直到可以成功获取，或者发生错误为止。\n
 *        int   fd_       文件描述符，需要上写锁的文件。\n
 *        off_t offset_   相对whence_的偏移量（byte）。offset_和whence_共同决定了上锁的起始位置。\n
 *        int   whence_   可以是SEEK_SET（文件起始位置）、SEEK_CUR（文件当前位置）或SEEK_END（文件末尾）。\n
 *        off_t len_      从起始位置开始上锁的长度（byte）。如果len_为0，则把起始位置到文件可能的最大偏移量这个区间都上锁。\n
 *        用法示例：假设当前位于文件的第10个字节，则filelock_wlock(fd, 2, SEEK_CUR, -4);将会把该文件的9到12字节上锁。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_wlock(fd_, offset_, whence_, len_) \
		filelock_lock((fd_), F_SETLKW, F_WRLCK, (offset_), (whence_), (len_))
/**
 * @brief 获取写锁。如果获取不到，则马上返回-1。\n
 *        int   fd_       文件描述符，需要上写锁的文件。\n
 *        off_t offset_   相对whence_的偏移量（byte）。offset_和whence_共同决定了上锁的起始位置。\n
 *        int   whence_   可以是SEEK_SET（文件起始位置）、SEEK_CUR（文件当前位置）或SEEK_END（文件末尾）。\n
 *        off_t len_      从起始位置开始上锁的长度（byte）。如果len_为0，则把起始位置到文件可能的最大偏移量这个区间都上锁。\n
 *        用法示例：假设当前位于文件的第10个字节，则filelock_try_wlock(fd, 2, SEEK_SET, 0);将会把该文件的第2个字节开始，
 *                  一直到到文件可能的最大偏移量这段区间都上锁。这样，无论以后往该文件的末尾添加了多少字节，这些字节都
 *                  属于被上锁的区间。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_try_wlock(fd_, offset_, whence_, len_) \
		filelock_lock((fd_), F_SETLK, F_WRLCK, (offset_), (whence_), (len_))
/**
 * @brief 给整个文件上写锁。如果上不了，则一直阻塞，直到成功，或者发生错误为止。\n
 *        int fd_       文件描述符，需要上写锁的文件。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_wlockfile(fd_) \
		filelock_lock((fd_), F_SETLKW, F_WRLCK, 0, SEEK_SET, 0)
/**
 * @brief 给整个文件上写锁。如果上不了，则马上返回-1。\n
 *        int fd_       文件描述符，需要上写锁的文件。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_try_wlockfile(fd_) \
		filelock_lock((fd_), F_SETLK, F_WRLCK, 0, SEEK_SET, 0)
/**
 * @brief 解锁。
 *        int   fd_       文件描述符，需要解锁的文件。\n
 *        off_t offset_   相对whence_的偏移量（byte）。offset_和whence_共同决定了解锁的起始位置。\n
 *        int   whence_   可以是SEEK_SET（文件起始位置）、SEEK_CUR（文件当前位置）或SEEK_END（文件末尾）。\n
 *        off_t len_      从起始位置开始解锁的长度（byte）。如果len_为0，则把起始位置到文件可能的最大偏移量这个区间都解锁。\n
 *        用法示例：假设当前位于文件的第10个字节，则filelock_unlock(fd, 2, SEEK_CUR, 4);将会把该文件的12到15个字节解锁。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_unlock(fd_, offset_, whence_, len_) \
		filelock_lock((fd_), F_SETLK, F_UNLCK, (offset_), (whence_), (len_))
/**
 * @brief 解锁整个文件。\n
 *        int fd_       文件描述符，需要解锁的文件。\n
 *        返回值：  0成功，-1失败。
 */
#define filelock_unlockfile(fd_) \
		filelock_lock((fd_), F_SETLK, F_UNLCK, 0, SEEK_SET, 0)

#endif // LIBTAOMEE_FILELOCK_H_

