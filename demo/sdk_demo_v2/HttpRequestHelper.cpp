#include "stdafx.h"
#include <string>
#include "tchar.h"
#include "sdk_util.h"
#include "HttpRequestHelper.h"
#pragma comment(lib, "version.lib")

using namespace std;
std::wstring GetFileVersion(TCHAR * strFilePath)
{
	DWORD dwSize;
	DWORD dwRtn;
	std::string szVersion;
	//获取版本信息大小
	dwSize = GetFileVersionInfoSize(strFilePath, NULL);
	if (dwSize == 0)
	{
		return L"";
	}
	char *pBuf;
	pBuf = new char[dwSize + 1];
	if (pBuf == NULL)
		return L"";
	memset(pBuf, 0, dwSize + 1);
	//获取版本信息
	dwRtn = GetFileVersionInfo(strFilePath, NULL, dwSize, pBuf);
	if (dwRtn == 0)
	{
		return L"";
	}
	LPVOID lpBuffer = NULL;
	UINT uLen = 0;
	//版本资源中获取信息

	dwRtn = VerQueryValue(pBuf,
		TEXT("\\StringFileInfo\\080404b0\\FileVersion"), //0804中文
															 //04b0即1252,ANSI
															 //可以从ResourceView中的Version中BlockHeader中看到
															 //可以测试的属性
															 /*
															 CompanyName
															 FileDescription
															 FileVersion
															 InternalName
															 LegalCopyright
															 OriginalFilename
															 ProductName
															 ProductVersion
															 Comments
															 LegalTrademarks
															 PrivateBuild
															 SpecialBuild
															 */
		&lpBuffer,
		&uLen);
	if (dwRtn == 0)
	{
		return L"";
	}
	szVersion = (char*)lpBuffer;
	delete pBuf;
	const char* char_ = szVersion.c_str();
	TCHAR* tchar_;
	int iLength;
	iLength = MultiByteToWideChar(CP_ACP, 0, char_, strlen(char_), NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, char_, strlen(char_), tchar_, iLength);
	if (NULL != &tchar_[iLength])
		tchar_[iLength] = '\0';
	return tchar_;
}

#define PRODUCT_HOST_URL _T("https://api.zoom.us/v2/users/")
#define DEV_HOST_URL _T("https://dev.zoom.us/v2/users/")
#define APP_VERSION 108
#define VERSION_HOST_CHECK_URL _T("http://39.107.127.11:9966/attachment/checkVersion?version=108")

#define APP_SCREEN_VERSION 1
#define VERSION_HOST_CHECK_SCREEN_URL _T("http://39.107.127.11:9966/attachment/checkScreenVersion?version=1")

CZoomHttpRequestHelper::CZoomHttpRequestHelper()
{
	m_strAccessToken = NULL;
	m_strMeetingStartUrl = NULL;
	m_strUserEmail = NULL;
	m_strAPIURLUserToken[0] = 0;
	m_strAPIURLZakToken[0] = 0;
	m_strUserID[0] = 0;
	m_userPMI = 0;
}

CZoomHttpRequestHelper::~CZoomHttpRequestHelper()
{

}

void CZoomHttpRequestHelper::SetRestAPIAccessToken(wchar_t* strAccessToken)
{
	if(strAccessToken)
		m_strAccessToken = strAccessToken;
}

void CZoomHttpRequestHelper::SetRestAPIDomain(wchar_t* strDomain)
{
	if(strDomain)
		m_strDomain = strDomain;
}

void CZoomHttpRequestHelper::SetUserEmail(wchar_t* strUserEmail)
{
	if(strUserEmail)
		m_strUserEmail = strUserEmail;
}

