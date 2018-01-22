#ifndef FFMPEG_REQUIRED
#define FFMPEG_REQUIRED

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libavutil\channel_layout.h"
#include "libavutil\common.h"
#include "libavutil\imgutils.h"
#include "libswscale\swscale.h" 
#include "libavutil\imgutils.h"    
#include "libavutil\opt.h"       
#include "libavutil\mathematics.h"    
#include "libavutil\samplefmt.h" 
#include "libavutil\time.h"
};

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avutil.lib")
//#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")

#endif // !FFMPEG_REQUIRED


