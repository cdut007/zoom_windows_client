#include "stdafx.h"
#include <wininet.h>
#include <AccCtrl.h>
#include <AclAPI.h>
#include "DownloadProgressWindow.h"

CDownloadProgressUIMgr::CDownloadProgressUIMgr()
{
	
}

CDownloadProgressUIMgr::~CDownloadProgressUIMgr()
{
	
}



typedef void(*DownLoadCallback)(int ContentSize, int CUR_LEN, CDownloadProgressUIMgr* download_progress);


typedef struct _URL_INFO
{
	WCHAR szScheme[512];
	WCHAR szHostName[512];
	WCHAR szUserName[512];
	WCHAR szPassword[512];
	WCHAR szUrlPath[512];
	WCHAR szExtraInfo[512];
}URL_INFO, *PURL_INFO;




//与网页的交互,从网页下载 （参数1为文件的URL，参数2为下载的文件的绝对路径，包括文件名及文件类型）
BOOL downloadFile(TCHAR *lpDwownURL, TCHAR *SavePath, DownLoadCallback Func, CDownloadProgressUIMgr* download_mgr)
{

	HINTERNET hInternetOpen = NULL;
	HINTERNET hHttpConnect = NULL;
	HINTERNET hHttpRequest = NULL;
	BYTE *pMessageBody = NULL;    // 保存文件的指针    
	try
	{
		// 先判断是否联网
		if (!::InternetCheckConnection(lpDwownURL, FLAG_ICC_FORCE_CONNECTION, 0))
		{
			throw 0;
		}
		// 解析URL以及他的组成部分
		TCHAR szHostName[128] = { 0 };
		TCHAR szUrlPath[512] = { 0 };

		URL_COMPONENTS stUrlAnalysis;
		ZeroMemory(&stUrlAnalysis, sizeof(URL_COMPONENTS));
		stUrlAnalysis.dwStructSize = sizeof(URL_COMPONENTS);
		stUrlAnalysis.dwHostNameLength = sizeof(char) * 128;
		stUrlAnalysis.dwUrlPathLength = sizeof(char) * 512;
		stUrlAnalysis.lpszHostName = szHostName;
		stUrlAnalysis.lpszUrlPath = szUrlPath;

		// 解析域名
		if (!::InternetCrackUrl(lpDwownURL, 0, ICU_ESCAPE, &stUrlAnalysis))
		{
			throw 0;
		}

		//初始化WinInet,获取跟句柄
		hInternetOpen = ::InternetOpenA("Microsoft InternetExplorer", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);  //INTERNET_OPEN_TYPE_PRECONFIG
		if (NULL == hInternetOpen)
		{
			throw 0;
		}

		//打开一个HTTP的文件协议
		hHttpConnect = ::InternetConnect(hInternetOpen, szHostName, 9966, NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_FLAG_PASSIVE, 0);
		if (NULL == hHttpConnect)
		{
			throw 0;
		}

		//打开HTTP请求句柄

		hHttpRequest =  ::HttpOpenRequest(hHttpConnect, L"GET", szUrlPath, HTTP_VERSION, NULL, NULL, INTERNET_FLAG_NO_UI | INTERNET_FLAG_DONT_CACHE, 1);
		if (NULL == hHttpRequest)
		{
			throw 0;
		}


		if (!::HttpSendRequestW(hHttpRequest, NULL, 0, NULL, NULL))                     //向服务器发送这个请求
		{
			throw 0;
		}
		OutputDebugString(L"download file url\n");
		OutputDebugString(szUrlPath);
		//  检查HTTP响应消息的头
		DWORD dwInfoBufferLength = 0;
		BYTE *pInfoBuffer = NULL;
		TCHAR szlog[MAX_PATH] = { 0 };
		wsprintf(szlog, _T("准备下载文件\n"));
		OutputDebugString(szlog);
		while (!::HttpQueryInfo(hHttpRequest, HTTP_QUERY_CONTENT_LENGTH , pInfoBuffer, &dwInfoBufferLength, 0))        //接收服务器返回的信息  (此处我们接收文件的大小)    
		{
			
			DWORD dwError = GetLastError();
			if (dwError == ERROR_INSUFFICIENT_BUFFER)   // 传递给参数的缓冲区太小
			{
				pInfoBuffer = new BYTE[dwInfoBufferLength + 1];
			}
			else
			{
				throw 0;
			}
		}
		TCHAR szlog6[MAX_PATH] = { 0 };
		wsprintf(szlog6, _T("download dwInfoBufferLength size : %d\n"), dwInfoBufferLength);
		OutputDebugString(szlog6);

		memcpy(&pInfoBuffer[dwInfoBufferLength], "\0", 1);

		//得到的文件大小转换为int型 
		unsigned long dwFileSile =  _wtoi((WCHAR*)pInfoBuffer);
		delete[] pInfoBuffer;
		DWORD dwBytesRead = 0;
		pMessageBody = new BYTE[dwFileSile + 1];
		// 保存文件
		HANDLE hFile;
		hFile = ::CreateFile(SavePath, GENERIC_WRITE | GENERIC_READ, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);   // 打开一个文件，获取文件句柄  设置属性为可读可写
		if (NULL == hFile)
		{
			throw 0;
		}
		BYTE* lpReadeBuffer = pMessageBody;
		int currentCount = 0;
		if (NULL != pMessageBody)
		{
			do {

				InternetReadFile(hHttpRequest, lpReadeBuffer, 1024 * 8, &dwBytesRead);

				lpReadeBuffer += dwBytesRead;
				currentCount += dwBytesRead;
				Func(currentCount, dwFileSile, download_mgr);

				//Sleep(100);
			} while (dwBytesRead > 0);

			//			if (!InternetReadFile(hHttpRequest, pMessageBody, dwFileSile, &dwBytesRead))
			//			{
			//				throw 0;
			//			}



			unsigned long Bytes;
			if (NULL == WriteFile(hFile, pMessageBody, dwFileSile, &Bytes, NULL))   //向文件写入数据
			{
				CloseHandle(hFile);
				throw 0;
			}


		}
		else
		{
			throw 0;
		}

		

		TCHAR szlog2[MAX_PATH] = { 0 };
		wsprintf(szlog2, _T("下载成功文件\n"));
		OutputDebugString(szlog2);

		TCHAR szlog3[MAX_PATH] = { 0 };
		wsprintf(szlog3, _T("保存文件成功\n"));
		OutputDebugString(szlog3);

		// 进度回调
		//Func(dwContentSize, ReadedLen);
		CloseHandle(hFile);
		delete[] pMessageBody;

	TCHAR * v1 = SavePath;// (wchar_t *)fileDir.c_str();
	SHELLEXECUTEINFO shExecInfo = { 0 };
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExecInfo.hwnd = NULL;
	shExecInfo.lpVerb = _T("open");
	shExecInfo.lpFile = SavePath;
	//shExecInfo.lpDirectory = szFilePath;
	shExecInfo.nShow = SW_SHOW;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
	WaitForSingleObject(shExecInfo.hProcess, INFINITE);
	}
	catch (...)
	{
		if (NULL != hInternetOpen)
		{
			::InternetCloseHandle(hInternetOpen);
		}
		if (NULL != hHttpConnect)
		{
			::InternetCloseHandle(hHttpConnect);
		}
		if (NULL != hHttpRequest)
		{
			::InternetCloseHandle(hHttpRequest);
		}
		if (NULL != pMessageBody)
		{
			delete[] pMessageBody;
		}
		return FALSE;
	}
	return TRUE;
}



