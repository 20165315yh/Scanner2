// SQLScan.cpp: 实现文件
//
//#include "stdafx.h"
#include "pch.h"
#include "Scanner.h"
#include "SQLScan.h"
#include "afxdialogex.h"

#define INIFILENAME ".\\PortScan.ini"

#include <AFXINET.H>
#define MAXRECEIVENUM		(60*1000)

#include <winsock2.h> 
#pragma comment(lib,"ws2_32")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef struct ThreadParamStruct
{
	CString strIP;		//主机IP地址
	UINT uPort;			//端口地址
	CString strURL;		//URL地址
	CString strObject;	//除服务器之外，包含路径在内的页面
	CSQLScan* ptr;		//界面指针
	CString strParam;	//变量参数
	CString strFilter;	//成功词
}THREADPARAM;
THREADPARAM ThreadParam;

UINT ThreadInjectScan(LPVOID pParam);

// CSQLScan 对话框

IMPLEMENT_DYNAMIC(CSQLScan, CDialogEx)

CSQLScan::CSQLScan(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOGSQLSCAN, pParent)
{
m_strURL = (_T(""));
m_strURLParam = (_T(""));
m_strSQLSign = (_T(""));
m_strSendPack=(_T(""));
m_strRecievePack=(_T(""));
m_strResult=(_T(""));
}
CSQLScan::~CSQLScan()
{
}

void CSQLScan::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDITURL, m_strURL);
	DDX_Text(pDX, IDC_EDITURLParam, m_strURLParam);
	DDX_Text(pDX, IDC_EDITSQLSign, m_strSQLSign);
	DDX_Text(pDX, IDC_EDITSendPack, m_strSendPack);
	DDX_Text(pDX, IDC_EDITRecievePack, m_strRecievePack);
	DDX_Text(pDX, IDC_EDITResult, m_strResult);
}


BEGIN_MESSAGE_MAP(CSQLScan, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTONSQLScanStart, &CSQLScan::OnBnClickedButtonsqlscanstart)
	ON_BN_CLICKED(IDC_BUTTONSQLScanStop, &CSQLScan::OnBnClickedButtonsqlscanstop)
END_MESSAGE_MAP()


// CSQLScan 消息处理程序

BOOL CSQLScan::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	WSAData wsaData;
	WSAStartup(WINSOCK_VERSION, &wsaData);

	CString strTemp;
	char buff[MAX_PATH];

	UpdateData(TRUE);
	GetPrivateProfileString("Param", "URL", "", buff, MAX_PATH, INIFILENAME);
	m_strURL.Format("%s", buff);
	GetPrivateProfileString("Param", "Param", "", buff, MAX_PATH, INIFILENAME);
	m_strURLParam.Format("%s", buff);
	GetPrivateProfileString("Param", "Filter", "", buff, MAX_PATH, INIFILENAME);
	m_strSQLSign.Format("%s", buff);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CSQLScan::OnBnClickedButtonsqlscanstart()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	DWORD dwServiceType;
	INTERNET_PORT nPort;
	CString strServer;
	CString strObject;
	CString strUsername;
	CString strPassword;
	DWORD dwFlags = 0;

	//解析URL，判断格式是否符合要求
	//同时将URL拆分成几个部分
	if (!AfxParseURLEx(m_strURL, dwServiceType, strServer, strObject, nPort, strUsername, strPassword, dwFlags))
	{
		MessageBox("URL格式不对。", "操作提示", MB_OK | MB_ICONERROR);
		return;
	}
	//为避免二义性，要求URL中一些符号换成二进制表示形式
	CString strParam = m_strURLParam;
	strParam.Replace("'", "%27");
	strParam.Replace(" ", "%20");
	//开启线程，进行扫描
	CString strTemp;
	ThreadParam.strIP = strServer;
	ThreadParam.ptr = this;
	ThreadParam.uPort = nPort;
	ThreadParam.strURL = m_strURL;
	ThreadParam.strObject = strObject;
	ThreadParam.strParam = strParam;
	ThreadParam.strFilter = m_strSQLSign;
	AfxBeginThread(ThreadInjectScan, &ThreadParam, THREAD_PRIORITY_IDLE);
	UpdateData(FALSE);

	GetDlgItem(IDC_BUTTONSYNSCAN)->EnableWindow(false);
}


