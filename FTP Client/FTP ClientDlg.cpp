// FTP ClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FTP Client.h"
#include "FTP ClientDlg.h"
#include <sip.h> 
#include <Wininet.h>
#include "CeFileFind.h"
#include "UploadTypeDlg.h"
#include <atlsimpstr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CFTPClientDlg 对话框
//////////////////////////////////////////////////////////////////////////
// 全局变量定义
//////////////////////////////////////////////////////////////////////////
DWORD dwContext = 0;
HINTERNET hInet = 0, hSession = 0;
CString csOperateLog(_T("")), csUploadDirectory(_T("")), csUploadFile(_T(""));
BOOL bDownFlag = FALSE, bDeleteFlag = FALSE;

//////////////////////////////////////////////////////////////////////////

CFTPClientDlg::CFTPClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFTPClientDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFTPClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFTPClientDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	ON_WM_SIZE()
#endif
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CFTPClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CFTPClientDlg::OnBnClickedButtonClear)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SERVER, &CFTPClientDlg::OnNMDblclkListServer)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CFTPClientDlg::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, &CFTPClientDlg::OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_UPLOAD, &CFTPClientDlg::OnBnClickedButtonUpload)
END_MESSAGE_MAP()


// CFTPClientDlg 消息处理程序

BOOL CFTPClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	SHSipPreference(this->m_hWnd, SIP_UP);
	
	//////////////////////////////////////////////////////////////////////////
	// 初始化默认设置
	//////////////////////////////////////////////////////////////////////////
	GetDlgItem(IDC_EDIT_FTP_ADDRESS)->SetWindowText(_T("192.168.20.234"));
	GetDlgItem(IDC_EDIT_FTP_PORT)->SetWindowText(_T("21"));
	GetDlgItem(IDC_EDIT_FTP_USER)->SetWindowText(_T("anonymous"));
	GetDlgItem(IDC_EDIT_FTP_PASSWORD)->SetWindowText(_T("anonymous"));

	CListCtrl *pServerList = (CListCtrl*)GetDlgItem(IDC_LIST_SERVER);
	pServerList->InsertColumn(0, _T("文件名"), LVCFMT_LEFT, 200);
	pServerList->InsertColumn(1, _T("文件类型"), LVCFMT_LEFT, 85);
	pServerList->InsertColumn(2, _T("文件大小"), LVCFMT_LEFT, 80);


	DWORD dwStyle = pServerList->GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;			// 选中某行使整行高亮
	dwStyle |= LVS_EX_GRIDLINES;				// 网格线
	dwStyle |= LVS_SHOWSELALWAYS;				// 一直选中Item
	pServerList->SetExtendedStyle(dwStyle);		// 设置扩展风格

	//////////////////////////////////////////////////////////////////////////
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CFTPClientDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	if (AfxIsDRAEnabled())
	{
		DRA::RelayoutDialog(
			AfxGetResourceHandle(), 
			this->m_hWnd, 
			DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_FTPCLIENT_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_FTPCLIENT_DIALOG));
	}
}
#endif


void CFTPClientDlg::OnDestroy()
{
	CDialog::OnDestroy();
	SHSipPreference(this->m_hWnd, SIP_FORCEDOWN);	

	DisconnectSession();
	InternetCloseHandle(hInet);
	hInet = 0;
}

