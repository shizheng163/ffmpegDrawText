#include "DrawTextDemo.h"
#include <iostream>
using namespace std;

VideoDrawText::VideoDrawText():
	m_InputContext(NULL),
	m_OutputContext(NULL),
	m_CB_OpenInput(NULL),
	m_OutputFormat("mpegts"),
	m_LastReadPacketTime(0),
	m_DecodeCodecCtx(NULL),
	m_EncodeCodecCtx(NULL),
	m_FilterGraph(NULL),
	m_bufferSrcFilterCtx(NULL),
	m_bufferSinkFilterCtx(NULL)
{
	m_DrawText = "DrawText";
	m_DrawTextColor = "white";
	m_DrawTextFontSize = 30;
	this->RefreshDrawTextString();
}

VideoDrawText::VideoDrawText(std::string inputUrl, std::string outputUrl, std::string outFormat):
	m_InputContext(NULL),
	m_OutputContext(NULL),
	m_CB_OpenInput(NULL),
	m_OutputFormat(outFormat),
	m_LastReadPacketTime(0),
	m_InputUrl(inputUrl),
	m_outputUrl(outputUrl),
	m_DecodeCodecCtx(NULL),
	m_EncodeCodecCtx(NULL),
	m_FilterGraph(NULL),
	m_bufferSrcFilterCtx(NULL),
	m_bufferSinkFilterCtx(NULL)
{
	m_DrawText = "DrawText";
	m_DrawTextColor = "white";
	m_DrawTextFontSize = 30;
	this->RefreshDrawTextString();
}


VideoDrawText::~VideoDrawText()
{
	//Use Input Stream Decode
	//if (m_DecodeCodecCtx)
	//	avcodec_close(m_DecodeCodecCtx);
	if (m_EncodeCodecCtx)
		avcodec_close(m_EncodeCodecCtx);
	if (m_FilterGraph)
		avfilter_graph_free(&m_FilterGraph);
	if (m_InputContext)
		CloseInput();
	if (m_OutputContext)
		CloseOutput();
}

int VideoDrawText::OpenInput()
{
	if (m_InputContext)
		CloseInput();
	m_InputContext = avformat_alloc_context();
	m_LastReadPacketTime = av_gettime();

	m_InputContext->interrupt_callback.callback = m_CB_OpenInput == NULL ? DefaultAVInterruptCallBack : m_CB_OpenInput;
	m_InputContext->interrupt_callback.opaque = this;
	AVDictionary *format_opts = NULL;
	int ret = avformat_open_input(&m_InputContext, m_InputUrl.c_str(), NULL, &format_opts);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Open Input Url Failed : %s \n", m_InputUrl.c_str());
		return ret;
	}

	ret = avformat_find_stream_info(m_InputContext, NULL);
	if (ret < 0)
		av_log(NULL, AV_LOG_ERROR, "Find Input File Stream Info Failed:%s \n", m_InputUrl.c_str());
	//Print Stream Info About InputStream
	av_dump_format(m_InputContext, 0, m_InputUrl.c_str(), 0);
	printf("Open Input Success:%s \n", m_InputUrl.c_str());
	return ret;
}

int VideoDrawText::OpenOutput()
{
	if (m_OutputContext)
		CloseOutput();
	int ret = avformat_alloc_output_context2(&m_OutputContext, NULL, m_OutputFormat.c_str(), m_outputUrl.c_str());
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open output context failed: %s format :%s\n, ", m_outputUrl.c_str(), m_OutputFormat.c_str());
		return ret;
	}
	ret = avio_open2(&m_OutputContext->pb, m_outputUrl.c_str(), AVIO_FLAG_WRITE, NULL, NULL);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "open avio failed!\n");
		return ret;
	}

	for (int i = 0; i < m_InputContext->nb_streams; i++)
	{
		if (m_InputContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVStream * stream = avformat_new_stream(m_OutputContext, m_EncodeCodecCtx->codec);
			stream->codec = m_EncodeCodecCtx;
		}
		else
		{
			AVStream * stream = avformat_new_stream(m_OutputContext, m_InputContext->streams[i]->codec->codec);
			avcodec_copy_context(stream->codec, m_InputContext->streams[i]->codec);
		}
	}

	av_dump_format(m_OutputContext, 0, m_outputUrl.c_str(), 1);
	ret = avformat_write_header(m_OutputContext, NULL);

	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "format write header Failed!\n");
		goto Error;
	}

	printf("Open output File Success %s\n", m_outputUrl.c_str());
	return ret;
