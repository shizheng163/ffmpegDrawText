#include <iostream>
#include "DrawTextDemo.h"
#include <time.h>
using namespace std;
string GetCurrentTime()
{
	char timePtr[64] = { 0 };
	time_t t = time(0);
	strftime(timePtr, sizeof(timePtr), "%Y-%m-%d-%H-%M-%S", localtime(&t));
	return timePtr;
}

void DrawTextToVideo(std::string srcUrl, std::string dstUrl, std::string drawText);

int main()
{
	DrawTextToVideo("G://DiscardVideo/«‡…ﬂ-Converted.ts", "G://DiscardVideo/«‡…ﬂ" + GetCurrentTime() + ".ts", "Green-Snake");
	system("pause");
	return 0;
}

void DrawTextToVideo(std::string srcUrl, std::string dstUrl, std::string drawText)
{
	VideoDrawText::Init();
	VideoDrawText doDrawText(srcUrl, dstUrl);
	doDrawText.SetDrawText(drawText);
	doDrawText.SetOutputFormat("mpegts");
	int ret = 0;
	if (ret = doDrawText.OpenInput() < 0)
		return;
	if (ret = doDrawText.InitDecodeContext() < 0)
		return;
	AVStream * videoStream = doDrawText.GetStream(AVMEDIA_TYPE_VIDEO);
	if (ret = doDrawText.InitEncodeContext(videoStream->codec->width, videoStream->codec->height) < 0)
		return;
	if (ret = doDrawText.InitFilter() < 0)
		return;
	if (ret = doDrawText.OpenOutput() < 0)
		return;
	AVFrame * srcFrame = av_frame_alloc();
	AVFrame * sinkFrame = av_frame_alloc();
	while (true)
	{
		auto pkt = doDrawText.ReadPacket();
		if (!pkt)
		{
			printf("Read Packet Failed!\n");
			av_frame_free(&srcFrame);
			av_frame_free(&sinkFrame);
			return;
		}
		if (pkt->stream_index != videoStream->index)
		{
			doDrawText.WritePacket(pkt);
			continue;
		}
		do
		{
			if (doDrawText.Decode(videoStream, pkt.get(), srcFrame) == false)
			{
				printf("Decode Failed!\n");
				break;
			}
			if (av_buffersrc_add_frame(doDrawText.GetSrcFilterCtx(), srcFrame) < 0)
			{
				printf("buffersrc Failed!");
				break;
			}
			if (av_buffersink_get_frame(doDrawText.GetSinkFilterCtx(), sinkFrame) < 0)
			{
				printf("Get buffersink Frame Failed!\n");
				break;
			}
			auto pktConverted = doDrawText.encode(doDrawText.GetEncodeCtx(), sinkFrame);
			if (pktConverted == NULL)
			{
				printf("Encode Failed!\n");
				break;
			}
			else
			{
				pktConverted->pts = pkt->pts;
				pktConverted->dts = pkt->dts;
				if (int ret = doDrawText.WritePacket(pktConverted, true) < 0)
				{
					printf("Write pktConverted Failed!\n");
				}
				else
				{
					printf("Writing.............\n");
				}

			}
		} while (false);
		av_frame_unref(srcFrame);
		av_frame_unref(sinkFrame);
	}

	av_frame_free(&srcFrame);
	av_frame_free(&sinkFrame);
	av_write_trailer(doDrawText.GetOutCtx());
}