void CFTPClientDlg::OnBnClickedButtonConnect()
{
	// TODO: 在此添加控件通知处理程序代码
	CString csButtonText(_T(""));
	GetDlgItem(IDC_BUTTON_CONNECT)->GetWindowText(csButtonText);

	hInet = InternetOpen( _T("B&R FTP Server"), INTERNET_OPEN_TYPE_DIRECT, 0, 0, 0);

	if (csButtonText.Compare(_T("建立连接")) == 0)
	{
		if (hInet != NULL)
		{
			if (StartSession())
			{
				if (GetCurrentRemoteDirectory())
				{								
					GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("断开连接"));
					DisplayLogInfo(_T("成功连接到服务器\r\n"));
				}
				else
				{
					DisplayLogInfo(_T("GetCurrentRemoteDirectory() Error\r\n"));
				}
			}
		}
		else
		{
			DisplayLogInfo(_T("InternetOpen() Error！\r\n"));
		}		
	}
	else
	{
		GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("建立连接"));
		
		DisconnectSession();
		InternetCloseHandle(hInet);
		hInet = 0;
		
		CListCtrl *pServerList = (CListCtrl*)GetDlgItem(IDC_LIST_SERVER);
		pServerList->DeleteAllItems();
		GetDlgItem(IDC_EDIT_REMOTE_PATH)->SetWindowText(_T(""));
		DisplayLogInfo(_T("成功从服务器断开\r\n"));
	}	
}

void CFTPClientDlg::DisconnectSession(void)
{
	if (hSession)
	{
		InternetCloseHandle(hSession);
		hSession = 0;
	}
}

BOOL CFTPClientDlg::StartSession(void)
{
	TCHAR address[32];
	TCHAR *usr = 0;
	TCHAR *pwd = 0;
	GetDlgItem(IDC_EDIT_FTP_ADDRESS)->GetWindowText(address, 32);
	if (wcslen(address) != 0)
	{		
		hSession = InternetConnect(hInet, address, INTERNET_DEFAULT_FTP_PORT, usr, pwd, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, dwContext );
		if (hSession)
		{			
			return TRUE;
		}
		else
		{
			DisplayLogInfo(_T("建立与服务器的连接失败\r\n"));
			return FALSE;
		}
	}
	else
	{
		DisplayLogInfo(_T("服务器地址长度为零\r\n"));
		return FALSE;
	}
	return TRUE;
}

BOOL CFTPClientDlg::GetCurrentRemoteDirectory(void)
{
	BOOL ret = FALSE;
	DWORD dw = 0;

	TCHAR curDir[64];
	FtpGetCurrentDirectory(hSession, 0, &dw);
	if (dw)
	{
		if (FtpGetCurrentDirectory(hSession, curDir, &dw))
		{
			GetDlgItem(IDC_EDIT_REMOTE_PATH)->SetWindowText(curDir);
			//////////////////////////////////////////////////////////////////////////\
			// 从服务器中获取当前目录中的内容，加载至List Ctrl控件中
			//////////////////////////////////////////////////////////////////////////
			WIN32_FIND_DATA ffd;
			HANDLE hFind;
			int nItem = 1, nRow = 0;
			TCHAR* level[]={L"B", L"KB", L"MB", L"GB", L"TB"};
			TCHAR cFileSize[16];
			CListCtrl *pServerList = (CListCtrl*)GetDlgItem(IDC_LIST_SERVER);
			pServerList->DeleteAllItems();
			nRow = pServerList->InsertItem(nItem, _T(".."));
			pServerList->SetItemText(nRow, 1, _T(""));
			pServerList->SetItemText(nRow, 2, _T(""));
			nItem++;nRow++;			

			dwContext = 0;
			hFind = FtpFindFirstFile(hSession, curDir, &ffd, INTERNET_FLAG_RELOAD|INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_CACHE_ASYNC, (DWORD_PTR)&dwContext);
			if ((hFind != INVALID_HANDLE_VALUE) && (hFind != 0))
			{
				BOOL bFoundNext;
				do 
				{
					if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
					{
						if ((wcscmp(ffd.cFileName, L".") != 0))
						{														
							nRow = pServerList->InsertItem(nItem, ffd.cFileName);
							if ((wcscmp(ffd.cFileName, L"..") == 0))
							{																			
								pServerList->SetItemText(nRow, 1, _T(""));
							}
							else
							{
								pServerList->SetItemText(nRow, 1, _T("文件夹"));
							}
							pServerList->SetItemText(nRow, 2, _T(""));
							nItem++;
							nRow++;
						}
					}
					else
					{
						int i = 0;
						//TODO: filesize will probably overflow with big numbers
						DWORDLONG dwHighBase = MAXDWORD;
						dwHighBase += 1;
						double filesize = (double)((ffd.nFileSizeHigh * dwHighBase) + ffd.nFileSizeLow);
						while((filesize > 1024) && (i < 5))
						{
							filesize = filesize / 1024;
							i++;
						}
						wsprintf(cFileSize, _T("%02.2f %s"), filesize, level[i]);
						nRow = pServerList->InsertItem(nItem, ffd.cFileName);
						pServerList->SetItemText(nRow, 1, _T("文件"));
						pServerList->SetItemText(nRow, 2, cFileSize);
						nRow++;
						nItem++;
					}
					bFoundNext = InternetFindNextFile(hFind, &ffd);
				} while (bFoundNext);
			}
			else
			{
				CString csLastError(_T(""));
				csLastError.Format(_T("%d %x"), GetLastError(), GetLastError());
				DisplayLogInfo(csLastError + _T("\r\n"));
				switch (GetLastError())
				{
				case ERROR_NO_MORE_FILES:
					break;
				case ERROR_FTP_TRANSFER_IN_PROGRESS:
					DisplayLogInfo(_T("ERROR_FTP_TRANSFER_IN_PROGRESS\r\n"));
					break;
				}
			}

			//////////////////////////////////////////////////////////////////////////
			return TRUE;
		}
		else
		{
			DisplayLogInfo(_T("FtpGetCurrentDirectory Error\r\n"));
			return FALSE;
		}
	}
	return TRUE;
}

