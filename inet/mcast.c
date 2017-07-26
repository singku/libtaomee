// Standard C Headers
#include <assert.h>
#include <errno.h>
#include <string.h>
// POSIX Headers
#include <netdb.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "inet_utils.h"
#include "mcast.h"

static inline int family_to_level(int family);

int create_mcast_socket(const char* mcast_addr, const char* port, const char* nif,
								mcast_flag_t flag, struct sockaddr_storage* ss, socklen_t* addrlen)
{
	// forbid any IP address other than those begin with 239
	if (strncmp(mcast_addr, "239", 3) != 0) {
		errno = EADDRNOTAVAIL;
		return -1;
	}

	// convert ip and port to numeric form
	struct addrinfo  hints;
	struct addrinfo* res = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags    = 0;
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	int err = getaddrinfo(mcast_addr, port, &hints, &res);
	if (err != 0) {
		errno = eai_to_errno(err);
		return -1;
	}

	int mcastfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (mcastfd < 0) {
		err = errno;
		freeaddrinfo(res);
		errno = err;
		return -1;
	}

	if (flag & mcast_rdonly) {
		// to make use of multicast, we must set on the SO_REUSEADDR attribute of a socket fd 
		int reuse_addr = 1;	
		err = setsockopt(mcastfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
		if (err < 0) {
			goto ret;
		}

		err = bind(mcastfd, res->ai_addr, res->ai_addrlen);
		if (err < 0) {
			goto ret;
		}

		// Join a Multicast Group
		err = mcast_join(mcastfd, res->ai_addr, res->ai_addrlen, nif);
		if (err < 0) {
			goto ret;
		}
	}

	if (flag & mcast_wronly) {
		// Set Default Interface For Outgoing Multicast Packets
		err = mcast_set_if(mcastfd, res->ai_family, nif);
		if (err < 0) {
			goto ret;
		}
	}

	if (ss) {
		memcpy(ss, res->ai_addr, res->ai_addrlen);
		*addrlen = res->ai_addrlen;
	}

ret:
	if (err) {
		err     = errno;
		close(mcastfd);
		mcastfd = -1;
	}
	freeaddrinfo(res);
	errno = err;

	return mcastfd;
}

int mcast_join(int sockfd, const struct sockaddr* grp, socklen_t grplen, const char* ifname)
{
	struct group_req req;

	assert(grplen <= sizeof(req.gr_group));

	req.gr_interface = if_nametoindex(ifname);
	if (req.gr_interface == 0) {
		errno = ENXIO; /* i/f name not found */
		return -1;
	}

	memcpy(&req.gr_group, grp, grplen);
	return setsockopt(sockfd, family_to_level(grp->sa_family), MCAST_JOIN_GROUP, &req, sizeof(req));
}

int mcast_leave(int sockfd, const struct sockaddr* grp, socklen_t grplen, const char* ifname)
{
	struct group_req req;

	assert(grplen <= sizeof(req.gr_group));

	req.gr_interface = if_nametoindex(ifname);
	if (req.gr_interface == 0) {
		errno = ENXIO; /* i/f name not found */
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return setsockopt(sockfd, family_to_level(grp->sa_family),	MCAST_LEAVE_GROUP, &req, sizeof(req));
}

int mcast_set_if(int sockfd, int family, const char* ifname)
{
	switch (family) {
	case AF_INET:
	{
		struct ifreq ifreq;

		strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
		if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0) {
			return -1;
		}

		struct in_addr*	inaddr = &((struct sockaddr_in*)&ifreq.ifr_addr)->sin_addr;
		return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, inaddr, sizeof(*inaddr));
	}
	case AF_INET6:
	{
		u_int idx = if_nametoindex(ifname);
		if (idx == 0) {
			errno = ENXIO; /* i/f name not found */
			return -1;
		}

		return setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &idx, sizeof(idx));
	}
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}

int mcast_get_if(int sockfd, int family)
{
	switch (family) {
	case AF_INET:
	{
		/* TODO: similar to mcast_set_if() */
		return(-1);
	}
	case AF_INET6:
	{
		u_int		idx;
		socklen_t	len;

		len = sizeof(idx);
		if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
					   &idx, &len) < 0)
			return(-1);
		return(idx);
	}
	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
}

