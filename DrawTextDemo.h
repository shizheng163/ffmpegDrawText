#pragma once
#include <string>
#include "ffmpeg_required.h"
#include <memory>

extern "C"
{
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/avfiltergraph.h"
};
typedef int(*Callback_Interrupt)(void *) ;

struct Point
{
	Point()
	{
		x = y = 0;
	}
	Point(int x, int y)
	{
		this->x = x;
		this->y = y;
	}
	int x, y;
};
class VideoDrawText
{
public:
	VideoDrawText();
	VideoDrawText(std::string inputUrl, std::string outputUrl, std::string outFormat = "mpegts");
	virtual ~VideoDrawText();

	int OpenInput();
	int OpenOutput();
	void CloseInput();
	void CloseOutput();
	std::shared_ptr <AVPacket> ReadPacket();
	int WritePacket(std::shared_ptr<AVPacket> packet, bool isRescaleTs = true);
	//Encode-Decode
	int InitEncodeContext(int width, int height);
	int InitDecodeContext();
	int InitFilter();

	//Set Class Attrs
	void SetOutputFormat(std::string format) { this->m_OutputFormat = format; }
	void SetInputUrl(std::string url) { this->m_InputUrl = url; }
	void SetOutputUrl(std::string url) { this->m_outputUrl = url; }
	void SetCallback_OpenInput(Callback_Interrupt callback) { m_CB_OpenInput = callback; }
	//Set This Attr Before Called InitFilter()
	void SetDrawTextPoint(Point p) { m_DrawTextPoint = p; this->RefreshDrawTextString(); }
	void SetDrawTextFontSize(int fontSize) { m_DrawTextFontSize = fontSize; this->RefreshDrawTextString(); }
	void SetDrawTextColor(std::string colorCode) { m_DrawTextColor = colorCode; this->RefreshDrawTextString(); }
	void SetDrawText(std::string text) { m_DrawText = text; this->RefreshDrawTextString(); }
	//Get Class Attrs
	int GetInputStreamNb() const;
	int GetStreamIndex(AVMediaType mediaType);
	AVStream * GetStream(AVMediaType mediaType);
	AVFormatContext* GetOutCtx() const { return m_OutputContext; }
	AVCodecContext * GetEncodeCtx() const { return m_EncodeCodecCtx; }
	AVFilterContext * GetSrcFilterCtx() const { return m_bufferSrcFilterCtx; }
	AVFilterContext * GetSinkFilterCtx() const { return m_bufferSinkFilterCtx; }

	//static 
	static void Init();
	static bool Decode(AVStream * inputStream, AVPacket * packet, AVFrame *frame);
	static std::shared_ptr<AVPacket> encode(AVCodecContext *codecCtx, AVFrame * frame);
private:
	void RefreshDrawTextString();

	AVFormatContext *	m_InputContext, *m_OutputContext;
	std::string			m_InputUrl, m_outputUrl;
	std::string			m_OutputFormat;
	int64_t				m_LastReadPacketTime;
	Callback_Interrupt  m_CB_OpenInput;

	//DrawText About
	Point				m_DrawTextPoint;
	int					m_DrawTextFontSize;
	std::string			m_DrawTextColor;
	std::string			m_DrawText;
	char				m_DrawTextFilterCreateString[512];

	//AVFilter About
	AVFilterGraph *		m_FilterGraph;
	AVFilterContext *	m_bufferSrcFilterCtx, *m_bufferSinkFilterCtx;

	//Encode-Decode
	AVCodecContext *	m_EncodeCodecCtx; 
	AVCodecContext *	m_DecodeCodecCtx; 

	//static 
	static int DefaultAVInterruptCallBack(void *ctx);
};