void CFTPClientDlg::DisplayLogInfo(CString csLogInfo)
{
	csOperateLog.Insert(0, csLogInfo);
	GetDlgItem(IDC_EDIT_LOG)->SetWindowText(csOperateLog);
}

void CFTPClientDlg::OnBnClickedButtonClear()
{
	csOperateLog = _T("");
	DisplayLogInfo(_T(""));
}

void CFTPClientDlg::OnNMDblclkListServer(NMHDR *pNMHDR, LRESULT *pResult)
{
	CListCtrl *pServerList = (CListCtrl*)GetDlgItem(IDC_LIST_SERVER);
	CString csFileType(_T("")), csItemName(_T("")), csRemotePath(_T(""));
	int nID;
	POSITION pos = pServerList->GetFirstSelectedItemPosition();
	if (pos)
	{
		nID = (int)pServerList->GetNextSelectedItem(pos);
		csItemName = pServerList->GetItemText(nID, 0);
		csFileType = pServerList->GetItemText(nID, 1);	
		GetDlgItem(IDC_EDIT_REMOTE_PATH)->GetWindowText(csRemotePath);
		if (csFileType.Compare(_T("文件夹")) == 0)
		{			
			if (csRemotePath.Compare(_T("/")) != 0)
			{
				csRemotePath += _T("/");
			}
			csRemotePath += csItemName;			
			GetDlgItem(IDC_EDIT_REMOTE_PATH)->SetWindowText(csRemotePath);			
		}
		else if (csItemName.Compare(_T("..")) == 0)
		{
			int TarPos = csRemotePath.ReverseFind('/');
			if ((TarPos == 0) || (csRemotePath.GetLength() == 1))
			{
				csRemotePath.Delete(TarPos + 1, csRemotePath.GetLength() - TarPos - 1);			
			}			
			else
			{
				csRemotePath.Delete(TarPos, csRemotePath.GetLength() - TarPos);				
			}
		}
		else if (csFileType.Compare(_T("文件")) == 0)
		{
			GetDlgItem(IDC_BUTTON_DOWN)->SetWindowText(_T("下载ing"));
			if (csRemotePath.GetLength() != 1)
			{
				csRemotePath += _T("/");
			}			
			csRemotePath += csItemName;
			csItemName.Insert(0, _T("/"));
			DWORD dwContext = 0;
			if (FtpGetFile(hSession, csRemotePath, csItemName, FALSE, FTP_TRANSFER_TYPE_BINARY, 0, (DWORD_PTR)&dwContext))
			{
				DWORD a = GetFileAttributes(csItemName);
				if (a != INVALID_FILE_ATTRIBUTES)
				{
					SetFileAttributes(csItemName, a^FILE_ATTRIBUTE_HIDDEN);	
				}
				DisplayLogInfo(csRemotePath + _T("文件已下载到根目录\r\n"));
				GetDlgItem(IDC_BUTTON_DOWN)->SetWindowText(_T("下载"));
			}
		}
		else
		{
			
		}

		DisplayCurrentRemoteDirectory(csRemotePath);						
	}
	
	*pResult = 0;
}

