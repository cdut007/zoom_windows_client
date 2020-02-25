#include "stdafx.h"
#include "sdk_init_auth_ui.h"
#include "mess_info.h"
#include <Windows.h>
#include <regex>
CInitSDKUIGroup::CInitSDKUIGroup()
{
	m_InitSDKPage = NULL;
	m_btnInit = NULL;
	m_editWebDomain = NULL;
	m_mainFrame = NULL;
	m_chkCustomizedUI = NULL;
}
CInitSDKUIGroup::~CInitSDKUIGroup()
{
	UninitWindow();
}
void CInitSDKUIGroup::InitWindow(CPaintManagerUI& ui_mgr, CSDKInitAuthUIMgr* main_frame_)
{
	m_InitSDKPage = static_cast<CVerticalLayoutUI* >(ui_mgr.FindControl(_T("page_init")));
	m_btnInit = static_cast<CButtonUI* >(ui_mgr.FindControl(_T("btn_init_domain")));
	m_editWebDomain = static_cast<CRichEditUI* >(ui_mgr.FindControl(_T("edit_web_domain")));
	m_chkCustomizedUI = static_cast<CCheckBoxUI* >(ui_mgr.FindControl(_T("chk_customizedui")));
	m_mainFrame = main_frame_;
	m_initSDKWorkFlow.SetUIEvent(this);
}

void CInitSDKUIGroup::UninitWindow()
{
	m_initSDKWorkFlow.SetUIEvent(NULL);
	m_initSDKWorkFlow.CleanupSDK();
}

void CInitSDKUIGroup::Show()
{
	if (m_InitSDKPage)
	{
		m_InitSDKPage->SetVisible(true);
		if (m_mainFrame)
		{
			m_mainFrame->SetCurrentPage(m_InitSDKPage);
			DoInitBtnClick();
		}
	}
}
void CInitSDKUIGroup::Hide()
{
	if (m_InitSDKPage)
	{
		m_InitSDKPage->SetVisible(false);
	}
}
void CInitSDKUIGroup::Notify( TNotifyUI& msg )
{
	if(msg.sType == _T("click"))
	{
		if(msg.pSender == m_btnInit)
		{
			DoInitBtnClick();
		}
	}
}

void CInitSDKUIGroup::DoInitBtnClick()
{
	if (NULL == m_editWebDomain)
		return;
	TCHAR strTitle[64] = { 0 };
	wsprintf(strTitle, _T("zoom_sdk_demo_ui"));
	OutputDebugString(strTitle);
	OutputDebugString(_T("\n"));
	std::wstring strWebDomain = m_editWebDomain->GetText().GetData();
	if (strWebDomain.size() > 0)
	{
		ZOOM_SDK_NAMESPACE::InitParam initParam;
		initParam.strWebDomain = strWebDomain.c_str();
		initParam.strSupportUrl = L"https://zoom.us";

		//set language id
		initParam.emLanguageID = ZOOM_SDK_NAMESPACE::LANGUAGE_Chinese_Simplified;
		//change icon
		initParam.uiWindowIconSmallID = IDI_ICON_LOGO;
		initParam.uiWindowIconBigID = IDI_ICON_LOGO;
		initParam.hResInstance = GetModuleHandle(NULL);
		initParam.enableLogByDefault = true;
		bool customiezed_ui_mode = false;
		if (m_chkCustomizedUI)
			customiezed_ui_mode = m_chkCustomizedUI->GetCheck();
		if (customiezed_ui_mode)
			initParam.obConfigOpts.optionalFeatures = ENABLE_CUSTOMIZED_UI_FLAG;
		//initParam.obConfigOpts.optionalFeatures = ENABLE_CUSTOMIZED_UI_FLAG;
		
		if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != m_initSDKWorkFlow.InitSDK(initParam, true))
		{
			if (m_mainFrame)
				m_mainFrame->ShowErrorMessage(L"init sdk failed");
		}
		else
		{
			m_mainFrame->SwitchToWaitingPage(L"正在连接服务器...", true);
		}
	}
}

