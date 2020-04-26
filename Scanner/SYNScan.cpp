// SYNScan.cpp: 实现文件
//

#include "stdafx.h"
#include "pch.h"
#include "Scanner.h"
#include "SYNScan.h"
#include "afxdialogex.h"
#include <Mstcpip.h>

#define RECV_BUF_SIZE	TCP_WINDOW_SIZE
#define TCP_WINDOW_SIZE 64240
#define TIMER_ID_RECV_TIMEOUT	5174

// CSYNScan 对话框

IMPLEMENT_DYNAMIC(CSYNScan, CDialogEx)

CSYNScan::CSYNScan(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOGSYNSCAN, pParent)
{

}

CSYNScan::~CSYNScan()
{
}

void CSYNScan::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS_SRCIP, m_ipaddrSrc);
	DDX_Control(pDX, IDC_IPADDRESS_DSTIP, m_ipaddrDst);
	DDX_Control(pDX, IDC_EDIT_STARTPORT, m_editDstStartPort);
	DDX_Control(pDX, IDC_EDIT_ENDPORT, m_editDstEndPort);
	DDX_Control(pDX, IDC_EDIT_SRCPORT, m_editSrcPort);
	DDX_Control(pDX, IDC_EDIT_RECVTIME, m_editRecvTimeOut);
	DDX_Control(pDX, IDC_LIST_SEND, m_clistctrlSend);
	DDX_Control(pDX, IDC_LIST_RECV, m_clistctrlRecv);
	DDX_Control(pDX, IDC_BUTTONSYNSCAN, m_cbtnScan);
}


BEGIN_MESSAGE_MAP(CSYNScan, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTONSYNSCAN, &CSYNScan::OnBnClickedButtonsynscan)
	ON_BN_CLICKED(IDC_BUTTON_STOPSYN, &CSYNScan::OnBnClickedButtonStopsyn)
END_MESSAGE_MAP()


// CSYNScan 消息处理程序


BOOL CSYNScan::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	//初始化 WIN SOCKET
	int iResult = 0;
	WSADATA wsaData = { 0 };
	iResult = WSAStartup(MAKEWORD(2, 0), &wsaData);
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("WSAStartup failed with error : %d"), WSAGetLastError());
		MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		SendMessage(WM_CLOSE);
		return TRUE;
	}
	
	//获取本地IP
	char strHostName[255] = { 0 };
	iResult = gethostname(strHostName, 255);//获取主机名
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("gethostname failed with error : %d"), WSAGetLastError());
		MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		SendMessage(WM_CLOSE);
		return TRUE;
	}
	
	PHOSTENT pHostInfo = NULL;
	pHostInfo = gethostbyname(strHostName);	//获取包含主机名字和地址信息的hostent结构的指针
	if (pHostInfo == NULL) {
		CString str;
		str.Format(TEXT("gethostbyname failed with error : %d"), WSAGetLastError());
		MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		SendMessage(WM_CLOSE);
		return TRUE;
	}
	m_cstrMyIP = inet_ntoa(*(struct in_addr*)pHostInfo->h_addr_list[0]);//获取IP地址

	//创建发送数据的原始套接字
	iResult = m_sockRawSend = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("socket failed with error : %d"), WSAGetLastError());
		MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		SendMessage(WM_CLOSE);
		return TRUE;
	}

	//设置IP_HDRINCL选项方便自己组建原始IP包
	DWORD  bRaw = TRUE;
	iResult = setsockopt(m_sockRawSend, IPPROTO_IP, IP_HDRINCL, (const char*)&bRaw, sizeof(bRaw));
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("setsockopt IP_HDRINCL failed with error : %d"), WSAGetLastError());
		MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		SendMessage(WM_CLOSE);
		return TRUE;
	}

	//初始化运行界面数据
	m_ipaddrSrc.SetAddress((DWORD)htonl(*(u_long*)pHostInfo->h_addr_list[0]));
	m_editSrcPort.SetLimitText(5);
	m_editDstStartPort.SetLimitText(5);
	m_editDstEndPort.SetLimitText(5);
	m_editRecvTimeOut.SetWindowText(TEXT("2000"));//设置超时时间
	m_editSrcPort.SetWindowText(TEXT("5174"));
	m_editDstStartPort.SetWindowText(TEXT("70"));
	m_editDstEndPort.SetWindowText(TEXT("80"));

	m_clistctrlSend.InsertColumn(0, TEXT("IP ADDRESS"), LVCFMT_LEFT, 150);
	m_clistctrlSend.InsertColumn(1, TEXT("DST PORT"), LVCFMT_LEFT, 125);
	m_clistctrlSend.InsertColumn(2, TEXT("SYN SEND STATUS"), LVCFMT_LEFT, 225);

	m_clistctrlRecv.InsertColumn(0, TEXT("OPEN PROT"), LVCFMT_LEFT, 200);

	//生产随机SEQ
	srand(time(NULL));

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

