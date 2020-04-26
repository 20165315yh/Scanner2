#if !defined(AFX_STDAFX_H__77844DEE_B96F_4DB6_BFA7_39B3A7CD0A1F__INCLUDED_)
#define AFX_STDAFX_H__77844DEE_B96F_4DB6_BFA7_39B3A7CD0A1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#define INIFILENAME ".\\Scanner.ini"
#define LIST_RESULT_INDEX	0
#define LIST_RESULT_IP		1
#define LIST_RESULT_STATE	2
#define LIST_RESULT_OS		3

#include	<winsock2.h>

#pragma comment(lib,"ws2_32")
#include	<afxsock.h>

typedef union MultiByteStruct
{
	int iInt;
	float fFloat;
	UINT uInt;
	ULONG uLong;
	DWORD dwDword;
	WORD wWord[2];
	UCHAR ucByte[4];
}UNIONIP, * PUNIONIP;

typedef struct iphdr {
	UCHAR	ver_pack;
	UCHAR	tos;
	USHORT	total_len;
	USHORT	ident;
	USHORT	frag_and_flags;
	UCHAR	ttl;
	UCHAR	proto;
	USHORT	checksum;
	UINT	sourceIP;
	UINT	desrIP;
}IPHEADER, * PIPHEADER;

typedef struct tcphdr //定义TCP首部
{
	USHORT th_sport; //16位源端口
	USHORT th_dport; //16位目的端口
	unsigned int th_seq; //32位序列号
	unsigned int th_ack; //32位确认号
	USHORT usHeaderLenAndFlag; //4位首部长度/6位保留字/6位标志位
	//unsigned char th_flag; //6位标志位
	USHORT th_win; //16位窗口大小
	USHORT th_sum; //16位校验和
	USHORT th_urp; //16位紧急数据偏移量
}TCPHEADER, * PTCPHEADER;

typedef struct psd_hdr {
	unsigned long saddr;			//源地址
	unsigned long daddr;			//目的地址
	char mbz;
	char ptcl;						//协议类型
	unsigned short tcpl;			//TCP长度
}PSDHEADER;

typedef struct udphdr //定义UDP首部
{
	unsigned short uh_sport;     //16位源端口
	unsigned short uh_dport;     //16位目的端口
	unsigned short uh_len;           //16位长度
	unsigned short uh_sum;    //16位校验和
} UDPHEADER;

typedef struct icmpher {
	BYTE	i_type;
	BYTE	i_code;
	USHORT	i_cksum;
	USHORT	i_id;
	USHORT	i_seq;
	UCHAR	buff[32];
}ICMPHEADER, * PICMPHEADER;

typedef struct tcp_check_sum {
	PSDHEADER tcpPsdHeader;
	TCPHEADER tcpHeader;
}TCP_CHECK_SUM, * PTCP_CHECK_SUM;

struct SEND_DATA {
	IPHEADER ipHeader;
	TCPHEADER tcpHeader;
};

CString IPIntToStr(UINT IPInt);
UINT IPStrToInt(CString IPStr);
UINT16 getCheckSum(UINT8* pBuf, UINT32 uLen);
IPHEADER getIpHeader(UINT32 uihostIPSRC, UINT32 uihostIPDST, UINT16 ushostIdentification);
TCPHEADER getTCPHeader(const PIPHEADER const pIpHeader, UINT16 uihostSrcPort, UINT16 uihostDstPort);

CString IPIntToStr(UINT IPInt);
UINT IPStrToInt(CString IPStr);
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#endif // !defined(AFX_STDAFX_H__77844DEE_B96F_4DB6_BFA7_39B3A7CD0A1F__INCLUDED_)
#endif
#endif