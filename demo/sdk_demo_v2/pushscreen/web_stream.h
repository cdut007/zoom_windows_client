/// by fanxiushu 2017-05-14

#pragma once

struct dp_frame_t
{
	int        cx;          ///��Ļ���
	int        cy;          ///��Ļ�߶�
	int        line_bytes;  ///ÿ��ɨ���е�ʵ�����ݳ���
	int        line_stride; ///ÿ��ɨ���е�4�ֽڶ�������ݳ���
	int        bitcount;    ///8.16.24.32 λ���, 8λ��256��ɫ�壻 16λ��555��ʽ��ͼ��

	char*      buffer;      ///��Ļ����
};


class web_stream
{
protected:
	bool quit;
	int listenfd;
	vector<int> socks;
	CRITICAL_SECTION cs;
	static DWORD CALLBACK accept_thread(void* _p);
	static DWORD CALLBACK client_thread(void* _p);
	///
	int jpeg_quality;
public:
	web_stream();
	~web_stream();
	
	////
	int start(const char* strip, int port);
	void set_jpeg_quality(int quality) { jpeg_quality = quality; }
	void frame(struct dp_frame_t* frame);
};

