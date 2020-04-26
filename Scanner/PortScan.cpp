// PortScan.cpp: 实现文件
//

#include "StdAfx.h"
#include "pch.h"
#include "Scanner.h"
#include "PortScan.h"
#include "afxdialogex.h"
#include "resource.h"
#include <Mstcpip.h>
//#include "sys/socket.h"

#define FINISH 100
#define RUN 200
#define SIO_RCVALL _WSAIOW(IOC_VENDOR,1)
#define SEQ 0x12345678
#define TCP_WINDOW_SIZE 64240
#define RECV_BUF_SIZE	TCP_WINDOW_SIZE

//发送线程结构体
typedef struct PSThreadParamStruct {
	CString	strIP; 
	UINT	uPort;
	//BOOL	bNoOrder;  //是否随机扫描
	CPortScan* ptr;    //指向父线程的对话框
	HTREEITEM	TreeItem;   //指向结果要显示的树型控件节点
}PSTHREADPARAM;

PSTHREADPARAM	PSThreadParam;
UINT	TCPThreadScan(LPVOID pParam);
UINT SYNScan(UNIONIP uIPBegin, UNIONIP uIPEnd, UINT uPortBegin, UINT uPortEnd);
BOOL	g_bPSRunning;   //全局控制线程停止的变量
int		g_iTotalThreadCount;   //当前运行的总线程数
CPortScan* pDlgPortScan;

// CPortScan 对话框

IMPLEMENT_DYNAMIC(CPortScan, CDialogEx)

CPortScan::CPortScan(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOGPORTSCAN, pParent)
	, m_uBeginPort(0)
	, m_uEndPort(0)
{

}

CPortScan::~CPortScan()
{
}

void CPortScan::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESSBeginIP, m_ctlBeginIP);
	DDX_Control(pDX, IDC_IPADDRESSEndIP, m_ctlEndIP);
	DDX_Control(pDX, IDC_TREEResult, m_ctlTreeResult);
	DDX_Text(pDX, IDC_EDITBeginPort, m_uBeginPort);
	DDX_Text(pDX, IDC_EDITEndPort, m_uEndPort);
}


BEGIN_MESSAGE_MAP(CPortScan, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTONStart, &CPortScan::OnBnClickedButtonstart)
	ON_BN_CLICKED(IDC_BUTTONStop, &CPortScan::OnBnClickedButtonstop)
	//ON_BN_CLICKED(IDC_BUTTONStartSYN, &CPortScan::OnBnClickedButtonstartsyn)
END_MESSAGE_MAP()


// CPortScan 消息处理程序
//程序初始化
BOOL CPortScan::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	if (!AfxSocketInit()) {
		MessageBox("Socket初始化失败，请重新运行。");
		return FALSE;
	}

	WSADATA wsaData = { 0 };
	WSAStartup(MAKEWORD(2, 0), &wsaData);

	m_strLocalIP = "192.168.228.128";
	g_uLocalPort = 7000;

	srand((unsigned)time(NULL));
	CString	strTemp;
	char	buff[MAX_PATH];
	GetPrivateProfileString("PortScan", "BeginIP", "192.168.1.105", buff, MAX_PATH, INIFILENAME);
	m_ctlBeginIP.SetWindowText(buff);
	GetPrivateProfileString("PortScan", "EndIP", "192.168.1.105", buff, MAX_PATH, INIFILENAME);
	m_ctlEndIP.SetWindowText(buff);
	m_uBeginPort = GetPrivateProfileInt("PortScan", "BeginPort", 70, INIFILENAME);
	m_uEndPort = GetPrivateProfileInt("PortScan", "EndPort", 80, INIFILENAME);

	UpdateData(false);
	pDlgPortScan = this;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CPortScan::OnBnClickedButtonstart()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	UNIONIP unBeginIP, unEndIP;
	//获取IP地址用于判断是否有意义
	m_ctlBeginIP.GetAddress(unBeginIP.dwDword);
	m_ctlEndIP.GetAddress(unEndIP.dwDword);
	if (unBeginIP.dwDword > unEndIP.dwDword) {
		AfxMessageBox("开始IP大于结束IP！");
		return;
	}
	//判断端口是否有意义
	if (m_uBeginPort < 1 || m_uBeginPort >65535 || m_uEndPort < 1 || m_uEndPort > 65535)
	{
		AfxMessageBox("端口值应在1~65535范围");
		return;
	}
	else if (m_uBeginPort > m_uEndPort) {
		if (IDOK == AfxMessageBox("开始端口大于结束端口，为您交换？", MB_OKCANCEL)) {
			UINT uTempPort = m_uBeginPort;
			m_uBeginPort = m_uEndPort;
			m_uEndPort = uTempPort;
			UpdateData(FALSE);
		}
		else {
			return;
		}
	}

	m_ctlTreeResult.DeleteAllItems();  //清空当前列表内容
	CString strIP, strTemp;  //创建各线程，其中每一个线程扫描一个IP的一个端口
	g_bPSRunning = TRUE;  //开启线程

	for (; unBeginIP.dwDword <= unEndIP.dwDword; unBeginIP.dwDword++) {
		strIP.Format("%d.%d.%d.%d", unBeginIP.ucByte[3], unBeginIP.ucByte[2], unBeginIP.ucByte[1], unBeginIP.ucByte[0]);
		HTREEITEM tcptree = m_ctlTreeResult.InsertItem(strIP,1,1,TVI_ROOT);
		
		PSThreadParam.strIP = strIP;
		PSThreadParam.ptr = this;
		PSThreadParam.TreeItem = tcptree;
		for (UINT port = m_uBeginPort; port <= m_uEndPort; port++) {
			PSThreadParam.uPort = port;
			AfxBeginThread(TCPThreadScan, &PSThreadParam, THREAD_PRIORITY_IDLE);
			Sleep(50);
		}
	}
	GetDlgItem(IDC_BUTTONStart)->EnableWindow(false);
	UpdateData(FALSE);
	return;
}

