/**
 *============================================================
 *  @file      constants.h
 *  @brief     定义淘米项目公共常量。
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef TAOMEE_PRJ_CONSTANTS_H_
#define TAOMEE_PRJ_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

enum {
	/*! 起始用户米米号 */
	base_user_id	= 50000,
	/*! 起始游客米米号 */
	base_guest_id	= 2000000001
};

#ifdef LOGIN_DES_KEY
#undef LOGIN_DES_KEY
#endif

/**
  * @brief DES key for encrypting the 'session' to login
  */
#ifdef TW_VER
#define LOGIN_DES_KEY ",.ta0me>"
#else
#define LOGIN_DES_KEY "!tA:mEv,"
#endif

#ifdef __cplusplus
}
#endif

#endif // TAOMEE_PRJ_CONSTANTS_H_

