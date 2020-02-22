#include "stdafx.h"
#include "sdk_demo_app.h"
#include "zoomHmacSHA256.h"
#include <regex>
#pragma comment(lib,"ws2_32.lib")

#define ops_bswap_64(val) (((val) >> 56) |\
                    (((val) & 0x00ff000000000000ll) >> 40) |\
                    (((val) & 0x0000ff0000000000ll) >> 24) |\
                    (((val) & 0x000000ff00000000ll) >> 8)   |\
                    (((val) & 0x00000000ff000000ll) << 8)   |\
                    (((val) & 0x0000000000ff0000ll) << 24) |\
                    (((val) & 0x000000000000ff00ll) << 40) |\
                    (((val) << 56)))

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ws_hton64(ll) (ops_bswap_64(ll))
#define ws_ntoh64(ll) (ops_bswap_64(ll))
#else
#define ws_hton64(ll) (ll)
#define ws_ntoh64(ll) (ll)
#endif

HANDLE   handle;
using namespace DuiLib;

class WSProtocol
{
public:
	WSProtocol() :statusWebSocketConnection(0), bIsClose(false)
	{
	}
	bool isClose() const { return bIsClose; }
	bool isWebSocketConnection() const { return  statusWebSocketConnection == 1; }
	const std::map<std::string, std::string>& getParam() const { return dictParams; }
	const std::vector<std::string>& getSendPkg()
	{
		return listSendPkg;
	}
	void clearSendPkg()
	{
		listSendPkg.clear();
	}
	void addSendPkg(const std::string& data)
	{
		listSendPkg.push_back(data);
	}
	const std::vector<std::string>& getRecvPkg()
	{
		return listRecvPkg;
	}
	void clearRecvPkg()
	{
		listRecvPkg.clear();
	}
	void addRecvPkg(const std::string& data)
	{
		if (data.size() > 0)
		{
			listRecvPkg.push_back(data);
		}
	}
	int strSplit(const std::string& str, std::vector<std::string>& ret_, std::string sep = ",")
	{
		if (str.empty())
		{
			return 0;
		}

		std::string tmp;
		std::string::size_type pos_begin = str.find_first_not_of(sep);
		std::string::size_type comma_pos = 0;

		while (pos_begin != std::string::npos)
		{
			comma_pos = str.find(sep, pos_begin);
			if (comma_pos != std::string::npos)
			{
				tmp = str.substr(pos_begin, comma_pos - pos_begin);
				pos_begin = comma_pos + sep.length();
			}
			else
			{
				tmp = str.substr(pos_begin);
				pos_begin = comma_pos;
			}

			if (!tmp.empty())
			{
				ret_.push_back(tmp);
				tmp.clear();
			}
		}
		return 0;
	}