LRESULT CDownloadProgressUIMgr::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;


	if (uMsg == WM_CREATE)
	{
		m_PaintManager.Init(m_hWnd);
		CDialogBuilder builder;
		STRINGorID xml(GetSkinRes());
		CControlUI* pRoot = builder.Create(xml, _T("xml"), 0, &m_PaintManager);
		ASSERT(pRoot && "Failed to parse XML");

		m_PaintManager.AttachDialog(pRoot);
		m_PaintManager.AddNotifier(this);
		InitWindow();
		return lRes;
	}
	else if (uMsg == WM_CLOSE)
	{
		ShowWindow(false);
		SetForegroundWindow(GetParent(m_hWnd));
		return lRes;
	}
	else if (uMsg == WM_TIMER)
	{
		
		KillTimer(m_hWnd, 2);
	}

	if (m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes))
	{
		return lRes;
	}

	return __super::HandleMessage(uMsg, wParam, lParam);
}

void CDownloadProgressUIMgr::InitWindow()
{
	RECT rc = { 0 };
	if (!::GetClientRect(m_hWnd, &rc)) return;
	rc.right = rc.left + 524;
	rc.bottom = rc.top + 376;
	if (!::AdjustWindowRectEx(&rc, GetWindowStyle(m_hWnd), (!(GetWindowStyle(m_hWnd) & WS_CHILD) && (::GetMenu(m_hWnd) != NULL)), GetWindowExStyle(m_hWnd))) return;
	int ScreenX = GetSystemMetrics(SM_CXSCREEN);
	int ScreenY = GetSystemMetrics(SM_CYSCREEN);

	::SetWindowPos(m_hWnd, NULL, (ScreenX - (rc.right - rc.left)) / 2,
		(ScreenY - (rc.bottom - rc.top)) / 2, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_HIDEWINDOW);
	download_progress = dynamic_cast<CProgressUI* >(m_PaintManager.FindControl(L"download_progress"));
	
	HANDLE hThread = CreateThread(NULL, 0, checkVersion, this, 0, NULL);
	CloseHandle(hThread);
	//m_copy_invite_info_btn = dynamic_cast<CButtonUI* >(m_PaintManager.FindControl(L"copy_invitate_info_btn"));
	//m_notify_lb = dynamic_cast<CLabelUI* >(m_PaintManager.FindControl(L"copy_notify"));
}