void  CInitSDKUIGroup::write_reg_sz()
{
	HKEY hkey;
	HKEY hTempKey;
	char m_empty_set[256] = "";

	LPCTSTR data_set = _T("SOFTWARE\\Classes");
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, data_set, 0, KEY_SET_VALUE, &hkey))
	{
		wchar_t error_msg[100] = { 0 };
		wsprintf(error_msg, L"write RegOpenKeyEx in..");
		OutputDebugString(error_msg);
		OutputDebugString(_T("\n"));

		if (ERROR_SUCCESS == ::RegCreateKey(hkey, _T("jinchan"), &hTempKey))
		{
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"write RegCreateKey in..");
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));

			if (ERROR_SUCCESS != ::RegSetValueEx(hTempKey, _T(""), 0, REG_SZ, (const BYTE*)_T("URL:jinchan"), sizeof(_T("URL:jinchan")) + 1))
			{
				wchar_t error_msg[100] = { 0 };
				wsprintf(error_msg, L"write reg 1 failed");
				OutputDebugString(error_msg);
				OutputDebugString(_T("\n"));
			}

			if (ERROR_SUCCESS != ::RegSetValueEx(hTempKey, _T("URL Protocol"), 0, REG_SZ, (const BYTE*)m_empty_set, sizeof(m_empty_set)+1))
			{
				wchar_t error_msg[100] = { 0 };
				wsprintf(error_msg, L"write reg 2 failed");
				OutputDebugString(error_msg);
				OutputDebugString(_T("\n"));
			}
		}
		else {
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"write RegCreateKey failed");
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
		}
	}
	else {
		wchar_t error_msg[100] = { 0 };
		wsprintf(error_msg, L"write RegOpenKeyEx failed");
		OutputDebugString(error_msg);
		OutputDebugString(_T("\n"));
	}
	::RegCloseKey(hkey);
}

void  CInitSDKUIGroup::write_reg_sz_icon()
{
	HKEY hkey;
	HKEY hTempKey;
	char m_empty_set[256] = "";

	LPCTSTR data_set = _T("SOFTWARE\\Classes\\jinchan");
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, data_set, 0, KEY_SET_VALUE, &hkey))
	{
		wchar_t error_msg[100] = { 0 };
		wsprintf(error_msg, L"write RegOpenKeyEx jinchan in..");
		OutputDebugString(error_msg);
		OutputDebugString(_T("\n"));

		if (ERROR_SUCCESS == ::RegCreateKey(hkey, _T("DefaultIcon"), &hTempKey))
		{
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"writejinchan  RegCreateKey in..");
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
			wchar_t exeFullPath[MAX_PATH]; // Full path   
			string strPath = "";
			GetModuleFileName(NULL, exeFullPath, MAX_PATH);
			//获取文件路径。
			//追加字符
			int nLen = wcslen(exeFullPath);
			wcscat_s(exeFullPath, nLen * 2, _T(",1"));

			TCHAR* lpStrPath = exeFullPath;
			//PathRemoveFileSpec(lpStrPath);
			OutputDebugString(lpStrPath);
			OutputDebugString(_T("\r\n"));


			if (ERROR_SUCCESS != ::RegSetValueEx(hTempKey, _T(""), 0, REG_SZ, (const BYTE*)lpStrPath, sizeof(exeFullPath) + 1))
			{
				wchar_t error_msg[100] = { 0 };
				wsprintf(error_msg, L"write reg 1 failed");
				OutputDebugString(error_msg);
				OutputDebugString(_T("\n"));
			}

		}
		else {
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"write jinchan RegCreateKey failed");
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
		}
	}
	else {
		wchar_t error_msg[100] = { 0 };
		wsprintf(error_msg, L"write RegOpenKeyEx  jinchan failed");
		OutputDebugString(error_msg);
		OutputDebugString(_T("\n"));
	}
	::RegCloseKey(hkey);
}