DWORD WINAPI ThreadProcSend(_In_ LPVOID lpParameter) {
	CSYNScan* pWnd = (CSYNScan*)lpParameter;

	DWORD dwHostIPSrc = 0,
		dwHostIPDst = 0;
	UINT16 usHostSrcPort = 0,
		usHostDstStartPort = 0,
		usHostDstEndPort = 0;
	CString cstrTemp;

	pWnd->m_ipaddrSrc.GetAddress(dwHostIPSrc);
	pWnd->m_ipaddrDst.GetAddress(dwHostIPDst);

	pWnd->m_editSrcPort.GetWindowText(cstrTemp);
	usHostSrcPort = _ttoi(cstrTemp);
	pWnd->m_editDstStartPort.GetWindowText(cstrTemp);
	usHostDstStartPort = _ttoi(cstrTemp);
	pWnd->m_editDstEndPort.GetWindowText(cstrTemp);
	usHostDstEndPort = _ttoi(cstrTemp);

	sockaddr_in addrRecver = { 0 };
	addrRecver.sin_family = AF_INET;
	addrRecver.sin_addr.S_un.S_addr = htonl(dwHostIPDst);

	int iResult = 0;
	bool bSended = 0;
	u_long ulnDstIP = htonl(dwHostIPDst);
	CString cstrDstIP(inet_ntoa(*(in_addr*)&ulnDstIP));//获取目的IP
	CString cstrPortTemp;
	UINT16 usOffset = (UINT16)rand();//获取随机的SEQ
	for (UINT16 iDstPort = usHostDstStartPort; iDstPort <= usHostDstEndPort; iDstPort++) {
		bSended = true;
		//组建IP头和TCP头
		IPHEADER ipHeader = { 0 };
		TCPHEADER tcpHeader = { 0 };

		ipHeader = getIpHeader(dwHostIPSrc, dwHostIPDst, usOffset + iDstPort); 
		tcpHeader = getTCPHeader(&ipHeader, usHostSrcPort, iDstPort);
		SEND_DATA sendData = { ipHeader , tcpHeader };

		addrRecver.sin_port = htons(iDstPort);
		iResult = sendto(pWnd->m_sockRawSend, (const char*)&sendData, sizeof(SEND_DATA), 0, (sockaddr*)&addrRecver, sizeof(sockaddr));
		if (iResult == SOCKET_ERROR) {
			bSended = false;
		}
		//将目的IP显示到发送界面上
		pWnd->m_clistctrlSend.InsertItem(pWnd->m_clistctrlSend.GetItemCount(),cstrDstIP);
		//将扫描端口显示到发送界面上
		cstrPortTemp.Format(TEXT("%u"), iDstPort);
		pWnd->m_clistctrlSend.SetItemText(pWnd->m_clistctrlSend.GetItemCount() - 1,1,cstrPortTemp);
		//将发送状态显示到发送界面上
		pWnd->m_clistctrlSend.SetItemText(pWnd->m_clistctrlSend.GetItemCount() - 1,2,bSended ? TEXT("Success") : TEXT("Failed"));
	}
	//设置定时器函数SetTimer处理接收超时前的发送次数
	CString cstrTimeOut;
	pWnd->m_editRecvTimeOut.GetWindowText(cstrTimeOut);
	pWnd->SetTimer(TIMER_ID_RECV_TIMEOUT, atoi(CStringA(cstrTimeOut)), NULL);
	return 0;
}

