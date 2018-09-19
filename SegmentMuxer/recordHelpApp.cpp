#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef __cplusplus
}
#endif

#include "Baselibs.h"
#include "MeidaService.h"

static AVFormatContext *i_fmt_ctx=NULL;
static AVStream *i_video_stream=NULL;
static AVStream *o_video_stream=NULL;
static MeidaService service;

int main(int argc, char **argv)
{

	char *url=NULL;
	//if (argc<2)
	//{
	//	printf("please input media url \n");
	//	return 0;
	//}
	url=argv[1];
	printf("avformat_version= %d \n",avformat_version());
	printf("avformat_configuration= %s \n",avformat_configuration());
	av_register_all();
	avformat_network_init();
 
	//使用TCP连接打开RTSP，设置最大延迟时间
	AVDictionary *avdic=NULL;   
	av_dict_set(&avdic, "rtsp_transport","tcp",0); 
	av_dict_set(&avdic, "max_delay","1000000",0); 
	av_dict_set(&avdic, "buffer_size", "1024000", 0);

	url="rtsp://admin:e529xmedia@192.168.199.55/Streaming/Channels/101";

	if (avformat_open_input(&i_fmt_ctx, url, NULL, &avdic) < 0)
	{
		fprintf(stderr, "could not open input file\n");
		return -1;
	}
 
	if (avformat_find_stream_info(i_fmt_ctx, NULL)<0)
	{
		fprintf(stderr, "could not find stream info\n");
		return -1;
	}
 
	//av_dump_format(i_fmt_ctx, 0, argv[1], 0);
	service.record_set_path_fmt("c:/rec/my%d.mp4");
	service.record_init_output(i_fmt_ctx);
	AVPacket tmp_pkt;
	while (1)
	{
		av_init_packet(&tmp_pkt);
		if (av_read_frame(i_fmt_ctx, &tmp_pkt) <0)
			break;
		service.record_write_packet(&tmp_pkt);
		av_packet_unref(&tmp_pkt);
	}

	avformat_close_input(&i_fmt_ctx);
	return 0;
}
