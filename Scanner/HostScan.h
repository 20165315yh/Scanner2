#pragma once


// CHostScan 对话框

class CHostScan : public CDialogEx
{
	DECLARE_DYNAMIC(CHostScan)

public:
	CHostScan(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHostScan();
	USHORT Checksum(USHORT* buffer, int size);

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOGHOSTSCAN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	
	
public:
	afx_msg void OnBnClickedButtonstartscan();
	CIPAddressCtrl m_strIPBegin;
	CIPAddressCtrl m_strIPEnd;
	CListCtrl m_ctlListResult;
	CString m_strLocalIP;
	virtual BOOL OnInitDialog();
	
private:
	
public:
	afx_msg void OnBnClickedButtonstopscan();
};
