// CHostScan.cpp: 实现文件
//

#include "StdAfx.h"
#include "pch.h"
#include "Scanner.h"
#include "afxdialogex.h"
#include "HostScan.h"


#define	ICMP_ECHO		8;
#define	ICMP_ECHOREPLY	0;
#define	IPHEADLEN		sizeof(IPHEADER)
#define ICMPHEADLEN		sizeof(ICMPHEADER)
#define	ICMPHEADMINLEN	8
#define	IPICMPHEADLEN	(IPHEADLEN+ICMPHEADLEN)

#define	SIO_RCVALL		_WSAIOW(IOC_VENDOR,1)
#define	STATUS_FALLED	0xFFFF

//线程初始化时的结构
typedef	struct HSThreadParamStruct {
	UCHAR		a, b, c, d;
	int			index;
	CHostScan * ptr;
}HSTHREADPARAM;


//线程声明
HSTHREADPARAM	ThreadParam;
UINT	ThreadRawReceive(LPVOID pParam);
UINT	ThreadHostScanStart(LPVOID pParam);
BOOL	g_bRunning;			//线程是否运行
BOOL	g_bReceiving;		//线程是否需要对接收的内容进行判断
int		g_iItemCount;		//项目数
CHostScan *pDlgHostScan;
BOOL	bHostScan;

// CHostScan 对话框

IMPLEMENT_DYNAMIC(CHostScan, CDialogEx)

CHostScan::CHostScan(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOGHOSTSCAN, pParent)
	//, m_strIPBegin(_T(""))
	//, m_strIPEnd(_T(""))
{

}

CHostScan::~CHostScan()
{
}

void CHostScan::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTResult, m_ctlListResult);
	DDX_Control(pDX, IDC_IPADDRESSBegin, m_strIPBegin);
	DDX_Control(pDX, IDC_IPADDRESSEnd, m_strIPEnd);
}


BEGIN_MESSAGE_MAP(CHostScan, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTONStartScan, &CHostScan::OnBnClickedButtonstartscan)
	ON_BN_CLICKED(IDC_BUTTONStopScan, &CHostScan::OnBnClickedButtonstopscan)
END_MESSAGE_MAP()


// CHostScan 消息处理程序
//程序初始化
BOOL CHostScan::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化代码
		bHostScan = false;
		WSADATA	wsaData;
		if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0)
		{
			CString	strTemp;
			strTemp.Format("初始化失败，请重新运行。错误码：%d", GetLastError());
			MessageBox(strTemp, "出错提示");
			ExitProcess(0);
		}
		char	sLocalName[64] = { 0 };								
		gethostname((char*)sLocalName, sizeof(sLocalName) - 1);		//获取本机名
		hostent* pHost = gethostbyname(sLocalName);
		m_strLocalIP.Format("%s", inet_ntoa(*(struct in_addr*)pHost->h_addr_list[0]));	//获取本地IP地址
		//创建列表表头
		m_ctlListResult.InsertColumn(LIST_RESULT_INDEX, "序号", LVCFMT_LEFT, 100);
		m_ctlListResult.InsertColumn(LIST_RESULT_IP, "IP", LVCFMT_LEFT, 200);
		m_ctlListResult.InsertColumn(LIST_RESULT_STATE, "状态", LVCFMT_LEFT, 100);
		m_ctlListResult.InsertColumn(LIST_RESULT_OS, "操作系统", LVCFMT_LEFT, 430);
		m_ctlListResult.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);		//单击列表可以选中一行
		//初始化起始IP地址
		char buff[MAX_PATH];
		GetPrivateProfileString("Init", "BeginIP", "192.168.228.120", buff, MAX_PATH, INIFILENAME);
		m_strIPBegin.SetWindowText(buff);
		GetPrivateProfileString("Init", "EndIP", "192.168.228.130", buff, MAX_PATH, INIFILENAME);
		m_strIPEnd.SetWindowText(buff);
		//启动接收线程
		ThreadParam.ptr = this;
		g_bRunning = TRUE;
		g_bReceiving = FALSE;
		AfxBeginThread(ThreadRawReceive, &ThreadParam, THREAD_PRIORITY_IDLE);
		Sleep(500);

		UpdateData(FALSE);
		pDlgHostScan = this;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