	std::wstring s2ws(const std::string& str)
	{
		int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), 0, 0);
		if (nSize <= 0) return NULL;
		WCHAR *pwszDst = new WCHAR[nSize + 1];
		if (NULL == pwszDst)
			return NULL;
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), pwszDst, nSize);
		pwszDst[nSize] = 0;
		if (pwszDst[0] == 0xFEFF) // skip Oxfeff
			for (int i = 0; i < nSize; i++)
				pwszDst[i] = pwszDst[i + 1];
		wstring wcharString(pwszDst);
		delete pwszDst;
		return wcharString;
	}

	bool handleRecv(const char* buff, size_t len)
	{
		if (statusWebSocketConnection == -1)
		{
			return false;
		}
		cacheRecvData.append(buff, len);
		if (dictParams.empty() == true)
		{
			std::string& strRecvData = cacheRecvData;
			if (strRecvData.size() >= 3)
			{
				if (strRecvData.find("GET") == std::string::npos)
				{
					statusWebSocketConnection = -1;
					return false;
				}
			}
			else if (strRecvData.size() >= 2)
			{
				if (strRecvData.find("GE") == std::string::npos)
				{
					statusWebSocketConnection = -1;
					return false;
				}
			}
			else
			{
				if (strRecvData.find("G") == std::string::npos)
				{
					statusWebSocketConnection = -1;
					return false;
				}
			}
			statusWebSocketConnection = 1;
			//if (strRecvData.find("\r\n\r\n") == std::string::npos)//!header data not end
			//{
			//	OutputDebugString(_T("strRecvData.find..."));
			//	return true;
			//}
			if (strRecvData.find("Upgrade: websocket") == std::string::npos)
			{
				statusWebSocketConnection = -1;
				return false;
			}
			std::vector<std::string> strLines;
			strSplit(strRecvData, strLines, "\r\n");
			for (size_t i = 0; i < strLines.size(); ++i)
			{
				OutputDebugString(_T("strSplit.find...\n"));
				
				const std::string& line = strLines[i];
				OutputDebugString(s2ws(line).c_str());
				std::vector<std::string> strParams;
				strSplit(line, strParams, ":");
				if (strParams.size() == 2)
				{
					dictParams[strParams[0]] = strParams[1];
				}
				else if (strParams.size() == 1 && strParams[0].find("GET") != std::string::npos)
				{
					dictParams["PATH"] = strParams[0];
				}
			}


			if (dictParams.find("Sec-WebSocket-Key") != dictParams.end())
			{
				const std::string& Sec_WebSocket_Key = dictParams["Sec-WebSocket-Key"];
				std::string strGUID = Sec_WebSocket_Key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
				std::string dataHashed = sha1Encode(strGUID);
				std::string strHashBase64 = base64Encode(dataHashed.c_str(), dataHashed.length(), false);

				char buff[512] = { 0 };
				snprintf(buff, sizeof(buff), "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", strHashBase64.c_str());

				addSendPkg(buff);
			}
			else if (dictParams.find("Sec-WebSocket-Key1") != dictParams.end())
			{
				std::string handshake = "HTTP/1.1 101 Web Socket Protocol Handshake\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\n";

				std::string str_origin = dictParams["Origin"];
				if (str_origin.empty())
				{
					str_origin = "null";
				}
				handshake += std::string("Sec-WebSocket-Origin: ") + str_origin + "\r\n";

				std::string str_host = dictParams["Host"];
				if (false == str_host.empty())
				{
					std::vector<std::string> tmp_path_arg;
					strSplit(strLines[0], tmp_path_arg, " ");
					std::string tmp_path = "/";
					if (tmp_path_arg.size() >= 2)
					{
						tmp_path = tmp_path_arg[1];
					}

					handshake += std::string("Sec-WebSocket-Location: ws://") + dictParams["Host"] + tmp_path + "\r\n\r\n";
				}

				uint32_t key1 = computeWebsokcetKeyVal(dictParams["Sec-WebSocket-Key1"]);
				uint32_t key2 = computeWebsokcetKeyVal(dictParams["Sec-WebSocket-Key2"]);

				std::string& key_ext = strLines[strLines.size() - 1];
				if (key_ext.size() < 8)
				{
					statusWebSocketConnection = -1;
					return false;
				}

				char tmp_buff[16] = { 0 };
				memcpy(tmp_buff, (const char*)(&key1), sizeof(key1));
				memcpy(tmp_buff + sizeof(key1), (const char*)(&key2), sizeof(key2));
				memcpy(tmp_buff + sizeof(key1) + sizeof(key2), key_ext.c_str(), 8);

				handshake += computeMd5(tmp_buff, sizeof(tmp_buff));
				addSendPkg(handshake);
			}
			else
			{
				statusWebSocketConnection = -1;
				return false;
			}
			cacheRecvData.clear();
			return true;
		}
		int nFIN = ((cacheRecvData[0] & 0x80) == 0x80) ? 1 : 0;

		int nOpcode = cacheRecvData[0] & 0x0F;
		//int nMask = ((cacheRecvData[1] & 0x80) == 0x80) ? 1 : 0; //!this must be 1
		int nPayload_length = cacheRecvData[1] & 0x7F;
		int nPlayLoadLenByteNum = 1;
		int nMaskingKeyByteNum = 4;

		if (nPayload_length == 126)
		{
			nPlayLoadLenByteNum = 3;
		}
		else if (nPayload_length == 127)
		{
			nPlayLoadLenByteNum = 9;
		}
		if ((int)cacheRecvData.size() < (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum))
		{
			return true;
		}
		if (nPayload_length == 126)
		{
			uint16_t tmpLen = 0;
			memcpy(&tmpLen, cacheRecvData.c_str() + 2, 2);
			nPayload_length = ntohs((int16_t)tmpLen);
		}
		else if (nPayload_length == 127)
		{
			int64_t tmpLen = 0;
			memcpy(&tmpLen, cacheRecvData.c_str() + 2, 8);
			nPayload_length = (int)ws_ntoh64(tmpLen);
		}

		if ((int)cacheRecvData.size() < (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length))
		{
			return true;
		}
		std::string aMasking_key;
		aMasking_key.assign(cacheRecvData.c_str() + 1 + nPlayLoadLenByteNum, nMaskingKeyByteNum);
		std::string aPayload_data;
		aPayload_data.assign(cacheRecvData.c_str() + 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum, nPayload_length);
		int nLeftSize = cacheRecvData.size() - (1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length);

		if (nLeftSize > 0)
		{
			std::string leftBytes;
			leftBytes.assign(cacheRecvData.c_str() + 1 + nPlayLoadLenByteNum + nMaskingKeyByteNum + nPayload_length, nLeftSize);
			cacheRecvData = leftBytes;
		}
		else {
			cacheRecvData.clear();
		}
		for (int i = 0; i < nPayload_length; i++)
		{
			aPayload_data[i] = (char)(aPayload_data[i] ^ aMasking_key[i % nMaskingKeyByteNum]);
		}

		if (8 == nOpcode)
		{
			addSendPkg(buildPkg("", nOpcode));// close
			bIsClose = true;
		}
		else if (2 == nOpcode || 1 == nOpcode || 0 == nOpcode || 9 == nOpcode)
		{
			if (9 == nOpcode)//!ping
			{
				addSendPkg(buildPkg("", 0xA));// pong
			}

			if (nFIN == 1)
			{
				if (dataFragmentation.size() == 0)
				{
					addRecvPkg(aPayload_data);
				}
				else
				{
					dataFragmentation += aPayload_data;
					addRecvPkg(dataFragmentation);
					dataFragmentation.clear();
				}
			}
			else
			{
				dataFragmentation += aPayload_data;
			}
		}

		return true;
	}
	std::string buildPkg(const std::string& dataBody, int opcode = 0x01)
	{
		int nBodyLenByteNum = 0;
		if (dataBody.size() >= 65536)
		{
			nBodyLenByteNum = 8;
		}
		else if (dataBody.size() >= 126)
		{
			nBodyLenByteNum = 2;
		}
		std::string ret = "  ";
		ret[0] = 0;
		ret[1] = 0;
		ret[0] |= 0x80;//!_fin
		ret[0] |= (char)opcode;//0x01;//! opcode 1 text 2 binary

		if (nBodyLenByteNum == 0)
		{
			ret[1] = (char)dataBody.size();
			ret += dataBody;
		}
		else if (nBodyLenByteNum == 2)
		{
			ret[1] = 126;

			uint16_t extsize = (uint16_t)dataBody.size();
			int16_t extsizeNet = htons((int16_t)extsize);
			ret.append((const char*)(&extsizeNet), sizeof(extsizeNet));
			ret += dataBody;
		}
		else
		{
			ret[1] = 127;
			//Array.Copy(dataBody, 0, ret, 10, dataBody.Length);

			size_t extsize = dataBody.size();
			int64_t extsizeNet = ws_hton64((int64_t)extsize);//System.Net.IPAddress.HostToNetworkOrder((long)extsize);
			ret.append((const char*)(&extsizeNet), sizeof(extsizeNet));
			ret += dataBody;
		}
		return ret;
	}

	std::string sha1Encode(const std::string& strSrc)
	{
		return sha1(strSrc);
	}


	vector<int> X;//8*64=512，每个下标存放8位
	int W[80];//32位为一组
	int A, B, C, D, E;
	int A1, B1, C1, D1, E1;//缓冲区寄存器,产生最后结果
	int Turn;//加密分组数量
	void printX() {//输出填充后的文本
		for (int i = 0; i < X.size(); i++) {
			printf("%02x", X[i]);
			if ((i + 1) % 4 == 0)
				printf(" ");
			if ((i + 1) % 16 == 0)
				printf("\n");
		}
	}
	int S(unsigned int x, int n) {//循环左移
		return x >> (32 - n) | (x << n);
	}
	void append(string m) {//文本的填充处理
		Turn = (m.size() + 8) / 64 + 1;
		X.resize(Turn * 64);
		int i = 0;
		for (; i < m.size(); i++) {
			X[i] = m[i];
		}
		X[i++] = 0x80;
		while (i < X.size() - 8) {
			X[i] = 0;
			i++;
		}
		long long int a = m.size() * 8;
		for (i = X.size() - 1; i >= X.size() - 8; i--) {
			X[i] = a % 256;
			a /= 256;
		}
	}
	void setW(vector<int> m, int n) {//W数组的生成
		n *= 64;
		for (int i = 0; i < 16; i++) {
			W[i] = (m[n + 4 * i] << 24) + (m[n + 4 * i + 1] << 16)
				+ (m[n + 4 * i + 2] << 8) + m[n + 4 * i + 3];
		}
		for (int i = 16; i < 80; i++) {
			W[i] = S(W[i - 16] ^ W[i - 14] ^ W[i - 8] ^ W[i - 3], 1);
		}
	}
	int ft(int t) {//ft(B,C,D)函数
		if (t < 20)
			return (B & C) | ((~B) & D);
		else if (t < 40)
			return B ^ C ^ D;
		else if (t < 60)
			return (B & C) | (B & D) | (C & D);
		else
			return B ^ C ^ D;
	}
	int Kt(int t) {//常量K
		if (t < 20)
			return 0x5a827999;
		else if (t < 40)
			return 0x6ed9eba1;
		else if (t < 60)
			return 0x8f1bbcdc;
		else
			return 0xca62c1d6;
	}
	string sha1(string text) {
		A1 = A = 0x67452301;
		B1 = B = 0xefcdab89;
		C1 = C = 0x98badcfe;
		D1 = D = 0x10325476;
		E1 = E = 0xc3d2e1f0;
		append(text);
		//printX();
		for (int i = 0; i < Turn; i++) {
			setW(X, i);
			for (int t = 0; t < 80; t++) {
				int temp = E + ft(t) + S(A, 5) + W[t] + Kt(t);
				E = D;
				D = C;
				C = S(B, 30);
				B = A;
				A = temp;
			}
			A1 = A = A + A1;
			B1 = B = B + B1;
			C1 = C = C + C1;
			D1 = D = D + D1;
			E1 = E = E + E1;
		}
		char info[MAX_PATH] = { 0 };
		sprintf(info,"%08x%08x%08x%08x%08x", A1, B1, C1, D1, E1);

		return info;
	}

	string parseUri(const string& request, const string& _url) {
		smatch result;
		if (regex_search(_url.cbegin(), _url.cend(), result, regex(request + "=(.*?)&"))) {
			// 匹配具有多个参数的url

			// *? 重复任意次，但尽可能少重复  
			return result[1];
		}
		else if (regex_search(_url.cbegin(), _url.cend(), result, regex(request +"=(.*)"))) {
			// 匹配只有一个参数的url

			return result[1];
		}
		else {
			// 不含参数或制定参数不存在

			return string();
		}
	}

	std::string base64Encode(const char * input, int length, bool with_new_line)
	{
		char * outPut;
		zoomHmacSHA256 zoomHmacSHA256;
		zoomHmacSHA256.Base64URLEncode(input, outPut, strlen(input));
		return outPut;
	}
	uint32_t computeWebsokcetKeyVal(const std::string& val)
	{
		uint32_t ret = 0;
		uint32_t kongge = 0;
		std::string str_num;

		for (size_t i = 0; i < val.length(); ++i)
		{
			if (val[i] == ' ')
			{
				++kongge;
			}
			else if (val[i] >= '0' && val[i] <= '9')
			{
				str_num += val[i];
			}
		}
		if (kongge > 0)
		{
			ret = uint32_t(::atoi(str_num.c_str()) / kongge);
		}
		ret = htonl(ret);

		return ret;
	}

	std::string computeMd5(const char* s, int n)
	{
		std::string ret;
		MD5 md5;
		md5.update(s);

		return md5.toString();
	}
