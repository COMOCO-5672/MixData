// MixVideoData.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "ffmpeg_util.h"

int main()
{
	AVFormatContext *fmtctx = avformat_alloc_context();

	std::string url = "../../video1.mp4";

	if (avformat_open_input(&fmtctx, url.c_str(), nullptr, nullptr))
	{
		return 0;
	}

	avformat_find_stream_info(fmtctx, nullptr);
	std::cout << "stream count:" << fmtctx->nb_streams << "\r\n";

	av_dump_format(fmtctx, 0, url.c_str(), 0);

	AVInputFormat *input_format = fmtctx->iformat;

	std::cout << "input_format name:" << fmtctx->iformat->name << "\r\n";

	AVStream *st { nullptr };
	AVCodecContext *av_codec_context { nullptr };
	AVCodec *av_codec { nullptr };

	for (int i = 0; i < fmtctx->nb_streams; ++i)
	{
		if (fmtctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			st = fmtctx->streams[i];
			break;
		}
	}

	av_codec = avcodec_find_decoder(st->codecpar->codec_id);
	av_codec_context = avcodec_alloc_context3(av_codec);

	avcodec_parameters_to_context(av_codec_context, st->codecpar);



	return 1;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
