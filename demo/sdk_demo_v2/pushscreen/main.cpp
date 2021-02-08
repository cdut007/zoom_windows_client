#define    WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include "rgb2yuv.h"
#include "SDLMaster.h"
#include "MonitorMaster.h"
#include "ffmpegEncoder.h"
#include "libx264Master.h"

using namespace std;

#pragma comment(lib ,"ws2_32.lib")

extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }
extern "C" _CRT_STDIO_INLINE int __CRTDECL fprintf(
	_Inout_                       FILE*       const _Stream,
	_In_z_ _Printf_format_string_ char const* const _Format,
	...);

struct cap_screen_t
{
	HDC memdc;
	HBITMAP hbmp;
	unsigned char* buffer;
	int            length;

	int width;
	int height;
	int bitcount;
	int left, top;
};

extern MonitorMaster::VEC_MONITOR_INFO vecMonitorListInfo;

int init_cap_screen(struct cap_screen_t* sc, int indexOfMonitor = 0)
{
	MonitorMaster::GetAllMonitorInfo();
	if (indexOfMonitor >= vecMonitorListInfo.size())
		indexOfMonitor = 0;
	DEVMODE devmode;
	BOOL bRet;
	BITMAPINFOHEADER bi;
	sc->width = vecMonitorListInfo[indexOfMonitor]->nWidth;
	sc->height = vecMonitorListInfo[indexOfMonitor]->nHeight;
	sc->bitcount = vecMonitorListInfo[indexOfMonitor]->nBits;
	sc->left = vecMonitorListInfo[indexOfMonitor]->area.left;
	sc->top = vecMonitorListInfo[indexOfMonitor]->area.top;
	memset(&bi, 0, sizeof(bi));
	bi.biSize = sizeof(bi);
	bi.biWidth = sc->width;
	bi.biHeight = -sc->height; //���ϳ���ɨ��
	bi.biPlanes = 1;
	bi.biBitCount = sc->bitcount; //RGB
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	HDC hdc = GetDC(NULL); //��ĻDC
	sc->memdc = CreateCompatibleDC(hdc);
	sc->buffer = NULL;
	sc->hbmp = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&sc->buffer, NULL, 0);
	ReleaseDC(NULL, hdc);
	SelectObject(sc->memdc, sc->hbmp); ///
	sc->length = sc->height* (((sc->width*sc->bitcount / 8) + 3) / 4 * 4);
	return 0;
}

HCURSOR FetchCursorHandle()
{
	CURSORINFO hCur;
	ZeroMemory(&hCur, sizeof(hCur));
	hCur.cbSize = sizeof(hCur);
	GetCursorInfo(&hCur);
	return hCur.hCursor;
}

void AddCursor(HDC hMemDC, POINT origin)
{

	POINT xPoint;
	GetCursorPos(&xPoint);
	xPoint.x -= origin.x;
	xPoint.y -= origin.y;
	if (xPoint.x < 0 || xPoint.y < 0)
		return;
	HCURSOR hcur = FetchCursorHandle();
	ICONINFO iconinfo;
	BOOL ret;
	ret = GetIconInfo(hcur, &iconinfo);
	if (ret)
	{
		xPoint.x -= iconinfo.xHotspot;
		xPoint.y -= iconinfo.yHotspot;
		if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);
		if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
	}
	DrawIcon(hMemDC, xPoint.x, xPoint.y, hcur);
}

int blt_cap_screen(struct cap_screen_t* sc)
{
	HDC hdc = GetDC(NULL);
	BitBlt(sc->memdc, 0, 0, sc->width, sc->height, hdc, sc->left, sc->top, SRCCOPY); // ����
	AddCursor(sc->memdc, POINT{ sc->left, sc->top }); // ��������ȥ
	ReleaseDC(NULL, hdc);
	return 0;
}
#include "web_stream.h"