private:
	int   statusWebSocketConnection;
	bool  bIsClose;
	std::string cacheRecvData;
	std::map<std::string, std::string> dictParams;
	std::vector<std::string> listSendPkg;//!需要发送的数据
	std::vector<std::string> listRecvPkg;//!已经接收的
	std::string dataFragmentation;//!缓存分片数据
};


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

std::string ws2s(const std::wstring& wstr)
{
	std::string str;
	int nLen = (int)wstr.length();
	str.resize(nLen, ' ');
	int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);
	if (nResult == 0)
	{
		return "";
	}
	return str;
}

std::wstring s2ws(const std::string& str)
{
	int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), 0, 0);
	if (nSize <= 0) return NULL;
	WCHAR *pwszDst = new WCHAR[nSize + 1];
	if (NULL == pwszDst)
		return NULL;
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), pwszDst, nSize);
	pwszDst[nSize] = 0;
	if (pwszDst[0] == 0xFEFF) // skip Oxfeff
		for (int i = 0; i < nSize; i++)
			pwszDst[i] = pwszDst[i + 1];
	wstring wcharString(pwszDst);
	delete pwszDst;
	return wcharString;
}

DWORD WINAPI createSokectListener(LPVOID lpParam) {


	CSDKDemoApp  *app_ = (CSDKDemoApp*)lpParam;

	WSADATA wsaData;
	SOCKET sockServer;
	SOCKADDR_IN addrServer;
	SOCKET sockClient;
	SOCKADDR_IN addrClient;
	// 初始化socket
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)!= 0)
	{
		OutputDebugString(_T("客户端WSAStartup失败"));
		DWORD dwSokectServerThreadID;

		HANDLE ghSocketServerThread = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			createSokectListener,        // name of the thread function
			app_,              // no thread parameters
			0,                 // default startup flags
			&dwSokectServerThreadID);
			return -1001;
	}
	
	sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sockServer)
	{
		OutputDebugString(_T("服务端socket失败"));
		WSACleanup();
		DWORD dwSokectServerThreadID;

		HANDLE ghSocketServerThread = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			createSokectListener,        // name of the thread function
			app_,              // no thread parameters
			0,                 // default startup flags
			&dwSokectServerThreadID);
		return -1002;
	}

	int nNetTimeout = 3000;//10秒，
	//设置发送超时
	setsockopt(sockServer, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
	//设置接收超时
	setsockopt(sockServer, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));
	
	setsockopt(sockServer, SOL_SOCKET, SO_CONNECT_TIME, (char *)&nNetTimeout, sizeof(int));
	// 设置地址和端口号可以重复使用
	int optval = 1;
	int optlen = sizeof(optval);
	setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, optlen);

	addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//INADDR_ANY表示任何IP
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(9527);//绑定端口9527
	int retval = ::bind(sockServer, (SOCKADDR*)&addrServer, sizeof(addrServer));
	if (SOCKET_ERROR == retval)
	{
		OutputDebugString(_T("服务端bind失败"));
		closesocket(sockServer);
		WSACleanup();
		DWORD dwSokectServerThreadID;

		HANDLE ghSocketServerThread = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			createSokectListener,        // name of the thread function
			app_,              // no thread parameters
			0,                 // default startup flags
			&dwSokectServerThreadID);
		return -1003;
	}

	//Listen监听端
	retval = listen(sockServer, 1);//5为等待连接数目
	if (SOCKET_ERROR == retval)
	{
		OutputDebugString(_T("服务端listen失败"));
		closesocket(sockServer);
		WSACleanup();
		DWORD dwSokectServerThreadID;

		HANDLE ghSocketServerThread = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			createSokectListener,        // name of the thread function
			app_,              // no thread parameters
			0,                 // default startup flags
			&dwSokectServerThreadID);
		return -1004;
	}
	
	int len = sizeof(addrClient);
	int const CLIENT_MSG_SIZE = 4096;//接收缓冲区长度


	char sendBuf[CLIENT_MSG_SIZE];//发送至客户端的字符串
	char recvBuf[CLIENT_MSG_SIZE];//接受客户端返回的字符串
	

	OutputDebugString(_T("\n"));
	OutputDebugString(_T("服务器已启动:监听中..."));
	OutputDebugString(_T("\n"));

	//会阻塞进程，直到有客户端连接上来为止
	sockClient = accept(sockServer, (SOCKADDR*)&addrClient, &len);
	OutputDebugString(_T("客户端accept ready ok\n"));

	if (sockClient == INVALID_SOCKET) {//连接失败
		OutputDebugString(_T("客户端accept失败\n"));
		closesocket(sockServer);
		WSACleanup();
		CSDKDemoApp  *app_ = (CSDKDemoApp*)lpParam;
		DWORD dwSokectServerThreadID;

		HANDLE ghSocketServerThread = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			createSokectListener,        // name of the thread function
			app_,              // no thread parameters
			0,                 // default startup flags
			&dwSokectServerThreadID);
		
		sprintf(recvBuf,"GetLastError failed:%d\n", GetLastError());
		OutputDebugString(s2ws(string(recvBuf)).c_str());
		sprintf(sendBuf, "WSAStartup failed:%d\n", WSAGetLastError());
		OutputDebugString(s2ws(string(sendBuf)).c_str());
		

		return -7;
	}

	//一直不断地接收消息，直到客户端选择退出
	while (true) {
		memset(recvBuf, 0, CLIENT_MSG_SIZE);
		OutputDebugString(_T("客户端等待接收\n"));
		int recvCode = recv(sockClient, recvBuf, CLIENT_MSG_SIZE, 0);
		


		if (recvCode == SOCKET_ERROR) {//如果接收消息出错
			OutputDebugString(_T("客户端中断会话异常， 失败\n"));
			//continue;
			closesocket(sockServer);
			closesocket(sockClient);
			WSACleanup();
			CSDKDemoApp  *app_ = (CSDKDemoApp*)lpParam;
			DWORD dwSokectServerThreadID;

			HANDLE ghSocketServerThread = CreateThread(
				NULL,              // default security
				0,                 // default stack size
				createSokectListener,        // name of the thread function
				app_,              // no thread parameters
				0,                 // default startup flags
				&dwSokectServerThreadID);

			sprintf(recvBuf, "GetLastError failed:%d\n", GetLastError());
			OutputDebugString(s2ws(string(recvBuf)).c_str());
			sprintf(sendBuf, "WSAStartup failed:%d\n", WSAGetLastError());
			OutputDebugString(s2ws(string(sendBuf)).c_str());


			return -99;
		}

		//否则，输出消息
		OutputDebugString(_T("服务器已启动:\n接收数据..."));
		//OutputDebugString(s2ws(recvBuf).c_str());
		
		OutputDebugString(_T("\n"));
		if (true) {//模拟http 请求
			//char* s_header;
			strcpy(sendBuf, "HTTP/1.1 200 Ok\r\nConnection: close\r\n");//必须以HTTP协议版本和状态码开头，其他的field顺序不重要
			strcat(sendBuf, "Content-Type:application/json; charset=utf-8\r\n");//我打算发个html文件给客户端，所以Content-Type:text/html
			strcat(sendBuf, "\r\n");

			WSProtocol  m_oWSProtocol = WSProtocol();
			std::vector<std::string> strLines;
			m_oWSProtocol.strSplit(recvBuf, strLines, "\r\n");
			for (size_t i = 0; i < strLines.size(); ++i)
			{

				const std::string& line = strLines[i];
				
				std::vector<std::string> strParams;
				m_oWSProtocol.strSplit(line, strParams, ":");
				if (strParams.size() == 2)
				{
					OutputDebugString(_T("\n"));
					OutputDebugString(_T("key:"));
					OutputDebugString(s2ws(strParams[0]).c_str());
					OutputDebugString(_T("\r"));
					OutputDebugString(_T("value:"));
					OutputDebugString(s2ws(strParams[1]).c_str());
				}
				else if (strParams.size() == 1 && strParams[0].find("GET") != std::string::npos)
				{
					OutputDebugString(_T("\n"));
					string queryParams = strParams[0];
					
					if (queryParams.find("getCurrentMeetingInfo") != std::string::npos)
					{
						//parse params;
						string id = m_oWSProtocol.parseUri("id", queryParams);
						
						if (app_->m_sdk_login_ui_mgr) {
							std::string MeetingNumber = app_->m_sdk_login_ui_mgr->getMeetingId();
							
								if (MeetingNumber != id) {
									string result = "{\"statusCode\":200,\"data\":{\"id\":\"" + MeetingNumber + "\"}}";
									strcat(sendBuf, result.c_str());//s_buffer是html文件读取后所在的buffer
									HWND m_hWnd(NULL);
									m_hWnd = app_->m_sdk_login_ui_mgr->GetHWND();
									AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), TRUE);
									SetForegroundWindow(m_hWnd);
									SetFocus(m_hWnd);
									AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), FALSE);
									OutputDebugString(_T("\n【打印id】:"));
									OutputDebugString(s2ws(id).c_str());
									OutputDebugString(_T("\n"));
								}

								
							
							
						}
						
					}
					OutputDebugString(s2ws(queryParams).c_str());
				}
			}

			
			string sendStr = sendBuf;
			send(sockClient, sendStr.c_str(), sendStr.length(), MSG_DONTROUTE);
			OutputDebugString(_T("服务器已启动:\n回复数据..."));
			OutputDebugString(s2ws(sendBuf).c_str());
			memset(sendBuf, 0, CLIENT_MSG_SIZE);//每次回复之后，清空发送消息数组
			
			closesocket(sockServer);
			closesocket(sockClient);
			WSACleanup();
			DWORD dwSokectServerThreadID;

			HANDLE ghSocketServerThread = CreateThread(
				NULL,              // default security
				0,                 // default stack size
				createSokectListener,        // name of the thread function
				app_,              // no thread parameters
				0,                 // default startup flags
				&dwSokectServerThreadID);
			return 0;
		}
		WSProtocol  m_oWSProtocol = WSProtocol();
		if (m_oWSProtocol.handleRecv(recvBuf, len))
		{
			OutputDebugString(_T("准备回复数据...\n"));
			const vector<string>& waitToSend = m_oWSProtocol.getSendPkg();
			for (size_t i = 0; i < waitToSend.size(); ++i)
			{
				
				OutputDebugString(_T("服务器已启动:\n回复数据..."));
				OutputDebugString(s2ws(waitToSend[i]).c_str());
				OutputDebugString(_T("\n"));

				sprintf(sendBuf, waitToSend[i].c_str());
				send(sockClient, sendBuf, CLIENT_MSG_SIZE, 0);


				memset(sendBuf, 0, CLIENT_MSG_SIZE);//每次回复之后，清空发送消息数组


			}
			OutputDebugString(_T("准备回复数据End...\n"));

			m_oWSProtocol.clearSendPkg();

			const vector<string>& recvPkg = m_oWSProtocol.getRecvPkg();
			for (size_t i = 0; i < recvPkg.size(); ++i)
			{
				const string& eachRecvPkg = recvPkg[i];
				uint16_t nCmd = 0;
				OutputDebugString(_T("接收数据..\n"));
				OutputDebugString(s2ws(eachRecvPkg).c_str());

			}
			m_oWSProtocol.clearRecvPkg();
			if (m_oWSProtocol.isClose())
			{
				OutputDebugString(_T("准备关闭数据..."));
				//关闭socket
				closesocket(sockClient);
				closesocket(sockServer);
				WSACleanup();
				return 1;
			}
		}
		else {
			OutputDebugString(_T("handleRecv not match End...\n"));
		}
		
		
	}

	//关闭socket
	closesocket(sockServer);
	closesocket(sockClient);
	WSACleanup();

	
	return 1;
}


DWORD WINAPI delayTime(LPVOID lpParam) {

		CSDKDemoApp  *app_ = (CSDKDemoApp*)lpParam;
	while (true)
	{
		Sleep(1000);
		if (app_->m_sdk_login_ui_mgr) {
			if (app_->m_sdk_login_ui_mgr->isInMeetingNow()) {
				break;
			}
		
		}
	}

	DWORD dwSokectServerThreadID;

	HANDLE ghSocketServerThread = CreateThread(
		NULL,              // default security
		0,                 // default stack size
		createSokectListener,        // name of the thread function
		app_,              // no thread parameters
		0,                 // default startup flags
		&dwSokectServerThreadID);
	return 1;
}

void SokectListener(CSDKDemoApp &app_) {
	DWORD dwSokectServerThreadID;

	HANDLE ghSocketServerThread = CreateThread(
		NULL,              // default security
		0,                 // default stack size
		delayTime,        // name of the thread function
		&app_,              // no thread parameters
		0,                 // default startup flags
		&dwSokectServerThreadID);
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

	SokectListener(app_);

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