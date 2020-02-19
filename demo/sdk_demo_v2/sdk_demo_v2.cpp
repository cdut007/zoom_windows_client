#include "stdafx.h"
#include "sdk_demo_app.h"

HANDLE   handle;
using namespace DuiLib;

void writeUri() {

	//check uri
	LPWSTR *szArgList;
	int argCount;

	wstring content;

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (szArgList == NULL)
	{
		wchar_t error_msg[100] = { 0 };
		LPCWSTR w = GetCommandLine();
		wsprintf(error_msg, w);
		OutputDebugString(error_msg);
		OutputDebugString(_T("\n"));


	}
	else {
		for (int i = 0; i < argCount; i++)
		{
			LPCWSTR w = szArgList[i];
			std::wstring strParam = w;
			if (strParam.find(_T("jinchan:jinchan")) != wstring::npos) {
				//ShowErrorMessage(strParam.c_str());
				 content = strParam;
                 break;
			}
		}

		LocalFree(szArgList);
	}

	if (content.size() <= 0) {
		
		return;
	}


	HANDLE hFile = CreateFile(TEXT("c:\jcMeeting.dat"), GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL)
	{
		printf("create file error!");
		return ;
	}

	//  HANDLE hFile = (HANDLE)0xffffffff; //创建一个进程间共享的对象
	HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 1024 * 1024, TEXT("JINCHAN"));
	int rst = GetLastError();
	if (hMap != NULL && rst == ERROR_ALREADY_EXISTS)
	{
		printf("hMap error\n");
		CloseHandle(hMap);
		hMap = NULL;
		return ;
	}

	LPTSTR lpMapAddr = NULL;
	lpMapAddr = (LPTSTR)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 1024 * 1024);
	if (lpMapAddr == NULL)
	{
		printf("view map error!");
		return ;
	}
	std::wstring pszText = content;//其实是向文件中（共享内存中）写入了
    wcscpy(lpMapAddr, pszText.c_str());
	FlushViewOfFile(lpMapAddr, pszText.length() + 1);
	//UnmapViewOfFile(lpMapAddr);
	//CloseHandle(hMap); 
	CloseHandle(hFile);
}

wstring readUri() {
	HANDLE hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, TEXT("JINCHAN"));
	if (hMap == NULL)
	{
		printf("open file map error!");
		return  wstring();
	}

	LPTSTR pszText = NULL;
	pszText = (LPTSTR)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 1024 * 1024);
	if (pszText == NULL)
	{
		printf("map view error!\n");
		return  wstring();
	}


	OutputDebugString(pszText); //从文件中读(共享内存)
	wstring content = pszText;
	UnmapViewOfFile(pszText);
	CloseHandle(hMap);

	hMap = NULL;
	
	
	return  content;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	

	DWORD dwWaitResult;
	printf("Thread %d waiting for write event...\n", GetCurrentThreadId());
	CSDKDemoApp  *app_ = (CSDKDemoApp*)lpParam;
	
	 dwWaitResult = WaitForSingleObject( 
        handle, // event handle
        INFINITE);    // indefinite wait-
	switch (dwWaitResult)
	{
		// Event object was signaled
	case WAIT_OBJECT_0:
		//
		// TODO: Read from the shared buffer
		//-
		printf("Thread %d reading from buffer\n",
			GetCurrentThreadId());


		

		if (app_->m_sdk_login_ui_mgr) {
			//app_->m_sdk_login_ui_mgr->GetAppEvent()->onShowLoggedInUI();
			//居中窗口
			HWND m_hWnd(NULL);
			m_hWnd = app_->m_sdk_login_ui_mgr->GetHWND();
			app_->m_sdk_login_ui_mgr->SwitchToWaitingPage(L"", false);
			app_->m_sdk_login_ui_mgr->ShowWindow(true);

			//居中窗口
			RECT rc = { 0 };
			if (::GetClientRect(m_hWnd, &rc)){
			rc.right = rc.left + 524;
			rc.bottom = rc.top + 376;
			if (!::AdjustWindowRectEx(&rc, GetWindowStyle(m_hWnd), (!(GetWindowStyle(m_hWnd) & WS_CHILD) && (::GetMenu(m_hWnd) != NULL)), GetWindowExStyle(m_hWnd))) {

			}
			else {
				int ScreenX = GetSystemMetrics(SM_CXSCREEN);
				int ScreenY = GetSystemMetrics(SM_CYSCREEN);

				::SetWindowPos(m_hWnd, NULL, (ScreenX - (rc.right - rc.left)) / 2,
					(ScreenY - (rc.bottom - rc.top)) / 2, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);
				ActiveWindowToTop(m_hWnd);
			}

		
		}
			
			wstring getUriStr = readUri();
			if (getUriStr.size() > 0) {
				app_->m_sdk_login_ui_mgr->initCheckUriFromOther(getUriStr);
			}

			//AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), TRUE);
			//SetForegroundWindow(m_hWnd);
			//SetFocus(m_hWnd);
			//AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), FALSE);
		}
		//ThreadProc(lpParam);
		DWORD dwThreadID;

		CreateThread(
			NULL,              // default security
			0,                 // default stack size
			ThreadProc,        // name of the thread function
			app_,              // no thread parameters
			0,                 // default startup flags
			&dwThreadID);

		break;
	case WAIT_TIMEOUT:
		printf("超时没有收到-------");
		MessageBox(NULL, _T("超时没有收到"), _T("ERROR"), SW_NORMAL);
		break;
		// An error occurred
	case WAIT_ABANDONED:
		printf("另外一个进程意外终止-------");
		MessageBox(NULL, _T("另外一个进程意外终止"), _T("ERROR"), SW_NORMAL);
		break;
	default:
		printf("Wait error (%d)\n", GetLastError());
		break;

		return 0;
	}

	

	// Now that we are done reading the buffer, we could use another
	// event to signal that this thread is no longer reading. This
	// example simply uses the thread handle for synchronization (the
	// handle is signaled when the thread terminates.)

	printf("Thread %d exiting\n", GetCurrentThreadId());
	return 1;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow)
{
	
   handle = ::CreateEvent(NULL, FALSE, FALSE, _T("jinchan"));
	DWORD dwError = GetLastError();
	if (ERROR_ALREADY_EXISTS == dwError || ERROR_ACCESS_DENIED == dwError)
	{
		HANDLE oHandle = ::OpenEvent(EVENT_ALL_ACCESS, TRUE, _T("jinchan"));
		if (oHandle)
		{
			writeUri();
			SetEvent(oHandle);
			//::CloseHandle(oHandle); 
		}

		::CloseHandle(handle);
		
		// 已经有实例了，退出。
		return FALSE;
	}

	if (handle)
	{
		//::CloseHandle(handle);
	}

	CSDKDemoApp app_;

	DWORD dwThreadID;

	HANDLE ghThread = CreateThread(
		NULL,              // default security
		0,                 // default stack size
		ThreadProc,        // name of the thread function
		&app_,              // no thread parameters
		0,                 // default startup flags
		&dwThreadID);

	

	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
	
	app_.Run(hInstance);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (WM_QUIT == msg.message)
		{
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}