Error:
	CloseOutput();
	return ret;
}

void VideoDrawText::CloseInput()
{
	if (m_InputContext != NULL)
		avformat_close_input(&m_InputContext);
	m_InputContext = NULL;
}

void VideoDrawText::CloseOutput()
{
	if (m_OutputContext != NULL)
	{
		for (int i = 0; i < m_OutputContext->nb_streams; i++)
		{
			AVCodecContext * codecContext = m_OutputContext->streams[i]->codec;
			avcodec_close(codecContext);
		}
		avformat_close_input(&m_OutputContext);
	}
	m_OutputContext = NULL;
}

int VideoDrawText::GetInputStreamNb() const
{
	if (m_InputContext == NULL)
		return 0;
	else
		return m_InputContext->nb_streams;
}

int VideoDrawText::GetStreamIndex(AVMediaType mediaType)
{
	if (m_InputContext == NULL)
		return -1;
	for (int i = 0; i < m_InputContext->nb_streams; i++)
	{
		if (m_InputContext->streams[i]->codec->codec_type == mediaType)
			return i;
	}
	return -1;
}

AVStream * VideoDrawText::GetStream(AVMediaType mediaType)
{
	if (m_InputContext == NULL)
		return NULL;
	for (int i = 0; i < m_InputContext->nb_streams; i++)
	{
		if (m_InputContext->streams[i]->codec->codec_type == mediaType)
			return m_InputContext->streams[i];
	}
	return NULL;
}


shared_ptr<AVPacket> VideoDrawText::ReadPacket()
{
	auto ReleaseCallBack = [&](AVPacket *p)
	{
		av_packet_free(&p);
		av_freep(&p);
	};
	shared_ptr<AVPacket> packet(static_cast<AVPacket *>(av_malloc(sizeof(AVPacket))), ReleaseCallBack);
	av_init_packet(packet.get());
	int ret = av_read_frame(m_InputContext, packet.get());
	if (ret >= 0)
		m_LastReadPacketTime = av_gettime();
	return ret >= 0 ? packet : NULL;
}

int VideoDrawText::WritePacket(shared_ptr<AVPacket> packet, bool isRescaleTs)
{
	auto inputStream = m_InputContext->streams[packet->stream_index];
	auto outputStream = m_OutputContext->streams[packet->stream_index];
	if (isRescaleTs)
	{
		av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
	}
	if (m_OutputContext->nb_streams > 1)
		return av_interleaved_write_frame(m_OutputContext, packet.get());
	else
		return av_write_frame(m_OutputContext, packet.get());
}

bool VideoDrawText::Decode(AVStream * inputStream, AVPacket * packet, AVFrame * frame)
{
	int gotPictureRes;
	int ret = avcodec_decode_video2(inputStream->codec, frame, &gotPictureRes, packet);
	return ret >= 0 && gotPictureRes != 0;
}

std::shared_ptr<AVPacket> VideoDrawText::encode(AVCodecContext * codecCtx, AVFrame * frame)
{
	auto ReleaseCallBack = [&](AVPacket *p)
	{
		av_packet_free(&p);
		av_freep(&p);
	};
	int gotOutputRes = 0; //is Get ouput AVPacket
	shared_ptr<AVPacket> pkt(static_cast<AVPacket *>(av_malloc(sizeof(AVPacket))), ReleaseCallBack);
	av_init_packet(pkt.get());
	pkt->data = NULL;
	pkt->size = 0;
	int ret = avcodec_encode_video2(codecCtx, pkt.get(), frame, &gotOutputRes);
	if (ret >= 0 && gotOutputRes != 0)
		return pkt;
	return NULL;
}

void VideoDrawText::RefreshDrawTextString()
{
	memset(m_DrawTextFilterCreateString, 0, sizeof(char) * 512);
	sprintf(m_DrawTextFilterCreateString, "drawtext=fontfile=D\\\\:FreeSerifBoldItalic.ttf:fontsize=%d:fontcolor=%s:text=%s:x=%d:y=%d",
		m_DrawTextFontSize, m_DrawTextColor.c_str(), m_DrawText.c_str(), m_DrawTextPoint.x, m_DrawTextPoint.y);
}