//接收线程
UINT ThreadRawReceive(LPVOID pParam)
{
	HSTHREADPARAM* threadparam = (HSTHREADPARAM*)pParam;	//接收主线程传来的参数
	CHostScan* ptr = threadparam->ptr;						//可以使用CHostScan类的所有变量
	CListCtrl* pList = (CListCtrl*)&ptr->m_ctlListResult;	//CListCtrl控件使用频繁，直接使用指针，方便使用
	
	//使用RAW Socket
	SOCKET sock;
	SOCKADDR_IN addr_local, addr_remote;
	int buflen = sizeof(addr_remote);
	if ((sock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		AfxMessageBox("原始套接字设出错！\n", MB_OK | MB_ICONINFORMATION);
		return 1;
	}
	
	BOOL flag = true;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(flag)) == SOCKET_ERROR)
	{
		AfxMessageBox("套接字选项IP_HDRINCL设置出错！\n", MB_OK | MB_ICONINFORMATION);
		return 2;
	}
	
	int nTimeOver = 500;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&nTimeOver, sizeof(nTimeOver)) == SOCKET_ERROR)
	{
		AfxMessageBox("超时时间设置出错！\n", MB_OK | MB_ICONINFORMATION);
		return 3;
	}

	int localPort = GetPrivateProfileInt("Init", "LocalPort", 5000, INIFILENAME);
	addr_local.sin_family = AF_INET;
	addr_local.sin_port = htons(localPort);
	addr_local.sin_addr.S_un.S_addr = inet_addr(ptr->m_strLocalIP);

	if (bind(sock, (PSOCKADDR)&addr_local, sizeof(addr_local)) == SOCKET_ERROR)
	{
		AfxMessageBox("绑定口出错！\n", MB_OK | MB_ICONINFORMATION);
		return 4;
	}
	
	//将网卡设置为混杂模式
	DWORD dwBufferLen[10];
	DWORD dwBufferInLen = 1;
	DWORD dwByteReturned = 0;
	if (WSAIoctl(sock, SIO_RCVALL, &dwBufferInLen, sizeof(dwBufferInLen),
		&dwBufferLen, sizeof(dwBufferLen), &dwByteReturned, NULL, NULL) == SOCKET_ERROR)
	{
		AfxMessageBox("混杂模式设置出错！\n", MB_OK | MB_ICONINFORMATION);
		return 5;
	}
	
	//线程进入接收状态
	UCHAR RecvBuf[65000];
	PIPHEADER ipPtr = (PIPHEADER)RecvBuf;
	CString strTemp;
	
	while (g_bRunning) {
		int ret = recvfrom(sock, (char*)RecvBuf, sizeof(RecvBuf), 0,(LPSOCKADDR)&addr_remote, &buflen);
		if (ret == SOCKET_ERROR) {
			continue;
		}

		if (!g_bReceiving)
		{
			Sleep(10);
			continue;
		}
		
		//依次分析每个源ip地址，接到意味着该主机是开放的
		UNIONIP ip;
		ip.uInt = ipPtr->sourceIP;	//从IP包头中提取源IP地址
	
		for (int i = 0; i < g_iItemCount; i++) {
			//在表中依次查找，如果找到了则显示通
			if (pList->GetItemData(i) != (DWORD)ip.uInt)
				continue;
			pList->SetItemText(i, LIST_RESULT_STATE, "通");
			//根据TTL值判断操作系统
			UCHAR ucTTL = ipPtr->ttl;
			CString strTemp = "无法判断";
			if (ucTTL <= 32) {
				strTemp = "Windows 95/98/Me";
			}
			else if (ucTTL <= 64) {
				strTemp = "Linux";
			}
			else if (ucTTL <= 128) {
				strTemp = "Windows NT";
			}
			else {
				strTemp = "UNIX";
			}
			pList->SetItemText(i, LIST_RESULT_OS, strTemp);
			break;
		}
	}
	shutdown(sock, 2);
	closesocket(sock);
	return 0;
}

void CHostScan::OnBnClickedButtonstartscan()
{
	// TODO: 在此添加控件通知处理程序代码
	
	UpdateData(true);
	//清空列表数据
	m_ctlListResult.DeleteAllItems();

	AfxBeginThread(ThreadHostScanStart, &ThreadParam, THREAD_PRIORITY_IDLE);
}