DWORD WINAPI ThreadProcRecv(_In_ LPVOID lpParameter) {
	CSYNScan* pWnd = (CSYNScan*)lpParameter;

	DWORD dwSrcIP = 0;
	CStringA cstraMyIP(pWnd->m_cstrMyIP);//初始化中获取的本机IP值
	pWnd->m_ipaddrSrc.GetAddress(dwSrcIP);//界面IP框内IP值
	//判断界面主机IP是否正确
	if (inet_addr(cstraMyIP) != ntohl(dwSrcIP)) {
		CString str;
		str.Format(TEXT("The source IP address is inconsistent with the local IP address and cannot obtain the scan result"));
		pWnd->MessageBox(str, TEXT("WORNING"), MB_ICONWARNING);
		return 0;
	}

	int iResult = 0;
	SOCKET sockRawRecv = INVALID_SOCKET;
	//创建接收数据的原始套接字
	iResult = sockRawRecv = pWnd->m_sockRawRecv = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("ThreadProcRecv socket failed with error : %d"), WSAGetLastError());
		pWnd->MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		return -1;
	}

	//设置IP_HDRINCL选项方便自己分析过滤数据包
	DWORD  bRaw = TRUE;
	iResult = setsockopt(sockRawRecv, IPPROTO_IP, IP_HDRINCL, (const char*)&bRaw, sizeof(bRaw));
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("ThreadProcRecv setsockopt IP_HDRINCL failed with error : %d"), WSAGetLastError());
		pWnd->MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		closesocket(sockRawRecv);
		return -1;
	}

	sockaddr_in sockAddr = { 0 };
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(0);
	sockAddr.sin_addr.S_un.S_addr = inet_addr(cstraMyIP);
	//绑定本地端口
	iResult = bind(sockRawRecv, (const sockaddr*)&sockAddr, sizeof(sockaddr));
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("ThreadProcRecv bind failed with error : %d"), WSAGetLastError());
		pWnd->MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		closesocket(sockRawRecv);
		return -1;
	}

	DWORD uiOptval = RCVALL_ON;
	DWORD dwBytesRet = 0;
	//设置SIO_RCVALL函数以接收所有IP包
	iResult = WSAIoctl(sockRawRecv, SIO_RCVALL, &uiOptval, sizeof(uiOptval), NULL, 0, &dwBytesRet, NULL, NULL);
	if (iResult == SOCKET_ERROR) {
		CString str;
		str.Format(TEXT("ThreadProcRecv WSAIoctl SIO_RCVALL failed with error : %d"), WSAGetLastError());
		pWnd->MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		closesocket(sockRawRecv);
		return -1;
	}

	//接收并过滤数据包
	sockaddr_in sockAddrSrc = { 0 };
	int iLenSockAddr = sizeof(sockaddr_in);
	char* recvbuf = NULL;
	recvbuf = (char*)malloc(sizeof(char) * RECV_BUF_SIZE);

	DWORD uinSrcIP = 0;
	pWnd->m_ipaddrDst.GetAddress(uinSrcIP);
	uinSrcIP = htonl(uinSrcIP);//接收数据包的源IP

	USHORT ushSrcStartPort = 0,//接收数据包的开始端口
		ushSrcEndPort = 0;//接收数据包的结束端口
	CString cstrwTemp;
	CStringA cstraTemp;

	pWnd->m_editDstStartPort.GetWindowText(cstrwTemp);
	cstraTemp = cstrwTemp;
	ushSrcStartPort = LOWORD(atoi(cstraTemp));//取低16位

	pWnd->m_editDstEndPort.GetWindowText(cstrwTemp);
	cstraTemp = cstrwTemp;
	ushSrcEndPort = LOWORD(atoi(cstraTemp));//取低16位

	//初始化扫描状态
	char* cPortsStatus = (char*)malloc(0xffff);
	memset(cPortsStatus, 0, 0xffff);

	//创建发送线程
	pWnd->m_hThreadSend = CreateThread(NULL,0,ThreadProcSend,pWnd,0,NULL);
	if (pWnd->m_hThreadSend == NULL) {
		CString str;
		str.Format(TEXT("create send thread failed with error : %u"), GetLastError());
		pWnd->MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		return -1;
	}
	
	//创建接收线程
	while (true) {
		int iRecvLen = recvfrom(sockRawRecv, recvbuf, sizeof(char) * RECV_BUF_SIZE, 0, (sockaddr*)&sockAddrSrc, &iLenSockAddr);
		if (iRecvLen == 0) {
			break;
		}
		else if (iRecvLen == SOCKET_ERROR) {
			int iErrorCode = WSAGetLastError();
			CString str;
			str.Format(TEXT("ThreadProcRecv recvfrom failed with error : %d"), iErrorCode);
			pWnd->MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
			closesocket(sockRawRecv);
			break;
		}
		else if (iRecvLen > 0) {
			//解析数据包
			PIPHEADER pipHeader = (PIPHEADER)recvbuf;
			PTCPHEADER ptcpHeader = (PTCPHEADER)(recvbuf + (pipHeader->ver_pack & 0x0f) * 4);

			USHORT ushSrcPort = ntohs(ptcpHeader->th_sport);
			if (pipHeader->sourceIP == uinSrcIP &&//判断数据包的源IP是否为目的IP
				ushSrcPort >= ushSrcStartPort &&ushSrcPort <= ushSrcEndPort &&//判断数据包的源端口是否在扫描范围内
				!cPortsStatus[ushSrcPort] &&//判断是否为重复端口
				ptcpHeader->th_ack == 0x01000000 &&//判断数据包的确认号是否为1
				((ntohs(ptcpHeader->usHeaderLenAndFlag) & 0x0012) == 0x0012)//判断数据包的标志位是否为SYN/ACK
				) {
				//保存端口扫描状态
				cPortsStatus[ushSrcPort] = 1;

				//将该端口显示在接收界面上
				CString cstrOpenPort;
				cstrOpenPort.Format(TEXT("%u"), ushSrcPort);
				pWnd->m_clistctrlRecv.InsertItem(pWnd->m_clistctrlRecv.GetItemCount(),cstrOpenPort);
			}
		}
	}
	closesocket(sockRawRecv);
	free(cPortsStatus);
	free(recvbuf);
	cPortsStatus = NULL;
	recvbuf = NULL;

	return 0;
}