int VideoDrawText::InitDecodeContext()
{
	AVStream * videoStream = this->GetStream(AVMEDIA_TYPE_VIDEO);
	if (videoStream == NULL)
	{
		printf("Dont Find Video Stream!\n");
		return -1;
	}
	auto codec = avcodec_find_decoder(videoStream->codec->codec_id);
	if (codec == NULL)
		return -1; 
	if (m_DecodeCodecCtx)
		avcodec_close(m_DecodeCodecCtx);
	m_DecodeCodecCtx = videoStream->codec;
	if (m_DecodeCodecCtx == NULL)
	{
		fprintf(stderr, "Could not alloccate video codec context\n");
		exit(1);
	}
	return avcodec_open2(m_DecodeCodecCtx, codec, NULL);
}

int VideoDrawText::InitFilter()
{
	char args[512];
	int ret = 0;
	AVFilter * bufferSrc = avfilter_get_by_name("buffer");
	AVFilter * bufferSink = avfilter_get_by_name("buffersink");

	//设置字体格式大小
	AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P };

	m_FilterGraph = avfilter_graph_alloc();
	if (!m_FilterGraph)
		return AVERROR(ENOMEM);

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		m_EncodeCodecCtx->width, m_EncodeCodecCtx->height, m_EncodeCodecCtx->pix_fmt,
		m_EncodeCodecCtx->time_base.num, m_EncodeCodecCtx->time_base.den,
		m_EncodeCodecCtx->sample_aspect_ratio.num, m_EncodeCodecCtx->sample_aspect_ratio.den);

	//创建输入过滤器上下文
	ret = avfilter_graph_create_filter(&m_bufferSrcFilterCtx, bufferSrc, "in", args, NULL, m_FilterGraph);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		return ret;
	}

	AVBufferSinkParams * bufferSinkParams = av_buffersink_params_alloc();
	bufferSinkParams->pixel_fmts = pix_fmts;
	ret = avfilter_graph_create_filter(&m_bufferSinkFilterCtx, bufferSink, "out", NULL, bufferSinkParams, m_FilterGraph);
	av_free(bufferSinkParams);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		return ret;
	}

	AVFilterInOut * inputs = avfilter_inout_alloc();
	AVFilterInOut * outputs = avfilter_inout_alloc();
	/* Endpoints for the filter graph. */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = m_bufferSrcFilterCtx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = m_bufferSinkFilterCtx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	if ((ret = avfilter_graph_parse_ptr(m_FilterGraph, m_DrawTextFilterCreateString, &inputs, &outputs, NULL)) < 0)
	{
		avfilter_inout_free(&inputs);
		avfilter_inout_free(&outputs);
		return ret;
	}

	if ((ret = avfilter_graph_config(m_FilterGraph, NULL)) < 0)
		return ret;

	return 0;
}

int VideoDrawText::InitEncodeContext(int width, int height)
{
	AVStream * videoStream = this->GetStream(AVMEDIA_TYPE_VIDEO);
	if (videoStream == NULL)
	{
		printf("Dont Find Video Stream!\n");
		return -1;
	}
	AVCodec * pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (pH264Codec == NULL)
	{
		printf("Don't Find H264 Encode Codec\n");
		return -1;
	}
	if (m_EncodeCodecCtx)
		avcodec_close(m_EncodeCodecCtx);
	m_EncodeCodecCtx = avcodec_alloc_context3(pH264Codec);
	m_EncodeCodecCtx->gop_size = 30; //两个关键帧之间的距离，帧数。数值大节省带宽，但是容易导致质量下降，某一帧废弃，则gop_size的帧会全部废弃，无法使用
									 //实时流一般没有B帧
	m_EncodeCodecCtx->has_b_frames = 0;
	m_EncodeCodecCtx->max_b_frames = 0;
	m_EncodeCodecCtx->codec_id = pH264Codec->id;
	m_EncodeCodecCtx->time_base = videoStream->time_base;
	m_EncodeCodecCtx->pix_fmt = *(pH264Codec->pix_fmts);
	m_EncodeCodecCtx->width = width;
	m_EncodeCodecCtx->height = height;
	m_EncodeCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	return avcodec_open2(m_EncodeCodecCtx, pH264Codec, NULL);
}

void VideoDrawText::Init()
{
	av_register_all();
	avformat_network_init();
	avfilter_register_all();
	av_log_set_level(AV_LOG_DEBUG);
	//av_log_set_level(AV_LOG_ERROR);
}

int VideoDrawText::DefaultAVInterruptCallBack(void * ctx)
{
	VideoDrawText * pCustomBase = static_cast<VideoDrawText *>(ctx);
	int timeout = 3;
	if (av_gettime() - pCustomBase->m_LastReadPacketTime > timeout * 1000 * 1000)
	{
		return -1;
	}
	return 0;
}