int mcast_set_loop(int sockfd, int family, int onoff)
{
	switch (family) {
	case AF_INET:
	{
		u_char flag = onoff;
		return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, sizeof(flag));
	}
	case AF_INET6:
	{
		u_int flag = onoff;
		return setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &flag, sizeof(flag));
	}
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}

int mcast_get_loop(int sockfd, int family)
{
	switch (family) {
	case AF_INET:
	{
		u_char    flag;
		socklen_t len = sizeof(flag);

		if (getsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, &len) < 0) {
			return -1;
		}
		return flag;
	}
	case AF_INET6:
	{
		u_int     flag;
		socklen_t len = sizeof(flag);
		if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &flag, &len) < 0) {
			return -1;
		}
		return flag;
	}
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}

int mcast_set_ttl(int sockfd, int family, int ttl)
{
	switch (family) {
	case AF_INET:
	{
		u_char val = ttl;
		return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &val, sizeof(val));
	}
	case AF_INET6:
	{
		int hop = ttl;
		return setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hop, sizeof(hop));
	}
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}

int mcast_get_ttl(int sockfd, int family)
{
	switch (family) {
	case AF_INET:
	{
		u_char    ttl;
		socklen_t len = sizeof(ttl);
		if (getsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, &len) < 0) {
			return -1;
		}
		return ttl;
	}
	case AF_INET6:
	{
		int       hop;
		socklen_t len = sizeof(hop);
		if (getsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hop, &len) < 0) {
			return -1;
		}
		return hop;
	}
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}

int mcast_block_source(int sockfd, const struct sockaddr* src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen)
{
	struct group_source_req req;

	assert((grplen <= sizeof(req.gsr_group)) && (srclen <= sizeof(req.gsr_source)));

	req.gsr_interface = 0;
	memcpy(&req.gsr_group, grp, grplen);
	memcpy(&req.gsr_source, src, srclen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),	MCAST_BLOCK_SOURCE, &req, sizeof(req)));
}
 
int mcast_unblock_source(int sockfd, const struct sockaddr* src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen)
{
	struct group_source_req req;

	assert((grplen <= sizeof(req.gsr_group)) && (srclen <= sizeof(req.gsr_source)));

	req.gsr_interface = 0;
	memcpy(&req.gsr_group, grp, grplen);
	memcpy(&req.gsr_source, src, srclen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),	MCAST_UNBLOCK_SOURCE, &req, sizeof(req)));
}
 
int mcast_join_source_group(int sockfd, const struct sockaddr* src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen, const char* ifname)
{
	assert(ifname);

	struct group_source_req req;

	assert((grplen <= sizeof(req.gsr_group)) && (srclen <= sizeof(req.gsr_source)));

	req.gsr_interface = if_nametoindex(ifname);
	if (req.gsr_interface == 0) {
		errno = ENXIO; /* i/f name not found */
		return -1;
	}

	memcpy(&req.gsr_group, grp, grplen);
	memcpy(&req.gsr_source, src, srclen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),	MCAST_JOIN_SOURCE_GROUP, &req, sizeof(req)));
}
 
int mcast_leave_source_group(int sockfd, const struct sockaddr *src, socklen_t srclen, const struct sockaddr* grp, socklen_t grplen)
{
	struct group_source_req req;

	assert((grplen <= sizeof(req.gsr_group)) && (srclen <= sizeof(req.gsr_source)));

	req.gsr_interface = 0;
	memcpy(&req.gsr_group, grp, grplen);
	memcpy(&req.gsr_source, src, srclen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),	MCAST_LEAVE_SOURCE_GROUP, &req, sizeof(req)));
}

//-------------------------------------------------------
// utils
//
// translate the given address family to its corresponding level
static inline int family_to_level(int family)
{
	int level = -1;

	switch (family) {
	case AF_INET:
		level = IPPROTO_IP;
		break;
	case AF_INET6:
		level = IPPROTO_IPV6;
		break;
	default:
		break;
	}

	return level;
}