void CFTPClientDlg::OnBnClickedButtonDown()
{
	CListCtrl *pServerList = (CListCtrl*)GetDlgItem(IDC_LIST_SERVER);
	CString csFileType(_T("")), csItemName(_T("")), csRemotePath(_T("")), csRemoteFilePath(_T("")), csLocalFilePath(_T("/"));
	int nID;		
	POSITION pos = pServerList->GetFirstSelectedItemPosition();
	if (pos)
	{		
		GetDlgItem(IDC_BUTTON_DOWN)->SetWindowText(_T("下载ing"));
		nID = (int)pServerList->GetNextSelectedItem(pos);
		csItemName = pServerList->GetItemText(nID, 0);
		csFileType = pServerList->GetItemText(nID, 1);	
		if (csFileType.Compare(_T("文件夹")) == 0)
		{
			GetDlgItem(IDC_EDIT_REMOTE_PATH)->GetWindowText(csRemotePath);
			if (csRemotePath.GetLength() != 1)
			{
				csRemotePath += _T("/");
			}
			csRemotePath += csItemName;
			csLocalFilePath += csItemName;
			if (CreateDirectory(csLocalFilePath, NULL))
			{				
				RestartCurrentSession();
				FtpSetCurrentDirectory(hSession, csRemotePath);
				FtpEnumDirectory(hSession, csRemotePath, csLocalFilePath);
				DisplayLogInfo(_T("最终目录已下载!\r\n"));														
				DisplayCurrentRemoteDirectory();
			}			
		}
		if (csFileType.Compare(_T("文件")) == 0)
		{
			GetDlgItem(IDC_EDIT_REMOTE_PATH)->GetWindowText(csRemotePath);
			int TarPos = csRemotePath.GetLength();
			if (TarPos != 1)
			{
				csRemotePath += _T("/");
			}			
			csRemotePath += csItemName;
			csItemName.Insert(0, _T("/"));
			DWORD dwContext = 0;
			if (FtpGetFile(hSession, csRemotePath, csItemName, FALSE, FTP_TRANSFER_TYPE_BINARY, 0, (DWORD_PTR)&dwContext))
			{
				DWORD a = GetFileAttributes(csItemName);
				if (a != INVALID_FILE_ATTRIBUTES)
				{
					SetFileAttributes(csItemName, a^FILE_ATTRIBUTE_HIDDEN);	
				}
				DisplayLogInfo(csRemotePath + _T("文件已下载到根目录\r\n"));
				bDownFlag = TRUE;
			}
		}
		else
		{

		}
		GetDlgItem(IDC_BUTTON_DOWN)->SetWindowText(_T("下载"));
	}
	else
	{
		DisplayLogInfo(_T("没有选中任何文件\r\n"));
	}
}

