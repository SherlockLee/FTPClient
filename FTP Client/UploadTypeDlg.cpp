// UploadTypeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FTP Client.h"
#include "UploadTypeDlg.h"


// CUploadTypeDlg 对话框

extern CString csUploadDirectory, csUploadFile;

IMPLEMENT_DYNAMIC(CUploadTypeDlg, CDialog)

CUploadTypeDlg::CUploadTypeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUploadTypeDlg::IDD, pParent)
{

}

CUploadTypeDlg::~CUploadTypeDlg()
{
}

void CUploadTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CUploadTypeDlg, CDialog)
	ON_BN_CLICKED(IDC_FILE_RADIO, &CUploadTypeDlg::OnBnClickedFileRadio)
	ON_BN_CLICKED(IDC_DIRECTORY_RADIO, &CUploadTypeDlg::OnBnClickedDirectoryRadio)
END_MESSAGE_MAP()


// CUploadTypeDlg 消息处理程序

BOOL CUploadTypeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CUploadTypeDlg::OnBnClickedFileRadio()
{
	CFileDialog  OpenFileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
		_T("All Files(*.*)|*.*"));
	if (OpenFileDlg.DoModal() == IDOK)
	{
		csUploadFile = OpenFileDlg.GetPathName();
		CloseUploadTypeWindow();
	}	
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) 
{ 
	switch(uMsg)
	{ 
	case BFFM_INITIALIZED: 
		::SendMessage(hwnd, BFFM_SETSELECTION,TRUE, (LPARAM)L"\\Storage Card"); 
		break; 
	case BFFM_SELCHANGED: 
		{ 
			wchar_t curr[MAX_PATH]; 
			SHGetPathFromIDList((LPCITEMIDLIST)lParam, curr); 
			::SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)curr); 
		} 
		break; 
	} 
	return 0; 
} 

void CUploadTypeDlg::OnBnClickedDirectoryRadio()
{
	UpdateData(TRUE);
	TCHAR szDir[MAX_PATH] = {0};
	BROWSEINFO bi;
	ITEMIDLIST *pidl;
	bi.hwndOwner = this->m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDir;
	bi.lpszTitle = _T("请选择位置：");
	bi.ulFlags = BIF_STATUSTEXT | BIF_BROWSEINCLUDEFILES | BIF_RETURNONLYFSDIRS ;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = 0;
	bi.iImage = 0;
	pidl = (ITEMIDLIST *)SHBrowseForFolder(&bi);
	if(pidl == NULL)  
	{
		return;
	}
	if(!SHGetPathFromIDList(pidl, szDir))
	{
		return;
	}
	else  
	{
		csUploadDirectory.Append(szDir, wcslen(szDir));
		CloseUploadTypeWindow();
	}
	UpdateData(FALSE);    
}

BOOL CUploadTypeDlg::CloseUploadTypeWindow(void)
{
	CWnd * pWnd = NULL;
	pWnd = FindWindow(NULL, _T("上传类型"));
	if (pWnd)
	{
		pWnd->SendMessage(WM_CLOSE);
		return TRUE;
	}

	return FALSE;
}