UINT ThreadInjectScan(LPVOID pParam)
{
	//接收所有参数
	THREADPARAM* threadparam = (THREADPARAM*)pParam;
	CString strIP = threadparam->strIP;
	UINT uPort = threadparam->uPort;
	CSQLScan* ptr = threadparam->ptr;
	CString strURL = threadparam->strURL;
	CString strObject = threadparam->strObject;
	CString strParam = threadparam->strParam;
	CString strFilter = threadparam->strFilter;

	//创建一个socket
	SOCKET sock = -1;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == SOCKET_ERROR)
	{
		AfxMessageBox("创建SOCKET出错。\n");
		return 1;
	}

	//设置几个参数。
	int iTimeOut = 1000;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&iTimeOut, sizeof(iTimeOut)) == SOCKET_ERROR)
	{
		AfxMessageBox("发送时间溢出设置出错：SO_SNDTIMEO!", MB_OK | MB_ICONINFORMATION);
		closesocket(sock);
		return 2;
	}

	iTimeOut = 1000;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeOut, sizeof(iTimeOut)) == SOCKET_ERROR)
	{
		AfxMessageBox("接收时间溢出设置出错：SO_SNDTIMEO!", MB_OK | MB_ICONINFORMATION);
		closesocket(sock);
		return 3;
	}

	//连接对方的默认端口。
	SOCKADDR_IN addr_remote;
	addr_remote.sin_family = AF_INET;
	addr_remote.sin_addr.s_addr = inet_addr(strIP);
	addr_remote.sin_port = htons(uPort);
	//连接对方
	if (connect(sock, (PSOCKADDR)&addr_remote, sizeof(addr_remote)) != 0)
	{
		AfxMessageBox("Connect出错，对方未提供WWW服务。!", MB_OK | MB_ICONINFORMATION);
		closesocket(sock);
		return 4;
	}
	//发送指定格式的数据包
	CString strTemp;
	CString strSend = "", strReceive = "", strResult = "";

	strSend.Format("POST /inject/injectcenter.asp?%s HTTP/1.0\r\n", strParam);
	strSend += strTemp;	//URL地址传变量
	strSend += "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, application/vnd.ms-powerpoint, */*\r\n";
	strSend += "Accept-Language: zh-cn\r\n";
	strSend += "Accept-Encoding: gzip, deflate\r\n";
	strSend += "User-Agent: Mozilla/4.0\r\n";
	strTemp.Format("Content-Length: %d\r\n", strParam.GetLength());
	strSend += strTemp;	//不含头域的内容长度
	strSend += "Host: 127.0.0.1\r\n";
	strSend += "Content-Type: application/x-www-form-urlencoded\r\n";
	strSend += "\r\n";
	strSend += strParam;//以表单的方式传递参数
	//发送数据包
	send(sock, strSend.GetBuffer(0), strSend.GetLength(), 0);
	//接收IP包
	UCHAR RecvBuf[MAXRECEIVENUM];//IP包最大为64K左右，所以定义一个大的缓冲区。
	int iLenRet = recv(sock, (char*)RecvBuf, MAXRECEIVENUM, 0);
	if (iLenRet > 0)
	{
		RecvBuf[iLenRet] = '\0';
		strReceive.Format("%s", RecvBuf);
		//根据成功词判断是否有漏洞
		if (strReceive.Find(strFilter) >= 0)
		{
			strResult = "有注入式漏洞！";
		}
		else
		{
			strResult = "没有注入式漏洞！";
		}
	}
	else
	{
		strResult = "网络不通！";
	}

	ptr->ShowSendAndReceive(strSend, strReceive, strResult);

	shutdown(sock, 2);
	closesocket(sock);


	return 0;
}

void CSQLScan::OnBnClickedButtonsqlscanstop()
{
	// TODO: 在此添加控件通知处理程序代码
		WSACleanup();
		CDialog::OnCancel();

		AfxMessageBox("已结束扫描", MB_OK);
		GetDlgItem(IDC_BUTTONSYNSCAN)->EnableWindow(true);
}

void CSQLScan::ShowSendAndReceive(CString strSend, CString strReceive, CString strResult)
{
	// TODO: 在此处添加实现代码.
	/*UpdateData(TRUE);
	
	m_strSendPack = strSend;
	m_strRecievePack = strReceive;
	m_strResult = strResult;
	UpdateData(FALSE);*/

	SetDlgItemText(IDC_EDITSendPack, strSend);
	SetDlgItemText(IDC_EDITRecievePack, strReceive);
	SetDlgItemText(IDC_EDITResult, strResult);
}