void CSYNScan::OnBnClickedButtonsynscan()
{
	// TODO: 在此添加控件通知处理程序代码

	//清空列表数据
	m_clistctrlSend.DeleteAllItems();
	m_clistctrlRecv.DeleteAllItems();

	//判断界面编辑框内是否成功赋值
	if (m_editDstStartPort.GetWindowTextLength() < 1 ||
		m_editDstEndPort.GetWindowTextLength() < 1 ||
		m_editSrcPort.GetWindowTextLength() < 1 ||
		m_editRecvTimeOut.GetWindowTextLength() < 1)
	{
		MessageBox(TEXT("All parament need filled!"), TEXT("WORNING"), MB_ICONWARNING);
		return;
	}
	//判断界面IP框内是否成功赋值
	if (m_ipaddrDst.IsBlank() ||
		m_ipaddrSrc.IsBlank()) {
		MessageBox(TEXT("All parament need filled!"), TEXT("WORNING"), MB_ICONWARNING);
		return;
	}

	//开始扫描

	//创建接收线程（函数内实际还是先开启发送线程，再开启接受线程）
	m_hThreadRecv = CreateThread(NULL,0,ThreadProcRecv,this,0,NULL);
	if (m_hThreadRecv == NULL) {
		CString str;
		str.Format(TEXT("create recv thread failed with error : %u"), GetLastError());
		MessageBox(str, TEXT("ERROR"), MB_ICONERROR);
		return;
	}
	
	GetDlgItem(IDC_BUTTONSYNSCAN)->EnableWindow(false);
	return;
}

