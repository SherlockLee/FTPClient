// FTP ClientDlg.h : ͷ�ļ�
//

#pragma once

#include <WinInet.h>

// CFTPClientDlg �Ի���
class CFTPClientDlg : public CDialog
{
// ����
public:
	CFTPClientDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_FTPCLIENT_DIALOG };


	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonConnect();
	void DisconnectSession(void);
	BOOL StartSession(void);
	BOOL GetCurrentRemoteDirectory(void);
	void DisplayLogInfo(CString csLogInfo);
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnNMDblclkListServer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonUpload();
	BOOL DisplayCurrentRemoteDirectory();
	BOOL DisplayCurrentRemoteDirectory(CString csRemotePath);
	BOOL RestartCurrentSession(void);
	BOOL LoopAllFileInDirectory(CString csDirectory, CString csTargetDirectory);
	void CreateAllDirectories(CString csDir);
	void FtpEnumDirectory(HINTERNET hConnect , CString szSourceDir , CString szSaveDir);
	void DeleteRemoteDirectory(HINTERNET hConnect, CString csRemoteDirectory);
};
