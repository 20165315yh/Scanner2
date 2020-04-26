
// ScannerDlg.h: 头文件
//

#pragma once
#include "TabSheet.h"
#include "HostScan.h"
#include "PortScan.h"
#include "SYNScan.h"
#include "SQLScan.h"

// CScannerDlg 对话框
class CScannerDlg : public CDialogEx
{
// 构造
public:
	CScannerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SCANNER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CTabSheet m_TabMain;

	CHostScan hScan;
	CPortScan pScan;
	CSYNScan yScan;
	CSQLScan sScan;

};