void CSYNScan::OnBnClickedButtonStopsyn()
{
	// TODO: 在此添加控件通知处理程序代码
	closesocket(m_sockRawSend);
	 WSACleanup();
	CDialogEx::OnClose();

	AfxMessageBox("已结束扫描", MB_OK);
	GetDlgItem(IDC_BUTTONSYNSCAN)->EnableWindow(true);
}

UINT16 getCheckSum(UINT8* pBuf, UINT32 uLen)
{
	UINT32 uCheckSum = 0, uLoop = 0;

	for (; uLoop < uLen; uLoop++)
	{
		if (0 == uLoop % 2) {
			uCheckSum += pBuf[uLoop] << 8;
		}
		else {
			uCheckSum += pBuf[uLoop];
		}
	}
	uCheckSum = (uCheckSum >> 16) + (uCheckSum & 0x0000FFFF);

	return (UINT16)(~uCheckSum);
}

IPHEADER getIpHeader(UINT32 uihostIPSRC, UINT32 uihostIPDST, UINT16 ushostIdentification) {
	IPHEADER ipHeader = { 0 };
	ipHeader.ver_pack = ((UINT8)IPPROTO_IPV4 << 4) | (UINT8)(sizeof(IPHEADER) / 4);
	ipHeader.tos = (UINT8)0x00;
	ipHeader.total_len = htons((UINT16)0);//this field should be set after when calculate check num.
	ipHeader.ident = htons((UINT16)ushostIdentification);//Unique for each msg.
	ipHeader.frag_and_flags = htons(0x02 << 13);//this field should be set when dgram need fragment
	ipHeader.ttl = (UINT8)64;
	ipHeader.proto = (UINT8)IPPROTO_TCP;
	ipHeader.checksum = htons((UINT16)0);//this field should be set after.
	ipHeader.sourceIP = htonl(uihostIPSRC);
	ipHeader.desrIP = htonl(uihostIPDST);
	ipHeader.total_len = htons(sizeof(IPHEADER) + sizeof(TCPHEADER));

	return ipHeader;
}

TCPHEADER getTCPHeader(const PIPHEADER const pIpHeader, UINT16 uihostSrcPort, UINT16 uihostDstPort) {
	TCPHEADER tcpHeader = { 0 };
	tcpHeader.th_sport = htons(uihostSrcPort);
	tcpHeader.th_dport = htons(uihostDstPort);
	tcpHeader.th_seq = htonl((UINT32)0);
	tcpHeader.th_ack = htonl((UINT32)0);//this field is valid when ACK valid.
	tcpHeader.usHeaderLenAndFlag = htons(((UINT16)(sizeof(TCPHEADER)) / 4 << 12) | 0x2);//0x2 SYN valid.
	tcpHeader.th_win = htons(TCP_WINDOW_SIZE);
	tcpHeader.th_sum = htons((UINT16)0);//this field should be set after.
	tcpHeader.th_urp = htons((UINT16)0);
	//Fill TCP PSD Header
	PSDHEADER tcpPsdHeader = { 0 };
	tcpPsdHeader.saddr = pIpHeader->sourceIP;
	tcpPsdHeader.daddr = pIpHeader->desrIP;
	tcpPsdHeader.mbz = (UINT8)0;
	tcpPsdHeader.ptcl = (UINT8)IPPROTO_TCP;
	tcpPsdHeader.tcpl = htons(sizeof(TCPHEADER));
	//Calculate check sum
	TCP_CHECK_SUM tcpCheckSum = { tcpPsdHeader, tcpHeader };
	tcpHeader.th_sum = htons(getCheckSum((UINT8*)&tcpCheckSum, sizeof(TCP_CHECK_SUM)));
	return tcpHeader;
}