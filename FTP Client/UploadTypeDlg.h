#pragma once


// CUploadTypeDlg �Ի���

class CUploadTypeDlg : public CDialog
{
	DECLARE_DYNAMIC(CUploadTypeDlg)

public:
	CUploadTypeDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CUploadTypeDlg();

// �Ի�������
	enum { IDD = IDD_UPLOADTYPE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedFileRadio();
	afx_msg void OnBnClickedDirectoryRadio();
	BOOL CloseUploadTypeWindow(void);
};
