#pragma once


// CSQLScan 对话框

class CSQLScan : public CDialogEx
{
	DECLARE_DYNAMIC(CSQLScan)

public:
	CSQLScan(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSQLScan();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOGSQLSCAN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonsqlscanstart();

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonsqlscanstop();
	void ShowSendAndReceive(CString strSend, CString strReceive, CString strResult);
	CString m_strURL;
	CString m_strURLParam;
	CString m_strSQLSign;
	CString m_strSendPack;
	CString m_strRecievePack;
	CString m_strResult;
};
