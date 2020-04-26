#pragma once


// CPortScan 对话框

class CPortScan : public CDialogEx
{
	DECLARE_DYNAMIC(CPortScan)

public:
	CPortScan(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CPortScan();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOGPORTSCAN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonstart();
	CIPAddressCtrl m_ctlBeginIP;
	CIPAddressCtrl m_ctlEndIP;
	CTreeCtrl m_ctlTreeResult;
	UINT m_uBeginPort;
	UINT m_uEndPort;
	CString GetPortName(UINT uPort);
	CString m_strLocalIP;
	afx_msg void OnBnClickedButtonstop();
	virtual BOOL OnInitDialog();
	//afx_msg void OnBnClickedButtonstartsyn();
	//USHORT CheckSum(USHORT* buffer, int size);
	UINT g_uLocalPort;
	//void SYNScan(UNIONIP uIPBegin, UNIONIP uIPEnd, UINT uPortBegin, UINT uPortEnd);
	SOCKET m_sockSend;
	SOCKET m_sockRecv;
	HANDLE m_hThreadSend;
	HANDLE m_hThreadRecv;
	CIPAddressCtrl m_ctlSrcIP;
};
