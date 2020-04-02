#pragma once
#include <Windows.h>
#include "sdk_util.h"
#include "Resource.h"
#include "UIlib.h"
using namespace DuiLib;


DWORD WINAPI checkVersion(LPVOID lpParamter);

class CDownloadProgressUIMgr :
	public CWindowWnd,
	public INotifyUI
{
public:
	CDownloadProgressUIMgr();
	~CDownloadProgressUIMgr();

public:
	virtual void Notify(TNotifyUI& msg);
	virtual LPCTSTR GetWindowClassName() const { return _T("CSDKDemoUI"); }
	UINT GetClassStyle() const { return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; };
	virtual UINT		GetSkinRes() { return IDXML_DOWNLOAD_PROGRESS_UI; }
	UILIB_RESOURCETYPE GetResourceType() const { return UILIB_RESOURCE; }
	virtual void		OnFinalMessage(HWND) {}
	virtual LRESULT	HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void setDownloadUrl(wstring url);
	void SetEvent(CSDKDemoAppEvent* pAppEvent);

public:
	void InitWindow();
	void DoClickPauseBtn();
	void DoClickResumeBtn();
	void download(CDownloadProgressUIMgr *p);

public:
	CPaintManagerUI m_PaintManager;
	CButtonUI* m_pause_btn;
	CButtonUI* m_resume_btn;
	CProgressUI* download_progress;
	bool screenApp;
	wstring fileMd5;
	wstring mUrl;
	CSDKDemoAppEvent* m_pAppEvent;
};