void  CInitSDKUIGroup::write_reg_sz_cmd()
{
	HKEY hkey;
	HKEY hTempKey;
	char m_empty_set[256] = "";

	LPCTSTR data_set = _T("SOFTWARE\\Classes\\jinchan");
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, data_set, 0, KEY_SET_VALUE, &hkey))
	{
		wchar_t error_msg[100] = { 0 };
		wsprintf(error_msg, L"write RegOpenKeyEx in..");
		OutputDebugString(error_msg);
		OutputDebugString(_T("\n"));

		if (ERROR_SUCCESS == ::RegCreateKey(hkey, _T("shell\\open\\command"), &hTempKey))
		{
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"write RegCreateKey in..");
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
			wchar_t exeFullPath[MAX_PATH]; // Full path   
			string strPath = "";
			GetModuleFileName(NULL, exeFullPath, MAX_PATH);
			//获取文件路径。
			//追加字符
			int nLen = wcslen(exeFullPath);
			wcscat_s(exeFullPath, nLen * 2, _T("\" \"%1\""));
			wchar_t wcs[MAX_PATH];

			wcscpy(wcs, _T("\""));
			wcscat(wcs, exeFullPath);
			TCHAR* lpStrPath = wcs;
			//PathRemoveFileSpec(lpStrPath);
			OutputDebugString(lpStrPath);
			OutputDebugString(_T("\r\n"));


			if (ERROR_SUCCESS != ::RegSetValueEx(hTempKey, _T(""), 0, REG_SZ, (const BYTE*)lpStrPath, sizeof(wcs) + 1))
			{
				wchar_t error_msg[100] = { 0 };
				wsprintf(error_msg, L"write reg 1 failed");
				OutputDebugString(error_msg);
				OutputDebugString(_T("\n"));
			}

		}
		else {
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"write RegCreateKey failed");
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
		}
	}
	else {
		wchar_t error_msg[100] = { 0 };
		wsprintf(error_msg, L"write RegOpenKeyEx failed");
		OutputDebugString(error_msg);
		OutputDebugString(_T("\n"));
	}
	::RegCloseKey(hkey);
}


void CInitSDKUIGroup::onSDKInited(ZOOM_SDK_NAMESPACE::SDKError err)
{
	if (m_mainFrame)
	{
		//m_mainFrame->SwitchToWaitingPage(NULL, false);
		if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != err)
		{
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"sdk init failed-%d", err);
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
			//m_mainFrame->ShowErrorMessage(error_msg);
			if (m_mainFrame->GetAppEvent())
			{
				m_mainFrame->GetAppEvent()->onQuitApp();
			}
		}
		else
		{
			
			write_reg_sz();
			write_reg_sz_icon();
			write_reg_sz_cmd();
			m_mainFrame->SwitchToAuthPage(true);
		}
	}
}
//////////////////////////////////////////////

CAuthSDKUIGroup::CAuthSDKUIGroup()
{
	m_AuthSDKPage = NULL;
	m_btnAuth = NULL;
	m_editSDKKey = NULL;
	m_editSDKSecret = NULL;
	m_mainFrame = NULL;
	m_chkUseJWTToken = NULL;
}

CAuthSDKUIGroup::~CAuthSDKUIGroup()
{
	UninitWindow();
}

void CAuthSDKUIGroup::InitWindow(CPaintManagerUI& ui_mgr, CSDKInitAuthUIMgr* main_frame_)
{
	m_AuthSDKPage = static_cast<CVerticalLayoutUI* >(ui_mgr.FindControl(_T("page_sdkauth")));
	m_btnAuth = static_cast<CButtonUI* >(ui_mgr.FindControl(_T("btn_auth")));
	m_editSDKKey = static_cast<CRichEditUI* >(ui_mgr.FindControl(_T("edit_sdk_key")));
	m_editSDKSecret = static_cast<CRichEditUI* >(ui_mgr.FindControl(_T("edit_sdk_secret")));
	m_chkUseJWTToken = static_cast<CCheckBoxUI* >(ui_mgr.FindControl(_T("chk_jwttoken_auth")));
	m_mainFrame = main_frame_;
	m_AuthSDKWorkFlow.SetUIEvent(this);
}

void CAuthSDKUIGroup::UninitWindow()
{
	m_AuthSDKWorkFlow.SetUIEvent(NULL);
	m_AuthSDKWorkFlow.Cleanup();
}

void CAuthSDKUIGroup::Show()
{
	if (m_AuthSDKPage)
	{
		m_AuthSDKPage->SetVisible(false);
		if (m_mainFrame)
		{
			m_mainFrame->SetCurrentPage(m_AuthSDKPage);
			DoAuthBtnClick();		
		}
	}
}

void CAuthSDKUIGroup::Hide()
{
	if (m_AuthSDKPage)
	{
		m_AuthSDKPage->SetVisible(false);
	}
}

void CAuthSDKUIGroup::Notify( TNotifyUI& msg )
{
	if(msg.sType == _T("click"))
	{
		if(msg.pSender == m_btnAuth)
		{
			DoAuth();
		}
		if(msg.pSender == m_chkUseJWTToken)
		{
			DoUseJWTTokenChkClick();
		}
	}
}