void CFTPClientDlg::OnBnClickedButtonDelete()
{
	CListCtrl *pServerList = (CListCtrl*)GetDlgItem(IDC_LIST_SERVER);
	CString csFileType(_T("")), csItemName(_T("")), csRemotePath(_T("")), csRemoteFilePath(_T(""));
	int nID;
	GetDlgItem(IDC_EDIT_REMOTE_PATH)->GetWindowText(csRemotePath);	
	POSITION pos = pServerList->GetFirstSelectedItemPosition();
	if (pos)
	{
		GetDlgItem(IDC_BUTTON_DELETE)->SetWindowText(_T("删除ing"));
		nID = (int)pServerList->GetNextSelectedItem(pos);
		csItemName = pServerList->GetItemText(nID, 0);
		csFileType = pServerList->GetItemText(nID, 1);	
		if (csFileType.Compare(_T("文件夹")) == 0)
		{
			if (csRemotePath.GetLength() != 1)
			{
				csRemotePath += _T("/");
			}
			csRemotePath += csItemName;
			RestartCurrentSession();
			FtpSetCurrentDirectory(hSession, csRemotePath);
			DeleteRemoteDirectory(hSession, csRemotePath);
			if (FtpRemoveDirectory(hSession, csRemotePath))
			{
				DisplayLogInfo(_T("最终目录已删除\r\n"));				
			}			
		}
		if (csFileType.Compare(_T("文件")) == 0)
		{
			csRemoteFilePath += csRemotePath;
			if (csRemotePath.GetLength() == 1)
			{
				csRemoteFilePath += csItemName;
			}
			else
			{
				csRemoteFilePath += _T("/");
				csRemoteFilePath += csItemName;
			}

			if (FtpDeleteFile(hSession, csRemoteFilePath))
			{
				DisplayLogInfo(_T("文件已删除\r\n"));
			}
			else
			{
				CString csLastError(_T(""));
				csLastError.Format(_T("%d %x"), GetLastError(), GetLastError());
				DisplayLogInfo(csLastError + _T("\r\n"));
				switch (GetLastError())
				{
				case ERROR_NO_MORE_FILES:
					break;
				case ERROR_FTP_TRANSFER_IN_PROGRESS:
					DisplayLogInfo(_T("ERROR_FTP_TRANSFER_IN_PROGRESS\r\n"));
					break;
				}
				DisplayLogInfo(_T("文件删除失败\r\n"));
			}
		}
		else
		{

		}
		DisplayCurrentRemoteDirectory();
		GetDlgItem(IDC_BUTTON_DELETE)->SetWindowText(_T("删除"));
	}
	else
	{
		AfxMessageBox(_T("没有选中任何文件"));
	}
}

void CFTPClientDlg::OnBnClickedButtonUpload()
{
	CUploadTypeDlg UploadTypeDlg;
	if (!UploadTypeDlg.DoModal())
	{
		MessageBox(_T("打开上传类型窗口失败"), _T("信息提示"), MB_OK|MB_ICONINFORMATION);
	}
	
	CString csRemotePath(_T("")), csTargetRemotePath(_T("")), csUploadFileName(_T("")), csRemoteFilePath(_T(""));
	GetDlgItem(IDC_EDIT_REMOTE_PATH)->GetWindowText(csRemotePath);	
	if (csUploadDirectory.GetLength() != 0)
	{				
		if (csRemotePath.GetLength() != 1)
		{
			csRemotePath += _T("/");
		}	
		csTargetRemotePath = csUploadDirectory;
		csTargetRemotePath.Replace(csUploadDirectory.Left(csUploadDirectory.ReverseFind('\\') + 1), csRemotePath);
		if (!FtpCreateDirectory(hSession, csTargetRemotePath))
		{
			DisplayLogInfo(_T("创建目录失败！\r\n"));
		}								
		if (LoopAllFileInDirectory(csUploadDirectory, csTargetRemotePath))
		{			
			DisplayLogInfo(_T("上传目录成功！\r\n"));	
		}
		csUploadDirectory = _T("");
	}

	if (csUploadFile.GetLength() != 0)
	{
		csUploadFileName = csUploadFile.Right(csUploadFile.GetLength() - csUploadFile.ReverseFind('\\') - 1);
		csRemoteFilePath = csRemotePath;
		if (csRemotePath.GetLength() != 1)
		{
			csRemoteFilePath += _T("/");
		}
		csRemoteFilePath += csUploadFileName;				
		if (FtpPutFile(hSession, csUploadFile, csRemoteFilePath, INTERNET_FLAG_RELOAD|INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_TRANSFER_ASCII, 0))
		{
			DisplayLogInfo(csRemoteFilePath + _T("文件上传成功\r\n"));
		}
		csUploadFile = _T("");
	}
	DisplayCurrentRemoteDirectory();
}

