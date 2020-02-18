#include "stdafx.h"
#include "sdk_demo_app.h"

HANDLE   handle;
using namespace DuiLib;
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
			//���д���
			HWND m_hWnd(NULL);
			m_hWnd = app_->m_sdk_login_ui_mgr->GetHWND();
			app_->m_sdk_login_ui_mgr->SwitchToWaitingPage(L"", false);
			app_->m_sdk_login_ui_mgr->ShowWindow(true);

			//���д���
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
			
			
			app_->m_sdk_login_ui_mgr->initCheckUri();
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
		printf("��ʱû���յ�-------");
		MessageBox(NULL, _T("��ʱû���յ�"), _T("ERROR"), SW_NORMAL);
		break;
		// An error occurred
	case WAIT_ABANDONED:
		printf("����һ������������ֹ-------");
		MessageBox(NULL, _T("����һ������������ֹ"), _T("ERROR"), SW_NORMAL);
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
	
   handle = ::CreateEvent(NULL, FALSE, FALSE, _T("Global\\jinchan"));
	DWORD dwError = GetLastError();
	if (ERROR_ALREADY_EXISTS == dwError || ERROR_ACCESS_DENIED == dwError)
	{
		HANDLE oHandle = ::OpenEvent(EVENT_ALL_ACCESS, TRUE, _T("Global\\jinchan"));
		if (oHandle)
		{
			
			SetEvent(oHandle);
			//::CloseHandle(oHandle); 
		}

		::CloseHandle(handle);
		
		// �Ѿ���ʵ���ˣ��˳���
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