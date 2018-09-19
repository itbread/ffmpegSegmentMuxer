#include "MeidaService.h"
#include <stdio.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <process.h> 
#include <Windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif


static const char* REC_FOLDER="rec";
static long m_frames;

static bool IsPathExist(const char* path)
{
	int nRet = access(path, 0);
	return 0 == nRet || EACCES == nRet;
}

MeidaService::MeidaService(void)
{
	inputContext =NULL;
	outputContext=NULL;
	outputFormat =NULL;
	video_stream_idx=-1 ;
	audio_stream_idx=-1;
	frame_index=0;
	m_frames=1;
	record_cycle=1;
	frame_rate = 0;
}

MeidaService::~MeidaService(void)
{

}

void MeidaService::record_set_path_fmt(const char* path_fmt)
{
	fmt=path_fmt;
}

int MeidaService::record_init_output(AVFormatContext *inputContext)
{
	video_stream_idx = av_find_best_stream(inputContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	audio_stream_idx = av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	frame_rate = 0;

	AVStream *stream = inputContext->streams[video_stream_idx ];  

	if(stream->avg_frame_rate.den > 0)
	{
		frame_rate = stream->avg_frame_rate.num/stream->avg_frame_rate.den;
	}
	else
		frame_rate =25;
	m_frames = (audio_stream_idx >= 0) ? record_cycle * frame_rate*60*2 : record_cycle*frame_rate*60;
	return record_open_output(inputContext);
}

int MeidaService::record_open_output(AVFormatContext *ic)
{
	//char path[128]={"d:/rec/my%d.mp4"};
	AVDictionary *options=NULL;
	inputContext=ic;

	//if(!outputFormat)
	//	outputFormat=av_guess_format("segment",path,"mkv");
	//int ret  = avformat_alloc_output_context2(&outputContext, outputFormat, "mkv", path);
	int ret  = avformat_alloc_output_context2(&outputContext, NULL, "segment", fmt.c_str());
	if(outputContext== NULL)	
	{
		av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
		goto Error;
	}

	ret = avio_open2(&outputContext->pb, fmt.c_str(), AVIO_FLAG_WRITE,NULL, NULL);	
	if(ret < 0)	
	{
		av_log(NULL, AV_LOG_ERROR, "open avio failed");
		goto Error;
	}

	for (int i = 0; i < inputContext->nb_streams; i++) 
	{
		AVStream *st;
		AVCodecParameters *ipar, *opar;

		if (!(st = avformat_new_stream(outputContext, NULL)))
			goto Error;
		ipar = inputContext->streams[i]->codecpar;
		opar = st->codecpar;
		avcodec_parameters_copy(opar, ipar);
		if (!outputContext->oformat->codec_tag ||
			av_codec_get_id (outputContext->oformat->codec_tag, ipar->codec_tag) == opar->codec_id ||
			av_codec_get_tag(outputContext->oformat->codec_tag, ipar->codec_id) <= 0) 
		{
			opar->codec_tag = ipar->codec_tag;
		} 
		else 
		{
			opar->codec_tag = 0;
		}
		st->sample_aspect_ratio =inputContext->streams[i]->sample_aspect_ratio;
		st->time_base = inputContext->streams[i]->time_base;
		av_dict_copy(&st->metadata, inputContext->streams[i]->metadata, 0);
	}

	video_stream_idx = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	audio_stream_idx = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);


 
	av_dict_set(&options, "individual_header_trailer","1",0); 	
	av_dict_set(&options, "reset_timestamps","1",0); 
	av_dict_set(&options, "segment_format","mp4",0); 
	av_dict_set(&options, "segment_time","60",0); 		
	//av_dict_set(&options, "strftime","1",0); 			
	av_dict_set(&options, "increment_tc ","1",0); 		

	ret = avformat_write_header(outputContext, &options);
	if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "format write header failed");
		goto Error;
	}

	av_log(NULL, AV_LOG_FATAL, " Open output file success %s\n",fmt.c_str());			
	return ret ;
Error:
	if(outputContext) 
	{
		for(int i = 0; i < outputContext->nb_streams; i++) 
		{
			avcodec_close(outputContext->streams[i]->codec);
		}
		avformat_close_input(&outputContext);
		outputContext=NULL;
	}
	return ret ;
}

void MeidaService::record_write_packet(AVPacket* pkt)
{
	if (outputContext)
	av_interleaved_write_frame(outputContext, pkt);

	frame_index++;
		if(frame_index%100==1)
	printf("[  write pkt %ld]\n",frame_index);
}


void MeidaService::record_close_output()
{
	int i;
	//AVFormatContext* ofmt_ctx = *outfmt_ctx;
	frame_index=0;
	if(outputContext != NULL)
	{
		/* 写文件尾 */
		int ret = av_write_trailer(outputContext);
		if(ret < 0)
		{
			//LOG_ERROR(printf("[  write trailer failed]\n"));
		}


		/* 关闭清理 */
		if(outputContext && outputContext->oformat && !(outputContext->oformat->flags & AVFMT_NOFILE))
		{
			avio_closep(&outputContext->pb);
			//LOG_DEBUG(printf("[ cleanup avio ]\n"));
		}
		for(i=0; i<outputContext->nb_streams; i++)
		{
			avcodec_parameters_free(&outputContext->streams[i]->codecpar);
			av_dict_free(&outputContext->streams[i]->metadata);
			//LOG_DEBUG(printf("[ cleanup avcodec parameters ]\n"));
		}
		avformat_free_context(outputContext);
		outputContext=NULL;
		//LOG_DEBUG(printf("[ cleanup avformat ]\n"));
	}
}