//扫描函数
UINT ThreadHostScanStart(LPVOID pParam) {
	HSTHREADPARAM* threadparam = (HSTHREADPARAM*)pParam;
	CHostScan* ptr = threadparam->ptr;
	CListCtrl* pList = (CListCtrl*)&ptr->m_ctlListResult;



	//ping期间，按钮无效
	ptr->GetDlgItem(IDC_BUTTONStartScan)->EnableWindow(FALSE);

	SOCKET sockSend = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sockSend == INVALID_SOCKET) {
		AfxMessageBox("RAW SOCKET 创建失败");
		return 1;
	}

	CString strIP, strTemp;
	int iCount = 0;
	UNIONIP uBeginIP, uEndIP;
	ptr->m_strIPBegin.GetAddress(uBeginIP.dwDword);
	ptr->m_strIPEnd.GetAddress(uEndIP.dwDword);
	//
	while (uBeginIP.dwDword <= uEndIP.dwDword && iCount < 1000) {
		strTemp.Format("%d", iCount + 1);
		ptr->m_ctlListResult.InsertItem(iCount, strTemp);
		strIP.Format("%d.%d.%d.%d", uBeginIP.ucByte[3], uBeginIP.ucByte[2], uBeginIP.ucByte[1], uBeginIP.ucByte[0]);
		ptr->m_ctlListResult.SetItemText(iCount, LIST_RESULT_IP, strIP);//写值显示
		ptr->m_ctlListResult.SetItemText(iCount, LIST_RESULT_STATE, "");
		ptr->m_ctlListResult.SetItemData(iCount, IPStrToInt(strIP));//绑定，方便获取
		uBeginIP.dwDword++;
		iCount++;
	}
	g_iItemCount = iCount;

	int timeout = 1000;
	setsockopt(sockSend, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	setsockopt(sockSend, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

	//填充SOCKADDR_IN结构
	sockaddr_in addr_inr, addr_in1;
	addr_in1.sin_family = AF_INET;
	addr_in1.sin_port = htons(54321);
	addr_in1.sin_addr.S_un.S_addr = inet_addr(ptr->m_strLocalIP);
	
	addr_inr.sin_family = AF_INET;
	addr_inr.sin_port = htons(12345);
	//设置IP头操作设置
	int flag = 1;
	setsockopt(sockSend, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(flag));
	//将原始套接字绑定到本地网卡地址上
	if (bind(sockSend, (PSOCKADDR)&addr_in1, sizeof(addr_in1)) == SOCKET_ERROR)
		return 2;

	g_bReceiving = TRUE;
	char sendBuf[1024] = { 0 };
	PIPHEADER ipHeader = (PIPHEADER)sendBuf;
	PICMPHEADER icmpHeader = (PICMPHEADER)&sendBuf[IPHEADLEN];
	char* dataHeader = (char*)icmpHeader->buff;
	CString strData = "abcdefghijklmnopqrstuvwabcdefghi";
	strcpy(dataHeader, strData);

	ipHeader->ver_pack = 0x45;
	ipHeader->tos = 0;
	ipHeader->total_len = htons(IPICMPHEADLEN);
	ipHeader->ident = 0;
	ipHeader->ttl = 128;
	ipHeader->proto = IPPROTO_ICMP;
	ipHeader->sourceIP = addr_in1.sin_addr.S_un.S_addr;

	icmpHeader->i_type = ICMP_ECHO;
	icmpHeader->i_code = 0;
	icmpHeader->i_id = 0x0;
	icmpHeader->i_seq = 0x0;
	icmpHeader->i_cksum = 0;
	icmpHeader->i_cksum = ptr->Checksum((USHORT*)icmpHeader, 60 - IPHEADLEN);

	for (int times = 0; times < 1; times++) {
		for (int i = 0; i < iCount; i++) {
			if (!g_bReceiving)
				break;

			strTemp = ptr->m_ctlListResult.GetItemText(i, LIST_RESULT_IP);
			addr_inr.sin_addr.S_un.S_addr = inet_addr(strTemp);
			ipHeader->desrIP = addr_inr.sin_addr.s_addr;   //设置IP头
			ipHeader->checksum = ptr->Checksum((USHORT*)sendBuf, IPHEADLEN);  //计算校验和

			CString ss, tt = "";
			for (int s = 0; s < 200; s++) {
				ss.Format("%02X", (UCHAR)sendBuf[s]);//以十六进制形式输出，若不足两位,前面补0输出
				tt += ss;
				if (s % 10 == 9)
					tt += "\r\n";
			}
			sendto(sockSend, (char*)sendBuf, ICMPHEADLEN, 0, (struct sockaddr*) & addr_inr,sizeof(addr_inr));
			Sleep(100);
		}
	}
	ptr->GetDlgItem(IDC_BUTTONStartScan)->EnableWindow(TRUE);
}

void CHostScan::OnBnClickedButtonstopscan()
{
	// TODO: 在此添加控件通知处理程序代码
	AfxMessageBox("已结束扫描", MB_OK);
	GetDlgItem(IDC_BUTTONStartScan)->EnableWindow(true);
	g_bReceiving = false;
}

USHORT CHostScan::Checksum(USHORT* buffer, int size)
{
	unsigned long cksum = 0;
	//将数据以字为单位累加到cksum中
	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}
	//如果为奇数，将最后一个字扩展到双字，再累加到cksum中
	if (size) {
		cksum += *(UCHAR*)buffer;
	}
	//将cksum的高16位和低16位相加，取反后得到校验和
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}

CString IPIntToStr(UINT IPInt)
{
	UNIONIP IP;
	CString IPStr;

	IP.uInt = IPInt;
	IPStr.Format("%d.%d.%d.%d", IP.ucByte[0], IP.ucByte[1], IP.ucByte[2], IP.ucByte[3]);
	return IPStr;
}

UINT IPStrToInt(CString IPStr)
{
	UNIONIP IP;

	int i, j = 0;
	IPStr.TrimLeft(" ");
	IPStr.TrimRight(" ");
	for (i = 0; i < IPStr.GetLength(); i++)
	{
		if (IPStr.GetAt(i) < '0' || IPStr.GetAt(i) > '9')
			if (IPStr.GetAt(i) == '.')
				j++;
			else
				return 0;
	}
	if (j != 3)
		return 0;
	i = 0;
	IPStr += ".";
	CString temp;
	for (int m = 0; m < 4; m++)
	{
		temp = "";
		while (IPStr.GetAt(i) != '.')
		{
			temp += IPStr.GetAt(i);
			i++;
		}
		i++;
		if (temp == "" || atoi(temp) > 0xFF)
			return 0;
		else
			IP.ucByte[m] = atoi(temp);
	}
	return IP.uInt;
}





