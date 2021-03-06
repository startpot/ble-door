#ifndef SM4_DPWD_H__
#define SM4_DPWD_H__

#include <stdint.h>

#define SM_DPWD_KEY_LEN_MIN				(128/8)
#define SM_DPWD_CHALLENGE_LEN_MIN		(4)
#define SM_DPWD_LEN_MAX					(10)
#define SM_HASH_OUT_LEN					(32)

#define SM_DPDW_KEY_LEN					(16)		// uint8_ts
#define SM_DPDW_T_LEN					(16)		// uint8_ts


int SM4_DPasswd(uint8_t * pKey, uint64_t Time, uint16_t Interval, uint32_t Counter, \
					uint8_t* pChallenge, uint8_t* pDynPwd);

// SM4的动态口令生成算法输入、输出用例
//				K							 T		 C	 	Q	 P(SM4)
//	1234567890abcdef1234567890abcdef	1340783053	1234  5678	 446720
//	1234567890abcdefabcdef1234567890	1340783416	5621  3698	 049845
//	1234567890abcdef0987654321abcdef	1340783476	2584  2105	 717777
//	87524138025adcfeabfedc2584376195	1340783509	2053  6984	 037000
//	87524138025adcfe2584376195abfedc	1340783588	2058  3024	 502206
//	1234567890abcdefabcdef0987654321	1340783624	2056  2018	 692843
//	adcfe87524138025abfedc2584376195	1340783652	2358  1036	 902690
//	58ade3698fe28dcb6925010dd236caef	1340783829	2547  2058	 499811
//	58ade365201d80cbdd236caef6925010	1340783771	6031  2058	 565180
//	65201d80cb58ade3dd236caef6925010	1340783815	6580  1047	 724654
	
#endif