void  dcallback(int ContentSize, int file_size , CDownloadProgressUIMgr* downloadUIMgr)
{
	TCHAR szlog[MAX_PATH] = { 0 };
	wsprintf(szlog, _T("download count:%d, size : %d\n"), ContentSize, file_size);
	OutputDebugString(szlog);
	int progress =  (int)(((float)ContentSize / file_size) * 100);
	if (downloadUIMgr && downloadUIMgr->download_progress) {
		downloadUIMgr->download_progress->SetValue(progress);
	}
	
}

void CDownloadProgressUIMgr::Notify(TNotifyUI& msg)
{
	if (msg.sType == _T("click"))
	{
		if (msg.pSender == m_pause_btn)
		{
			DoClickPauseBtn();
		}
		else if (msg.pSender == m_resume_btn)
		{
			DoClickResumeBtn();
		}
	}
}

DWORD WINAPI checkVersion(LPVOID lpParamter) {
	CDownloadProgressUIMgr *p = (CDownloadProgressUIMgr*)lpParamter;
	p->download(p);
	return 0;
}
void CDownloadProgressUIMgr::setDownloadUrl(wstring url) {
	this->mUrl = url;
}

int EnableFileAccountPrivilege(PCTSTR pszPath, PCTSTR pszAccount)
{
	BOOL bSuccess = TRUE;
	EXPLICIT_ACCESS ea;
	PACL pNewDacl = NULL;
	PACL pOldDacl = NULL;
	do
	{
		// 获取文件(夹)安全对象的DACL列表

		if (ERROR_SUCCESS != GetNamedSecurityInfo((LPTSTR)pszPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDacl, NULL, NULL))
		{
			bSuccess = FALSE;
			break;

		}

		// 此处不可直接用AddAccessAllowedAce函数,因为已有的DACL长度是固定,必须重新创建一个DACL对象

		// 生成指定用户帐户的访问控制信息(这里指定赋予全部的访问权限)

		::BuildExplicitAccessWithName(&ea, (LPTSTR)pszAccount, GENERIC_ALL, GRANT_ACCESS, SUB_CONTAINERS_AND_OBJECTS_INHERIT);

		// 创建新的ACL对象(合并已有的ACL对象和刚生成的用户帐户访问控制信息)

		if (ERROR_SUCCESS != ::SetEntriesInAcl(1, &ea, pOldDacl, &pNewDacl))
		{
			bSuccess = FALSE;
			break;
		}

		// 设置文件(夹)安全对象的DACL列表
		if (ERROR_SUCCESS != ::SetNamedSecurityInfo((LPTSTR)pszPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL))
		{
			bSuccess = FALSE;
		}
	} while (FALSE);

	if (NULL != pNewDacl)
	{
		::LocalFree(pNewDacl);
	}

	return bSuccess;
}


void CDownloadProgressUIMgr::download(CDownloadProgressUIMgr *p) {

	//check download file upgrade is ready.
	//HANDLE hThread2 = CreateThread(NULL, 0, checkVersion, this, 0, NULL);
	//CloseHandle(hThread2);
	
	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	TCHAR szFileName[MAX_PATH + 1] = { 0 };
	int Strlen = GetTempPath(MAX_PATH, szFilePath);

	if (Strlen < 1)
	{
		 //"获取临时路径失败.");
		GetModuleFileName(NULL, szFilePath, MAX_PATH);
	}
	else
	{
		//设置临时路径为工作目录
		SetCurrentDirectory(szFilePath);
	}
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
	wsprintf(szFileName, _T("%s%s"), szFilePath, _T("setup.msi"));

	wstring fileDir = szFileName;
	

	TCHAR * v1 = (wchar_t *)fileDir.c_str();

	downloadFile((wchar_t*)mUrl.c_str(), v1, &dcallback,p);
}

void CDownloadProgressUIMgr::DoClickPauseBtn()
{
	
}

void CDownloadProgressUIMgr::DoClickResumeBtn()
{
	
}
