#pragma once


// CUploadTypeDlg 对话框

class CUploadTypeDlg : public CDialog
{
	DECLARE_DYNAMIC(CUploadTypeDlg)

public:
	CUploadTypeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CUploadTypeDlg();

// 对话框数据
	enum { IDD = IDD_UPLOADTYPE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedFileRadio();
	afx_msg void OnBnClickedDirectoryRadio();
	BOOL CloseUploadTypeWindow(void);
};