int main(int argc, char* argv[])
{
	struct cap_screen_t sc;
	BYTE* out;
	AVFrame* frame;
	web_stream* web = new web_stream;
	time_t start, end;
	WSADATA d;

	ffmpeg_init();
	init_cap_screen(&sc, 1);  // 1����ڶ�����ʾ����0�����һ����ʾ�����������ƣ������ʾ������������Ϊ0
	out = (BYTE*)malloc(sc.length);

	// SDLMaster::init(sc.width, sc.height); // SDL���������������ڲ��Ų���
	YUVencoder enc(sc.width, sc.height);

	// ����Ϊ3����ͬ�ı���������ѡһ������
	x264Encoder ffmpeg264(sc.width, sc.height,  "save.h264"); // save.h264Ϊ����¼���ļ� ����ʹ��vlc����������
	//mpeg1Encoder ffmpegmpeg1(sc.width, sc.height, "save.mpg");
	//libx264Master libx264(sc.width, sc.height, "save2.h264");
	
	WSAStartup(0x0202, &d);
	web->start("0.0.0.0", 8000); // 8000�˿�����
	time(&start);


	//����
	//nginx-rtmp ֱ��������rtmp����URL
	char *outUrl = "rtmp://39.108.165.171:1935/live/room";//rtmp://127.0.0.1:1935/live/room

	//ע�����еı������
	avcodec_register_all();

	//ע�����еķ�װ��
	av_register_all();

	//ע����������Э��
	avformat_network_init();

	//rtmp flv ��װ��
	AVFormatContext *ic = NULL;

	AVStream *vs = NULL;

	try
	{

		int ret = 0;



		///5 �����װ������Ƶ������
		//a ���������װ��������
		ret = avformat_alloc_output_context2(&ic, 0, "flv", outUrl);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//b �����Ƶ��   ����Ƶ����Ӧ�Ľṹ�壬��������Ƶ����롣
		vs = avformat_new_stream(ic, NULL);
		if (!vs)
		{
			throw exception("avformat_new_stream failed");
		}
		//���ӱ�־�����һ��Ҫ����
		vs->codec->codec_tag = 0;
		//�ӱ��������Ʋ���
		avcodec_copy_context(vs->codec, ffmpeg264.pCodecCtx);
		av_dump_format(ic, 0, outUrl, 1);


		///��rtmp ���������IO  AVIOContext�����������Ӧ�Ľṹ�壬���������������д�ļ���RTMPЭ��ȣ���
		ret = avio_open(&ic->pb, outUrl, AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		//д���װͷ
		ret = avformat_write_header(ic, NULL);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}



	}
	catch (exception &ex)
	{

		cerr << ex.what() << endl;
	}

	for (int i = 0; ; i++)
	{
		blt_cap_screen(&sc);
		char* buf = NULL;
		dp_frame_t a;
		a.bitcount = 32;
		a.buffer = (char*)sc.buffer;
		a.line_bytes = sc.width * sc.bitcount / 8;
		a.line_stride = (a.line_bytes + 3) / 4 * 4;
		a.cx = sc.width;
		a.cy = sc.height;
		web->frame(&a);  // ����֡

		frame = enc.encode(sc.buffer, sc.length, sc.width, sc.height, out, sc.length);
		
		if (frame == NULL)
		{
			printf("Encoder error!!\n"); Sleep(1000); continue;
		}
		ffmpeg264.encodeRTMP(frame,vs,ic);
		// һ����3�ֱ��뷽ʽ����ѡһ�־���
		//ffmpeg264.encode(frame);
		//frame->dat
		// ffmpegmpeg1.encode(frame); 
		// libx264.encode(frame);

		// SDL������������Ƶ��ȡ��ע�ͼ��ɲ���
		// SDLMaster::updateScreen(frame); 

		

		free(buf);
	}
	time(&end);
	printf("%f\n", difftime(end, start));
	//ffmpeg264.flush(frame, &sender);
	SDL_Quit();
	return 0;
}