void CAuthSDKUIGroup::DoUseJWTTokenChkClick()
{
	if(NULL == m_chkUseJWTToken)
		return;
	if(!m_chkUseJWTToken->GetCheck())
	{
		if(m_editSDKSecret)
		{
			m_strOldValue = m_editSDKSecret->GetText().GetData();
			m_editSDKSecret->SetAttribute(_T("password"), _T("false"));
			m_editSDKSecret->SetReadOnly(true);
			m_editSDKSecret->SetText(_T(""));
			m_editSDKSecret->SetAttribute(_T("prompttext"), _T("No Use"));
			m_editSDKKey->SetAttribute(_T("prompttext"), _T("JWT token"));
			m_editSDKSecret->Invalidate();
			m_editSDKKey->Invalidate();
		}
	}
	else
	{
		if(m_editSDKSecret)
		{
			m_editSDKSecret->SetAttribute(_T("password"), _T("true"));
			m_editSDKSecret->SetText(m_strOldValue.c_str());
			m_editSDKSecret->SetReadOnly(false);
			m_editSDKSecret->SetAttribute(_T("prompttext"), _T("sdk secret"));
			m_editSDKKey->SetAttribute(_T("prompttext"), _T("sdk key"));
			m_editSDKSecret->Invalidate();
			m_editSDKKey->Invalidate();
		}
	}
}

void CAuthSDKUIGroup::DoAuth()
{
	if(m_chkUseJWTToken && m_chkUseJWTToken->GetCheck())
		DoJWTTokenBtnClick();
	else
		DoAuthBtnClick();		
}
void CAuthSDKUIGroup::DoJWTTokenBtnClick()
{
	if(NULL == m_editSDKKey)
		return;
	std::wstring strToken = _T("afFG4PCSpjKMory9SVgpgK1AMcPuPg6PgBp6"); //m_editSDKKey->GetText().GetData();
	if (strToken.size() > 0 )
	{
		ZOOM_SDK_NAMESPACE::AuthContext param;
		param.jwt_token = strToken.c_str();
		if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != m_AuthSDKWorkFlow.Auth(param))
		{
			if (m_mainFrame)
				m_mainFrame->ShowErrorMessage(L"auth sdk failed");
		}
		else
		{
			m_mainFrame->SwitchToWaitingPage(L"正在认证服务器...", true);
		}
	}
}


void CAuthSDKUIGroup::DoAuthBtnClick()
{
	if(NULL == m_editSDKKey || NULL == m_editSDKSecret)
		return;
	std::wstring strKey = _T("v9GTGRBNiRqdNFB5yri8oCyEcmrshS1FnBWC");// m_editSDKKey->GetText().GetData();
	std::wstring strSecret = _T("xVMM7l6BdICDZrVlrOYsUKVNSZ1mxpM5jUez");// m_editSDKSecret->GetText().GetData();

	if (strKey.size() > 0 && strSecret.size() > 0)
	{
		ZOOM_SDK_NAMESPACE::AuthParam param;
		param.appKey = strKey.c_str();
		param.appSecret = strSecret.c_str();
		if (ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS != m_AuthSDKWorkFlow.Auth(param))
		{
			//if (m_mainFrame)
			//	m_mainFrame->ShowErrorMessage(L"auth sdk failed");
			wchar_t error_msg[100] = { 0 };
			wsprintf(error_msg, L"auth sdk failed");
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
			if (m_mainFrame->GetAppEvent())
			{
				m_mainFrame->GetAppEvent()->onQuitApp();
			}
		}
		else
		{
			
			m_mainFrame->SwitchToWaitingPage(L"正在认证服务器...", true);
		}
	}
}

void CAuthSDKUIGroup::onSDKAuthed(ZOOM_SDK_NAMESPACE::AuthResult ret)
{
	if (m_mainFrame)
	{
		
		if (ZOOM_SDK_NAMESPACE::AUTHRET_SUCCESS != ret)
		{
			
			wchar_t error_msg[200] = { 0 };
			wsprintf(error_msg, CSDKMessInfo::GetInst().GetErrorMessInfo());
			OutputDebugString(error_msg);
			OutputDebugString(_T("\n"));
			if (m_mainFrame)
				m_mainFrame->ShowErrorMessage(L"认证失败");
			if (m_mainFrame->GetAppEvent())
			{
				m_mainFrame->GetAppEvent()->onQuitApp();
			}
		}
		else if (m_mainFrame->GetAppEvent())
		{
			m_mainFrame->SwitchToWaitingPage(NULL, false);
			m_mainFrame->GetAppEvent()->onSwitchToLoginUI(SwitchToLoginUIType_AUTHDONE);
		}
		else {
		m_mainFrame->SwitchToWaitingPage(NULL, false);}
	}
}
/////////////////////////////////////////
CSDKInitAuthUIMgr::CSDKInitAuthUIMgr()
{
	m_waitingPage = NULL;
	m_waitingLabelUI = NULL;
	m_currentPage = NULL;
}

