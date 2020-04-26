
// Scanner.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"		// 主符号

#define INIFILENAME ".\\Scanner.ini"
#define LIST_RESULT_INDEX	0
#define LIST_RESULT_IP		1
#define LIST_RESULT_STATE	2
#define LIST_RESULT_OS		3

#include	<winsock2.h>
#include    <WS2tcpip.h>
#pragma comment(lib,"ws2_32")
#include	<afxsock.h>
#include <afxwin.h> 

//(no data part)

// CScannerApp:
// 有关此类的实现，请参阅 Scanner.cpp
//

class CScannerApp : public CWinApp
{
public:
	CScannerApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CScannerApp theApp;
