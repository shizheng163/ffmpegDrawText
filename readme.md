# 环境配置 # 
	1.附加ffmpeg头文件:项目属性->C/C++->常规->附加包含目录:  .\include\x64
	2.附加ffmpeg相关lib:项目属性->链接器->常规->附加库目录:   .\lib
	3.附加ffmpeg相关dll:项目属性->配置属性->调试->环境:path=%path%;.\bin 或者将bin下的dll复制到生成的Debug目录下

	Demo存在的问题:
		只能转换相同格式的数据。
		音视频数据有些不同步。

# 概述 #
    MP4等封装格式的媒体文件，存放的都是编码后音视频的二进制数据。
    VLC,ffplay等播放器播放视频时,会将视频解码还原图像。播放器解码出的是图像数据，并不是图片数据(像素集合)。
    YUV420,YUV444,后面的数据为在像素中所占大小的比例。
    Y-表示颜色，白与黑
    U-表示色差

通常的视频格式,屏幕分辨率：

    1080P 1920*1080 P逐行扫描
    720P  1280*720
    VGA   640*480
    QVGA  320*240
    ...
    分辨率都是可以被4整除

  添加水印的原理为像素覆盖

# FFMpeg对于添加水印的实现原理 #
    Filter过滤器。最简单的可以理解为FIFO的管道

## filter过滤器的相关概念 ##
    Graph （存放filter的集合，实际上存放的是过滤器的上下文。）
        一个流程图。曲线连成的图，例如将多个节点（过滤器）连接起来，有起点（一个或多个）和终点（一个或多个终点）。
        用相同的流程，代码去完成一样的事情。
        例如:
            一个添加文字的需求，它会有三个流程，输入frame，处理frame，输出frame。
            那么相对应的它的流程图（Graph）中应该对应有三个filter来完成来对应的功能。
            source->darwtext->sink
                起点(source)为 处理pkt解码后的frame的filter
                终点/某个节点(sink-某个节点)为 处理需要添加水印的frame的filter。
                中途节点,drawtext 为接收source的输入，然后进行数据处理（给Frame添加文字），再将处理后的数据交给sink。
            其中
                source的宽高为解码后的frame的宽高。
                sink  的宽高根据需求来变。
                每个过滤器都有相对应的配置可以接收输入(in)输出(out)。
            那么为什么不只用一个过滤器（直接输入，处理，输出）去完成这个需求呢？
                有一种过滤器叫做copy，它的任务仅仅是什么都不做只是把传入的frame拷贝一下直接输出。
                为什么不只用一个过滤器去完成？为了兼容，还是设计如此，待补充。
            实际上还有一个scale过滤器在draw与sink之间，来调整压缩添加文字后的drawtext
                这个过滤器没有手动添加，是darwtext自己完成的吗？

    Filter Context
        Filter的上下文环境
    Filter 
        都为frame到frame
        过滤器具有in,out等属性（类似于类的成员变量。）

## filter过滤器的结构 ##
    input1,input2 等多个输入 例如打水印时一个 Frame of file, 一个frame of picture
        基本单位都是frame
    output

    伪代码
    struct Filter
    {
        InOut* inArray;//输入 Pad 
        InOut* output; //输出 Pad
        void (*FilterFrame)(FrameMain,FrameSub,...); //处理Frame的回调。根据filter的不同导致参数不同
        void (*RequestFrame)();// 不清楚。 去output中拿帧，没有的时候调用的函数
    };
