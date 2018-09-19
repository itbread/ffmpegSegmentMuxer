#pragma once

#include <stdio.h>
#include<string>
using namespace std;
#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

class MeidaService
{
public:
	MeidaService(void);
	~MeidaService(void);
	void record_set_path_fmt(const char* path_fmt);
	int record_init_output(AVFormatContext *inputContext);
	int record_open_output(AVFormatContext *inputContext);
	void record_write_packet(AVPacket* pkt);
	void record_close_output();

private:
	string fmt;
	AVPacket temp;
	AVFormatContext *inputContext ;
	AVFormatContext *outputContext;
	AVOutputFormat *outputFormat;
	int video_stream_idx ;
	int audio_stream_idx;
	long frame_index;
	long record_cycle;
	int  frame_rate ;
};