CSDKInitAuthUIMgr::~CSDKInitAuthUIMgr()
{
	m_waitingPage = NULL;
	m_waitingLabelUI = NULL;
	m_currentPage = NULL;
}

void CSDKInitAuthUIMgr::SetEvent(CSDKDemoAppEvent* pAppEvent)
{
	m_pAppEvent = pAppEvent;
}

void CSDKInitAuthUIMgr::InitWindow()

{
	m_InitSDKUIGroup.InitWindow(m_PaintManager, this);
	m_AuthSDKUIGroup.InitWindow(m_PaintManager, this);
	RECT rc = { 0 };
	if( !::GetClientRect(m_hWnd, &rc) ) return;
	rc.right = rc.left + 524;
	rc.bottom = rc.top + 376;
	if( !::AdjustWindowRectEx(&rc, GetWindowStyle(m_hWnd), (!(GetWindowStyle(m_hWnd) & WS_CHILD) && (::GetMenu(m_hWnd) != NULL)), GetWindowExStyle(m_hWnd)) ) return;
	int ScreenX = GetSystemMetrics(SM_CXSCREEN);
	int ScreenY = GetSystemMetrics(SM_CYSCREEN);
	
	::SetWindowPos(m_hWnd, NULL, (ScreenX - (rc.right - rc.left)) / 2, 
		(ScreenY - (rc.bottom - rc.top)) / 2, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);

	
	m_waitingPage = static_cast<CVerticalLayoutUI* >(m_PaintManager.FindControl(_T("waiting")));
	m_waitingLabelUI = static_cast<CLabelUI* >(m_PaintManager.FindControl(_T("waiting_label")));
	m_gifWaiting = static_cast<CGifAnimUI*>(m_PaintManager.FindControl(_T("gif_waiting"))); 
	SwitchToWaitingPage(L"", false);
	m_InitSDKUIGroup.Show();
	m_AuthSDKUIGroup.Hide();
}

void CSDKInitAuthUIMgr::Notify(TNotifyUI& msg)
{            
	m_InitSDKUIGroup.Notify(msg);
	m_AuthSDKUIGroup.Notify(msg);
}

LRESULT CSDKInitAuthUIMgr::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;

	if( uMsg == WM_CREATE )  
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
		OnClose(uMsg, wParam, lParam, bHandled);		
	}
	else if (uMsg == WM_DESTROY)
	{
		OnDestroy(uMsg, wParam, lParam, bHandled);		
	}

	if( m_PaintManager.MessageHandler(uMsg, wParam, lParam, lRes) ) 
	{
		return lRes;
	}

	return __super::HandleMessage(uMsg, wParam, lParam);
}

LRESULT CSDKInitAuthUIMgr::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if (m_pAppEvent)
	{
		m_pAppEvent->onQuitApp();
	}
	return 0;
}

LRESULT CSDKInitAuthUIMgr::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_InitSDKUIGroup.UninitWindow();
	m_AuthSDKUIGroup.UninitWindow();
	return 0;
}

void CSDKInitAuthUIMgr::SwitchToWaitingPage(const wchar_t* waiting_message, bool show)
{
	if (m_currentPage)
	{
		m_currentPage->SetVisible(!show);
	}
	if (waiting_message && m_waitingLabelUI)
	{
		m_waitingLabelUI->SetText(waiting_message);
	}

	if (m_waitingPage)
	{
		m_waitingPage->SetVisible(show);
		if (m_gifWaiting)
		{
			show ? m_gifWaiting->PlayGif() : m_gifWaiting->StopGif();
		}
	}
}

void CSDKInitAuthUIMgr::ShowErrorMessage(const wchar_t* error_message)
{
	if (error_message)
		::MessageBox(NULL, error_message, L"提示", MB_OK);
}

void CSDKInitAuthUIMgr::SwitchToAuthPage(bool show)
{
	m_InitSDKUIGroup.Hide();
	m_AuthSDKUIGroup.Show();
}

CSDKDemoAppEvent* CSDKInitAuthUIMgr::GetAppEvent()
{
	return m_pAppEvent;
}