bool CZoomHttpRequestHelper::CheckVersion()
{
	bool bRet(false);
	TCHAR szRealUrl[1024];
	TCHAR szHostName[1024];
	TCHAR szUrlPath[1024];
	wstring  url = VERSION_HOST_CHECK_URL;
	wsprintf(szRealUrl, url.c_str());

	OutputDebugString(szRealUrl);
	OutputDebugString(_T("\n"));
	URL_COMPONENTS crackedURL = { 0 };
	crackedURL.dwStructSize = sizeof(URL_COMPONENTS);
	crackedURL.lpszHostName = szHostName;
	crackedURL.dwHostNameLength = ARRAYSIZE(szHostName);
	crackedURL.lpszUrlPath = szUrlPath;
	crackedURL.dwUrlPathLength = ARRAYSIZE(szUrlPath);
	InternetCrackUrl(szRealUrl, (DWORD)_tcslen(szRealUrl), 0, &crackedURL);
	HINTERNET hInternet = InternetOpen(_T("Microsoft InternetExplorer"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL)
		return false;

	HINTERNET hHttpSession = InternetConnect(hInternet, crackedURL.lpszHostName, crackedURL.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hHttpSession == NULL)
	{
		OutputDebugString(_T("hHttpSession is null"));
		OutputDebugString(_T("\n"));
		InternetCloseHandle(hInternet);
		return false;
	}
	DWORD dwFlags =   INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI;

	HINTERNET hHttpRequest = HttpOpenRequest(hHttpSession, _T("GET"), crackedURL.lpszUrlPath, NULL, _T(""), NULL, dwFlags, 0);
	if (hHttpRequest == NULL)
	{
		InternetCloseHandle(hHttpSession);
		InternetCloseHandle(hInternet);
		OutputDebugString(_T("hHttpRequest is null"));
		OutputDebugString(_T("\n"));
		return false;
	}

	DWORD dwRetCode = 0;
	DWORD dwSizeOfRq = sizeof(DWORD);

	if (!HttpSendRequest(hHttpRequest, NULL, 0, NULL, 0) ||
		!HttpQueryInfo(hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwRetCode, &dwSizeOfRq, NULL)
		|| dwRetCode >= 400)
	{
		InternetCloseHandle(hHttpRequest);
		InternetCloseHandle(hHttpSession);
		InternetCloseHandle(hInternet);
		OutputDebugString(_T("HttpSendRequest is null"));
		OutputDebugString(_T("\n"));
		return false;
	}

	DWORD dwContentLen;

	if (!InternetQueryDataAvailable(hHttpRequest, &dwContentLen, 0, 0) || dwContentLen == 0)
	{
		InternetCloseHandle(hHttpRequest);
		InternetCloseHandle(hHttpSession);
		InternetCloseHandle(hInternet);
		OutputDebugString(_T("InternetQueryDataAvailable is null"));
		OutputDebugString(_T("\n"));
		return false;
	}

	

	DWORD dwError;
	DWORD dwBytesRead;
	DWORD nCurrentBytes = 0;
	char szBuffer[1025] = { 0 };
	string data = "";
	while (TRUE)
	{
		if (InternetReadFile(hHttpRequest, szBuffer,1024, &dwBytesRead))
		{
			if (dwBytesRead == 0)
			{
				break;
			}
		
			szBuffer[dwBytesRead] = '\0';
			data += szBuffer;
			ZeroMemory(szBuffer, 1024);
		}
		else
		{
			dwError = GetLastError();
			OutputDebugString(_T("InternetReadFile is null"));
			OutputDebugString(_T("\n"));
			break;
		}
	}

	
	InternetCloseHandle(hInternet);
	InternetCloseHandle(hHttpSession);
	InternetCloseHandle(hHttpRequest);

	return NeedUpgradeVersion(data);
}

bool CZoomHttpRequestHelper::CheckScreenVersion()
{
	bool bRet(false);
	TCHAR szRealUrl[1024];
	TCHAR szHostName[1024];
	TCHAR szUrlPath[1024];
	MD5* md5 = new MD5();
	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	TCHAR szFileName[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
	wsprintf(szFileName, _T("%s%s"), szFilePath, _T("SiMayService.exe"));
	wstring md5Info = md5->GetFileMd5(szFileName);
	wstring fileMd5 = _T("&md5=")+ md5Info;
	this->fileMd5 = md5Info;
	wstring  url = VERSION_HOST_CHECK_SCREEN_URL+ fileMd5;
	wsprintf(szRealUrl, url.c_str());

	OutputDebugString(szRealUrl);
	OutputDebugString(_T("\n"));
	URL_COMPONENTS crackedURL = { 0 };
	crackedURL.dwStructSize = sizeof(URL_COMPONENTS);
	crackedURL.lpszHostName = szHostName;
	crackedURL.dwHostNameLength = ARRAYSIZE(szHostName);
	crackedURL.lpszUrlPath = szUrlPath;
	crackedURL.dwUrlPathLength = ARRAYSIZE(szUrlPath);
	InternetCrackUrl(szRealUrl, (DWORD)_tcslen(szRealUrl), 0, &crackedURL);
	HINTERNET hInternet = InternetOpen(_T("Microsoft InternetExplorer"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL)
		return false;

	HINTERNET hHttpSession = InternetConnect(hInternet, crackedURL.lpszHostName, crackedURL.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hHttpSession == NULL)
	{
		OutputDebugString(_T("hHttpSession is null"));
		OutputDebugString(_T("\n"));
		InternetCloseHandle(hInternet);
		return false;
	}
	DWORD dwFlags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI;

	HINTERNET hHttpRequest = HttpOpenRequest(hHttpSession, _T("GET"), crackedURL.lpszUrlPath, NULL, _T(""), NULL, dwFlags, 0);
	if (hHttpRequest == NULL)
	{
		InternetCloseHandle(hHttpSession);
		InternetCloseHandle(hInternet);
		OutputDebugString(_T("hHttpRequest is null"));
		OutputDebugString(_T("\n"));
		return false;
	}

	DWORD dwRetCode = 0;
	DWORD dwSizeOfRq = sizeof(DWORD);

	if (!HttpSendRequest(hHttpRequest, NULL, 0, NULL, 0) ||
		!HttpQueryInfo(hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwRetCode, &dwSizeOfRq, NULL)
		|| dwRetCode >= 400)
	{
		InternetCloseHandle(hHttpRequest);
		InternetCloseHandle(hHttpSession);
		InternetCloseHandle(hInternet);
		OutputDebugString(_T("HttpSendRequest is null"));
		OutputDebugString(_T("\n"));
		return false;
	}

	DWORD dwContentLen;

	if (!InternetQueryDataAvailable(hHttpRequest, &dwContentLen, 0, 0) || dwContentLen == 0)
	{
		InternetCloseHandle(hHttpRequest);
		InternetCloseHandle(hHttpSession);
		InternetCloseHandle(hInternet);
		OutputDebugString(_T("InternetQueryDataAvailable is null"));
		OutputDebugString(_T("\n"));
		return false;
	}



	DWORD dwError;
	DWORD dwBytesRead;
	DWORD nCurrentBytes = 0;
	char szBuffer[1025] = { 0 };
	string data = "";
	while (TRUE)
	{
		if (InternetReadFile(hHttpRequest, szBuffer, 1024, &dwBytesRead))
		{
			if (dwBytesRead == 0)
			{
				break;
			}

			szBuffer[dwBytesRead] = '\0';
			data += szBuffer;
			ZeroMemory(szBuffer, 1024);
		}
		else
		{
			dwError = GetLastError();
			OutputDebugString(_T("InternetReadFile is null"));
			OutputDebugString(_T("\n"));
			break;
		}
	}


	InternetCloseHandle(hInternet);
	InternetCloseHandle(hHttpSession);
	InternetCloseHandle(hHttpRequest);

	return NeedUpgradeScreenVersion(data);
}


bool CZoomHttpRequestHelper::NeedUpgradeScreenVersion(string str)
{

	char* pzFirst = NULL;
	char* pzSecond = NULL;

	//get user id first
	if (str.find("http") == string::npos)//不存在。
	{
		return false;
	}
	//pzSecond = strstr(buf, "\"status\":false");
	int versionCode = APP_SCREEN_VERSION;

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	if (wstrTo.empty()) {
		return false;
	}

	this->downloadUrl = wstrTo;
	return true;
}


bool CZoomHttpRequestHelper::NeedUpgradeVersion(string str)
{
	
	char* pzFirst = NULL;
	char* pzSecond = NULL;
	
	//get user id first
	if (str.find("http") == string::npos)//不存在。
	{
		return false;
	}
	//pzSecond = strstr(buf, "\"status\":false");
	int versionCode = APP_VERSION;

	 int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	if (wstrTo.empty()) {
		return false;
	}

	this->downloadUrl = wstrTo;
	return true;
}

bool CZoomHttpRequestHelper::MakeAPIUrlTokens()
{
	do
	{
		if(NULL == m_strAccessToken || NULL == m_strUserEmail || NULL == m_strDomain)
			break;
	
		if(!getAPIUserInfo(info_user_token))
			break;
		
		if(!getAPIUserInfo(info_user_zak))
			break;
		
		if(!getAPIUserInfo(info_user_basic))
			break;

		if(NULL != m_strAPIURLUserToken && NULL !=m_strAPIURLZakToken)
			return true;
	}while(0);
	return false;
}

bool CZoomHttpRequestHelper::getAPIUserInfo(infoType nType)
{
	bool bRet(false);
	TCHAR szRealUrl[1024];
	TCHAR szHostName[1024];
	TCHAR szUrlPath[1024];

	switch(nType)
	{
	case info_user_token:
		wsprintf(szRealUrl,_T("https://api.%s%s%s/token?access_token=%s"), m_strDomain, _T("/v2/users/"),m_strUserEmail,m_strAccessToken);
		break;
	case info_user_zak:
		wsprintf(szRealUrl,_T("https://api.%s%s%s/token?access_token=%s&type=zak"), m_strDomain, _T("/v2/users/"),m_strUserEmail,m_strAccessToken);
		break;
	case info_user_basic:
		wsprintf(szRealUrl,_T("https://api.%s%s%s?access_token=%s"), m_strDomain, _T("/v2/users/"),m_strUserEmail,m_strAccessToken);
		break;
	default:
		break;
	}
	OutputDebugString(szRealUrl);
	OutputDebugString(_T("\n"));
	URL_COMPONENTS crackedURL = { 0 };
	crackedURL.dwStructSize = sizeof (URL_COMPONENTS);
	crackedURL.lpszHostName = szHostName;
	crackedURL.dwHostNameLength = ARRAYSIZE(szHostName);
	crackedURL.lpszUrlPath = szUrlPath;
	crackedURL.dwUrlPathLength = ARRAYSIZE(szUrlPath);
	InternetCrackUrl(szRealUrl, (DWORD)_tcslen(szRealUrl), 0, &crackedURL);
    HINTERNET hInternet = InternetOpen(_T("Microsoft InternetExplorer"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL)
        return false;
 
    HINTERNET hHttpSession = InternetConnect(hInternet, crackedURL.lpszHostName, crackedURL.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (hHttpSession == NULL)
    {
        InternetCloseHandle(hInternet);
        return false;
    }
	DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE;

    HINTERNET hHttpRequest = HttpOpenRequest(hHttpSession, _T("GET"), crackedURL.lpszUrlPath, NULL, _T(""), NULL, dwFlags, 0);
    if (hHttpRequest == NULL)
    {
        InternetCloseHandle(hHttpSession);
        InternetCloseHandle(hInternet);
        return false;
    }
   
    DWORD dwRetCode = 0;
    DWORD dwSizeOfRq = sizeof(DWORD);

    if (!HttpSendRequest(hHttpRequest, NULL, 0, NULL, 0) ||
        !HttpQueryInfo(hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwRetCode, &dwSizeOfRq, NULL)
        || dwRetCode >= 400)
    {
        InternetCloseHandle(hHttpRequest);
        InternetCloseHandle(hHttpSession);
        InternetCloseHandle(hInternet);
        return false;
    }

    DWORD dwContentLen;

	if (!InternetQueryDataAvailable(hHttpRequest, &dwContentLen, 0, 0) || dwContentLen == 0)
    {
        InternetCloseHandle(hHttpRequest);
        InternetCloseHandle(hHttpSession);
        InternetCloseHandle(hInternet);
        return false;
    }

	TCHAR szFilePath[MAX_PATH + 1]={0};
	TCHAR szFileName[MAX_PATH + 1]={0};
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
	wsprintf(szFileName,_T("%s%s"),szFilePath,_T("result.xml"));

    FILE* file = _wfopen(szFileName, _T("wb+"));
    if (file == NULL)
    {
        InternetCloseHandle(hHttpRequest);
        InternetCloseHandle(hHttpSession);
        InternetCloseHandle(hInternet); 
        return false;
    } 
	
    DWORD dwError;
    DWORD dwBytesRead;
    DWORD nCurrentBytes = 0;
    wchar_t szBuffer[1024] = {0};
    while (TRUE)
    {
        if (InternetReadFile(hHttpRequest, szBuffer, sizeof(szBuffer), &dwBytesRead))
        {
            if (dwBytesRead == 0)
            {
                break;
            }
            nCurrentBytes += dwBytesRead;
            fwrite(szBuffer, 1, dwBytesRead, file);
        }
        else
        {
            dwError = GetLastError();
            break;
        }
    }
  
    fclose(file);
    InternetCloseHandle(hInternet);
    InternetCloseHandle(hHttpSession);
    InternetCloseHandle(hHttpRequest);
	
	if(GetTokenFromFile(nType))
		bRet = true;
	
	return bRet;
}

bool CZoomHttpRequestHelper::WcharToChar(const TCHAR * tchar_, char * char_)
{  
	int iLength ;  
	iLength = WideCharToMultiByte(CP_ACP, 0, tchar_, -1, NULL, 0, NULL, NULL);  
	WideCharToMultiByte(CP_ACP, 0, tchar_, -1, char_, iLength, NULL, NULL);
	return true;
} 

bool CZoomHttpRequestHelper::CharToWchar (const char * char_, TCHAR * tchar_)
{  
	int iLength ;  
	iLength = MultiByteToWideChar(CP_ACP, 0, char_, strlen (char_), NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, char_, strlen (char_), tchar_, iLength);
	if(NULL != &tchar_[iLength])
		tchar_[iLength] = '\0';
	return true;
}

bool CZoomHttpRequestHelper::GetTokenFromFile(infoType nType)
{
	if(nType == info_user_basic)
	{
		return GetUserBasicInfoFromFile();
	}
	TCHAR szFilePath[MAX_PATH + 1]={0};
	TCHAR szFileName[MAX_PATH + 1]={0};
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
	wsprintf(szFileName,_T("%s%s"),szFilePath,_T("result.xml"));

	FILE* file = _wfopen(szFileName, _T("rb+"));
	if (file == NULL)
	{
		return false;
	}
	long length = 0;
	fseek( file, 0, SEEK_END );
	length = ftell( file );
	fseek( file, 10, SEEK_SET );

	if ( length <= 0 )
	{
		fclose(file);
		DeleteTempFile(szFileName);
		return false;
	}

	length = length - 12;
	char* buf = new char[ length +1 ];
	buf[0] = 0;
	long realReadLen = fread( buf,1, length, file );
	if (length != realReadLen)
	{
		delete [] buf;
		fclose(file);
		DeleteTempFile(szFileName);
		return false;
	}
	buf[length]= 0;
	
	if(info_user_token == nType)					
		CharToWchar(buf, m_strAPIURLUserToken);
	else if (info_user_zak == nType)
		CharToWchar(buf, m_strAPIURLZakToken);
	delete [] buf;
	fclose(file);
	DeleteTempFile(szFileName);
	return true;
}

bool CZoomHttpRequestHelper::GetUserBasicInfoFromFile()
{
	TCHAR szFilePath[MAX_PATH + 1]={0};
	TCHAR szFileName[MAX_PATH + 1]={0};
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
	wsprintf(szFileName,_T("%s%s"),szFilePath,_T("result.xml"));

	FILE* file = _wfopen(szFileName, _T("rb+"));
	if (file == NULL)
	{
		return false;
	}
	long length = 0;
	fseek( file, 0, SEEK_END );
	length = ftell( file );
	if ( length <= 0 )
	{
		fclose(file);
		DeleteTempFile(szFileName);
		return false;
	}
	rewind(file);
	char* buf = new char[ length +1 ];
	char* pzFirst = NULL;
	char* pzSecond = NULL;
	char cstrUserID[32] = {0};
	buf[0] = 0;
	fread(buf, length, length, file);
	fclose(file); 
	buf[length]= 0;
	//get user id first
	pzFirst = strstr(buf, "\"id\":\"");
	pzSecond = strstr(buf, "\",\"first_name");
	if(pzFirst && pzSecond)
	{
		int i;
		for( i=0 ; pzFirst[i+6] != pzSecond[0] && i<31; i++)
		{
			cstrUserID[i] = pzFirst[i+6];
		}

		int myint1 = std::stoi(cstrUserID);

	}
	else
	{
		fclose(file);
		DeleteTempFile(szFileName);
		return false;
	}
	//get PMI then
	pzFirst = strstr(buf, "\"pmi\":");
	pzSecond = strstr(buf, ",\"use_pmi\"");
	memset(cstrUserID, '\0', 32);
	if(pzFirst && pzSecond)
	{
		int i;
		for( i=0 ; pzFirst[i+6] != pzSecond[0] && i<31; i++)
		{
			cstrUserID[i] = pzFirst[i+6];
		}
		m_userPMI = _atoi64(cstrUserID);
	}
	else
	{
		fclose(file);
		DeleteTempFile(szFileName);
		return false;
	}
	delete [] buf;
	DeleteTempFile(szFileName);
	return true;
}

void CZoomHttpRequestHelper::DeleteTempFile(wchar_t* strFileDelete)
{
	char pcFileName[MAX_PATH + 1] = {0};
	WcharToChar(strFileDelete, pcFileName);
	remove(pcFileName);
}