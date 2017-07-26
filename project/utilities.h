/**
 *============================================================
 *  @file      utilities.h
 *  @brief     淘米项目公共小函数。
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef TAOMEE_PRJ_UTILITIES_H_
#define TAOMEE_PRJ_UTILITIES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#include <libtaomee/crypt/qdes.h>

#include "constants.h"
#include "types.h"

/**
  * @brief check if the given id is a guest id
  * @param uid
  * @return 0 if uid is a valid user id, 1 if uid is a guest id
  */
static inline int is_guest_id(userid_t uid)
{
	return (uid >= base_guest_id);
}

/**
  * @brief check if the given 'uid' is a valid user id
  * @param uid user id
  * @return 1 if uid is a valid user id, 0 otherwise
  */
static inline int is_valid_uid(userid_t uid)
{
	return ((uid < base_guest_id) && (uid >= base_user_id));
}

/**
  * @brief encrypt a login session
  * @param buf the session will be encrypt into buf, must be equal to or greater than 16 bytes
  * @param buflen length of the buf
  * @param uid user id
  * @param ip ipaddress of the user
  * @return length of the session
  */
static inline int encrypt_login_session(void* buf, int buflen, userid_t uid, uint32_t ip)
{
	uint32_t inbuf[4];

	inbuf[0] = ip;
	inbuf[1] = time(0);
	inbuf[2] = uid;
	inbuf[3] = inbuf[1];

	des_encrypt_n(LOGIN_DES_KEY, inbuf, buf, 2);

	return sizeof(inbuf);
}

/**
  * @brief verify a login session
  * @param uid user id
  * @param sess login session
  * @return 0 on success, -1 on failure
  */
static inline int verify_login_session(userid_t uid, const void* sess)
{
	uint32_t outbuf[4];
	des_decrypt_n(LOGIN_DES_KEY, sess, outbuf, 2);

	time_t diff = time(0) - outbuf[1];

	// user id
	if ((outbuf[2] != uid) || (outbuf[1] != outbuf[3]) || (diff > 1800) || (diff < -1800)) {
		return -1;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif // TAOMEE_PRJ_UTILITIES_H_