UINT TCPThreadScan(LPVOID pParam) {
	PSTHREADPARAM* threadParam = (PSTHREADPARAM*)pParam;
	CString strIP = threadParam->strIP;
	UINT uPort = threadParam->uPort;
	CPortScan* ptr = threadParam->ptr;
	HTREEITEM treeItem = threadParam->TreeItem;

	g_iTotalThreadCount++;  //线程计数器

	CSocket sock;
	CString strTemp;

	if (sock.Create()) {	//对己方端口无限制
		if (sock.Connect(strIP, uPort)) {	//连接对方IP地址的端口
			
			if (uPort < 1024) {
				//对于熟知端口，直接使用协议内置的名称
				struct servent* se;
				se = getservbyport(htons(uPort), "tcp");
				if (se != NULL) {
					strTemp.Format("%d %s", uPort, se->s_name);
				}
				else {
					strTemp.Format("%d", uPort);
				}
			}
			else {
				//对于非熟知端口，采用搜集到的名称
				strTemp.Format("%d %s", uPort, ptr->GetPortName(uPort));
			}
			//将读到的值插入树控件记忆的位置
			//AfxMessageBox(strTemp);
			ptr->m_ctlTreeResult.InsertItem(strTemp,2,2,treeItem);
			sock.ShutDown(2);  //关闭连接
		}
		/*CString err;
		err.Format("错误码：%d", GetLastError());
		AfxMessageBox(err);*/

		sock.Close();
	}
	g_iTotalThreadCount--;

	/*PSTHREADPARAM* threadParam = (PSTHREADPARAM*)pParam;
	CString strIP = threadParam->strIP;
	UINT uPort = threadParam->uPort;
	CPortScan* ptr = threadParam->ptr;
	HTREEITEM treeItem = threadParam->TreeItem;
	g_iTotalThreadCount++;  //线程计数器

	SOCKET sock = -1;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == SOCKET_ERROR) {
		AfxMessageBox("Error in create listen socket\n");
		return 1;
	}

	SOCKADDR_IN addr_inr;
	addr_inr.sin_family = AF_INET;
	addr_inr.sin_port = htons(uPort);				//对方端口
	addr_inr.sin_addr.s_addr = inet_addr(strIP);	//对方IP


	if (connect(sock, (PSOCKADDR)&addr_inr, sizeof(addr_inr)) != 0) {
		CString err;
		err.Format("错误码：%d", GetLastError());
		AfxMessageBox(err);

		g_iTotalThreadCount--;
		closesocket(sock);
		return 2;
	}

	CString strTemp;
	if (uPort < 1024) {
	//对于熟知端口，直接使用协议内置的名称
		struct servent* se;
		se = getservbyport(htons(uPort), "tcp");
		if (se != NULL) {
			strTemp.Format("%d %s", uPort, se->s_name);
		}
		else {
			strTemp.Format("%d", uPort);
		}
	}
	else {
	//对于非熟知端口，采用搜集到的名称
		strTemp.Format("%d %s", uPort, ptr->GetPortName(uPort));
	}
	//将读到的值插入树控件记忆的位置
	ptr->m_ctlTreeResult.InsertItem(strTemp, 2, 2, treeItem);
	shutdown(sock,2);  //关闭连接
	closesocket(sock);

	g_iTotalThreadCount--;*/
return 0;
}

CString CPortScan::GetPortName(UINT uPort)
{
	// TODO: 在此处添加实现代码.
	CString strRet;
	switch (uPort) {
	case 1026:strRet = "常用动态分配的TCP端口"; break;
	case 1080:strRet = "WinGate,IRC等防火墙管道"; break;
	case 1243:strRet = "Sub-7木马"; break;
	case 1433:strRet = "MSSQL数据库服务端口"; break;
	case 3128:strRet = "Squid HTTP代理服务器"; break;
	case 3306:strRet = "MySql数据库服务端口"; break;
	case 5632:strRet = "pcAnywhere所用端口"; break;
	default:strRet = ""; break;
	}
	return strRet;
}

void CPortScan::OnBnClickedButtonstop()
{
	// TODO: 在此添加控件通知处理程序代码
	AfxMessageBox("已结束扫描", MB_OK);
	GetDlgItem(IDC_BUTTONStart)->EnableWindow(true);
	g_bPSRunning = false;
}

