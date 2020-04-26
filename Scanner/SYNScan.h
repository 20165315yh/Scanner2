#pragma once


// CSYNScan 对话框

class CSYNScan : public CDialogEx
{
	DECLARE_DYNAMIC(CSYNScan)

public:
	CSYNScan(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSYNScan();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOGSYNSCAN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CIPAddressCtrl m_ipaddrSrc;
	CIPAddressCtrl m_ipaddrDst;
	CEdit m_editDstStartPort;
	CEdit m_editDstEndPort;
	CEdit m_editSrcPort;
	CEdit m_editRecvTimeOut;
	CListCtrl m_clistctrlSend;
	CListCtrl m_clistctrlRecv;
	afx_msg void OnBnClickedButtonsynscan();
	afx_msg void OnBnClickedButtonStopsyn();
	CString m_cstrMyIP;
	SOCKET m_sockRawSend;
	SOCKET m_sockRawRecv;
	HANDLE m_hThreadSend;
	HANDLE m_hThreadRecv;
	CButton m_cbtnScan;
};