BOOL CFTPClientDlg::DisplayCurrentRemoteDirectory()
{
	CString csRootPath(_T(""));
	GetDlgItem(IDC_EDIT_REMOTE_PATH)->GetWindowText(csRootPath);

	RestartCurrentSession();
	FtpSetCurrentDirectory(hSession, csRootPath);
	if (!GetCurrentRemoteDirectory())
	{
		DisplayLogInfo(_T("获取当前远程目录失败\r\n"));
		return FALSE;		
	}
	else
	{
		return TRUE;
	}

	return TRUE;
}

BOOL CFTPClientDlg::DisplayCurrentRemoteDirectory(CString csRemotePath)
{
	RestartCurrentSession();
	FtpSetCurrentDirectory(hSession, csRemotePath);
	if (!GetCurrentRemoteDirectory())
	{
		DisplayLogInfo(_T("获取当前远程目录失败\r\n"));
		return FALSE;		
	}
	else
	{
		return TRUE;
	}

	return TRUE;
}

BOOL CFTPClientDlg::RestartCurrentSession(void)
{
	if (hSession)
	{
		DisconnectSession();
	}
	if (hInet)
	{
		InternetCloseHandle(hInet);
		hInet = 0;
	}

	if ((!hInet) && (!hSession))
	{
		hInet = InternetOpen( _T("B&R FTP Server"), INTERNET_OPEN_TYPE_DIRECT, 0, 0, 0);
		if (hInet)
		{
			if (StartSession())
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL CFTPClientDlg::LoopAllFileInDirectory(CString csDirectory, CString csTargetDirectory)
{
	BOOL bRet = TRUE;
	// 循环遍历源目录下的文件及目录，将其一一拷贝至目录路径下
	CCeFileFind Finder;
	CString csSubSrcPath(_T("")), csSubDesPath("");

	BOOL Found = Finder.FindFile(csDirectory + _T("\\*.*"));
	while(Found)
	{
		Found = Finder.FindNextFile();
		if (Finder.IsDots() || Finder.IsHidden())
		{
			continue;
		}
		csSubSrcPath = Finder.GetFilePath();
		csSubDesPath = csSubSrcPath;
		csSubDesPath.Replace(csDirectory, csTargetDirectory);
		csSubDesPath.Replace(_T("\\"), _T("/"));

		if (Finder.IsDirectory())
		{
			if (FtpCreateDirectory(hSession, csSubDesPath))
			{
				DisplayLogInfo(csSubDesPath + _T("目录已上传！\r\n"));
				bRet = LoopAllFileInDirectory(csSubSrcPath, csSubDesPath);
			}				
		} 
		else
		{
			if (Finder.GetFileName().Compare(_T("Thumbs.db")) != 0)
			{
				bRet = FtpPutFile(hSession, csSubSrcPath, csSubDesPath, INTERNET_FLAG_RELOAD|INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_TRANSFER_BINARY, 0);
				DisplayLogInfo(csSubDesPath + _T("文件已上传！\r\n"));
			}
		}
	}
	Finder.Close();
	return bRet;
}

void CFTPClientDlg::CreateAllDirectories(CString csDir)
{
	if(csDir.GetLength()!=0)   
	{   
		if(csDir.Right(1)=="/")
		{
			csDir=csDir.Left(csDir.GetLength()-1);     
		}
		// 目录存在则返回   
		if((GetFileAttributes(csDir) != INVALID_FILE_ATTRIBUTES) && (GetFileAttributes(csDir) != FILE_ATTRIBUTE_DIRECTORY))
			return;   
		// 递归创建目录   
		int nFound = csDir.ReverseFind('/');
		CreateAllDirectories(csDir.Left(nFound));
		// 实际创建目录
		CreateDirectory(csDir,NULL);
	} 
}

void CFTPClientDlg::FtpEnumDirectory(HINTERNET hConnect , CString szSourceDir , CString szSaveDir)
{
	CString szFileName;
	CStringArray dirs;
	WIN32_FIND_DATA pData;
	HINTERNET hFind;
	dwContext = 0;
	// Start enumeration and get file handle
	if(szSourceDir.GetAt(szSourceDir.GetLength()-1) != '/') 
	{
		szSourceDir += _T("/"); 
	}
	hFind = FtpFindFirstFile(hConnect, szSourceDir + _T("*.*"), &pData, INTERNET_FLAG_RELOAD|INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_CACHE_ASYNC, (DWORD_PTR)&dwContext);
	if (!hFind)   
	{
		if(GetLastError() == ERROR_NO_MORE_FILES)
		{   
			InternetCloseHandle(hFind);
		}   
		else   
		{
#if 0
			CString csLastError(_T(""));
			csLastError.Format(_T("%d"), GetLastError());
			DisplayLogInfo(_T("错误代码为: ") + csLastError + _T("\r\n"));
			DisplayLogInfo(_T("FindFirst failed!\r\n"));
#endif
			InternetCloseHandle(hFind);
			return ;   
		}  
	}
	else
	{
		szFileName=pData.cFileName; 
		if(pData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)   
		{
			if((szFileName != _T(".")) && (szFileName != _T("..")))   // content is sub dir
			{   
				//递归
				FtpSetCurrentDirectory(hConnect, szSourceDir+szFileName);
				dirs.Add(szFileName);
			} 
		}
		else   //download 
		{
			CreateAllDirectories(szSaveDir);   
			if(!FtpGetFile(hConnect, szSourceDir+szFileName, szSaveDir + _T("/") + szFileName, FALSE, FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD ,NULL))
			{
				DisplayLogInfo(_T("FtpGetFile failed!\r\n"));
				return;
			}
		}
	}

	do 
	{
		if(!InternetFindNextFile(hFind, &pData))
		{
			if(GetLastError() == ERROR_NO_MORE_FILES)     
			{   
				InternetCloseHandle(hFind);
				break;
			}   
			else   
			{   
				InternetCloseHandle(hFind);
				return;   
			}  
		}
		else
		{
			szFileName=(LPCTSTR) &pData.cFileName; 
			// Entry is a directory, mark it as such   
			if(pData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)   
			{
				if((szFileName != _T(".")) && (szFileName != _T("..")))   //content is sub dir
				{   
					//递归
					FtpSetCurrentDirectory(hConnect,szSourceDir+szFileName);
					dirs.Add(szFileName);
				} 
			}
			else   //download 
			{
				CreateAllDirectories(szSaveDir);   
				if(!FtpGetFile(hConnect,szSourceDir+szFileName,szSaveDir+_T("/")+szFileName,FALSE,FILE_ATTRIBUTE_NORMAL,FTP_TRANSFER_TYPE_ASCII | INTERNET_FLAG_TRANSFER_ASCII | INTERNET_FLAG_RELOAD ,NULL))
				{
					DisplayLogInfo(_T("FtpGetFile failed!\r\n"));
					return;
				}
			}
		}
	} while(TRUE);

	for(int i=0;i<dirs.GetSize();i++)
	{
		szFileName=dirs.GetAt(i);
		FtpEnumDirectory(hConnect,szSourceDir+szFileName, szSaveDir+_T("/")+szFileName);
		CreateAllDirectories(szSaveDir+_T("/")+szFileName);
	}
	return;
}

void CFTPClientDlg::DeleteRemoteDirectory(HINTERNET hConnect, CString csRemoteDirectory)
{
	CString szFileName;
	CStringArray dirs;
	WIN32_FIND_DATA pData;
	HINTERNET hFind;
	dwContext = 0;
	// Start enumeration and get file handle
	if(csRemoteDirectory.GetAt(csRemoteDirectory.GetLength()-1) != '/') 
	{
		csRemoteDirectory += _T("/"); 
	}
	hFind = FtpFindFirstFile(hConnect, csRemoteDirectory + _T("*.*"), &pData, INTERNET_FLAG_RELOAD|INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_CACHE_ASYNC, (DWORD_PTR)&dwContext);
	if (!hFind)   
	{
		if(GetLastError() == ERROR_NO_MORE_FILES)
		{   
			InternetCloseHandle(hFind);
		}   
		else   
		{
#if 0
			CString csLastError(_T(""));
			csLastError.Format(_T("%d"), GetLastError());
			DisplayLogInfo(_T("错误代码为: ") + csLastError + _T("\r\n"));
			DisplayLogInfo(_T("FindFirst failed!\r\n"));
#endif
			InternetCloseHandle(hFind);
			return ;   
		}  
	}
	else
	{
		szFileName=pData.cFileName; 
		if(pData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)   
		{
			if((szFileName != _T(".")) && (szFileName != _T("..")))   // content is sub dir
			{   
				//递归
				FtpSetCurrentDirectory(hConnect, csRemoteDirectory+szFileName);
				dirs.Add(szFileName);
			} 
		}
		else   //delete 
		{
			if (FtpDeleteFile(hConnect, csRemoteDirectory+szFileName))
			{
				DisplayLogInfo(csRemoteDirectory+szFileName + _T("文件已删除\r\n"));
			}
		}
	}

	do 
	{
		if(!InternetFindNextFile(hFind, &pData))
		{
			if(GetLastError() == ERROR_NO_MORE_FILES)     
			{   
				InternetCloseHandle(hFind);
				break;
			}    
			else   
			{   
#if 0
				CString csLastError(_T(""));
				csLastError.Format(_T("%d"), GetLastError());
				DisplayLogInfo(_T("错误代码为: ") + csLastError + _T("\r\n"));
				DisplayLogInfo(_T("FindNext failed!\r\n"));
#endif
				InternetCloseHandle(hFind);
				return;   
			}  
		}
		else
		{
			szFileName=(LPCTSTR) &pData.cFileName; 
			// Entry is a directory, mark it as such   
			if(pData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)   
			{
				if((szFileName != _T(".")) && (szFileName != _T("..")))   //content is sub dir
				{   
					//递归
					FtpSetCurrentDirectory(hConnect,csRemoteDirectory+szFileName);
					dirs.Add(szFileName);
				} 
			}
			else   //delete 
			{
				if (FtpDeleteFile(hConnect, csRemoteDirectory+szFileName))
				{
					DisplayLogInfo(csRemoteDirectory+szFileName + _T("文件已删除\r\n"));
				}
			}
		}
	} while(TRUE);

	for(int i=0;i<dirs.GetSize();i++)
	{
		szFileName=dirs.GetAt(i);
		DeleteRemoteDirectory(hConnect, csRemoteDirectory+szFileName);
		if (FtpRemoveDirectory(hSession, csRemoteDirectory+szFileName))
		{
			DisplayLogInfo(csRemoteDirectory+szFileName + _T("目录删除成功！\r\n"));
		}
	}
	return;
}
