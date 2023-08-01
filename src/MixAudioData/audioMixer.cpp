#include "audioMixer.h"
#include <iostream>

AudioMixer::AudioMixer() :
    inited(false),
    filterGraph(nullptr),
    audio_output_info_ptr(nullptr)
{
    //��ʼ����������ָ��
    audio_mix_info_ptr.reset(new AudioInfo);
    audio_mix_info_ptr->name = "amix";//����

    audio_sink_info_ptr.reset(new AudioInfo);
    audio_sink_info_ptr->name = "sink";//���


}

AudioMixer::~AudioMixer()
{
    if (inited)
    {
        exit();
    }
}

int AudioMixer::addAudioInput(uint32_t index, uint32_t samplerate, uint32_t channels, uint32_t bitsPerSample, AVSampleFormat format)
{
    std::lock_guard<std::mutex> locker(mutex);

    if (inited)
    {
        std::cout << __FUNCTION__ << "inited return -1!" << std::endl;
        return -1;
    }

    //����index�����Ƿ��Ѿ�����
    if (audio_input_infos.find(index) != audio_input_infos.end())
    {
        return -1;
    }

    //
    auto &filterInfo = audio_input_infos[index];
    //������Ƶ��ز���
    filterInfo.samplerate = samplerate;
    filterInfo.channels = channels;
    filterInfo.bitsPerSample = bitsPerSample;
    filterInfo.format = format;
    filterInfo.name = std::string("input") + std::to_string(index);

    return 0;

}

int AudioMixer::addAudioOutput(const uint32_t samplerate, const uint32_t channels, const uint32_t bitsPerSample, const AVSampleFormat format)
{
    std::lock_guard<std::mutex> locker(mutex);

    if (inited)
    {
        std::cout << __FUNCTION__ << "inited return -1!" << std::endl;
        return -1;
    }

    //������Ƶ��ز���
    audio_output_info_ptr.reset(new AudioInfo);
    audio_output_info_ptr->samplerate = samplerate;
    audio_output_info_ptr->channels = channels;
    audio_output_info_ptr->bitsPerSample = bitsPerSample;
    audio_output_info_ptr->format = format;
    audio_output_info_ptr->name = "output";

    return 0;
}

int AudioMixer::init(const char *duration)
{
    std::lock_guard<std::mutex> locker(mutex);

    if (inited)
    {
        std::cout << __FUNCTION__ << "inited return -1!" << std::endl;
        return -1;
    }

    if (!audio_output_info_ptr)
    {
        std::cout << __FUNCTION__ << "audio_output_info_ptr return -1!" << std::endl;
        return -1;
    }

    if (audio_input_infos.size() == 0)
    {
        std::cout << __FUNCTION__ << "audio_input_infos.size() == 0 return -1!" << std::endl;
        return -1;
    }

    //���������������̵�һ����װ
    filterGraph = avfilter_graph_alloc();
    if (filterGraph == nullptr)
    {
        return -1;
    }

    char args[512] = { 0 };
    //����
    const AVFilter *amix = avfilter_get_by_name("amix");
    audio_mix_info_ptr->filterCtx = avfilter_graph_alloc_filter(filterGraph, amix, "amix");
    //inputs=����������
    //duration=�������Ľ���(longest�����ʱ��,shortest���,first��һ�����������ʱ��)
    //dropout_transition= ����������ʱ,��������ʱ��
    snprintf(args, sizeof(args), "inputs=%d:duration=%s:dropout_transition=0",
        audio_input_infos.size(), duration);
    if (avfilter_init_str(audio_mix_info_ptr->filterCtx, args) != 0)
    {
        std::cout << __FUNCTION__ << "avfilter_init_str audio_mix_info_ptr failed!" << std::endl;
        return -1;
    }

    //���
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    audio_sink_info_ptr->filterCtx = avfilter_graph_alloc_filter(filterGraph, abuffersink, "sink");
    if (avfilter_init_str(audio_sink_info_ptr->filterCtx, nullptr) != 0)
    {
        std::cout << __FUNCTION__ << "avfilter_init_str audio_sink_info_ptr failed!" << std::endl;
        return -1;
    }

    //����
    for (auto &iter : audio_input_infos)
    {
        const AVFilter *abuffer = avfilter_get_by_name("abuffer");
        snprintf(args, sizeof(args),
            "sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
            iter.second.samplerate,
            av_get_sample_fmt_name(iter.second.format),
            av_get_default_channel_layout(iter.second.channels));

        iter.second.filterCtx = avfilter_graph_alloc_filter(filterGraph, abuffer, audio_output_info_ptr->name.c_str());

        if (avfilter_init_str(iter.second.filterCtx, args) != 0)
        {
            std::cout << __FUNCTION__ << "avfilter_init_str iter.second.filterCtx failed!" << std::endl;
            return -1;
        }

        //audio_input_infos[index] -> audio_min_info_ptr[index]
        if (avfilter_link(iter.second.filterCtx, 0, audio_mix_info_ptr->filterCtx, iter.first) != 0)
        {
            std::cout << __FUNCTION__ << "avfilter_link audio_input_infos[index] -> audio_min_info_ptr[index] failed!" << std::endl;
            return -1;
        }
    }

    if (audio_output_info_ptr != nullptr)
    {
        //ת����ʽ
        const AVFilter *aformat = avfilter_get_by_name("aformat");
        snprintf(args, sizeof(args),
            "sample_rates=%d:sample_fmts=%s:channel_layouts=0x%I64x",
            audio_output_info_ptr->samplerate,
            av_get_sample_fmt_name(audio_output_info_ptr->format),
            av_get_default_channel_layout(audio_output_info_ptr->channels));
        audio_output_info_ptr->filterCtx = avfilter_graph_alloc_filter(filterGraph,
            aformat,
            "aformat");


        if (avfilter_init_str(audio_output_info_ptr->filterCtx, args) != 0)
        {
            std::cout << __FUNCTION__ << "avfilter_init_str audio_output_info_ptr failed!" << std::endl;
            return -1;
        }

        //audio_mix_info_ptr -> audio_output_info_ptr
        if (avfilter_link(audio_mix_info_ptr->filterCtx, 0, audio_output_info_ptr->filterCtx, 0) != 0)
        {
            std::cout << __FUNCTION__ << "avfilter_link audio_mix_info_ptr -> audio_output_info_ptr failed!" << std::endl;
            return -1;
        }

        //audio_output_info_ptr -> audio_sink_info_ptr
        if (avfilter_link(audio_output_info_ptr->filterCtx, 0, audio_sink_info_ptr->filterCtx, 0) != 0)
        {
            std::cout << __FUNCTION__ << "avfilter_link audio_output_info_ptr -> audio_sink_info_ptr failed!" << std::endl;
            return -1;
        }

    }

    if (avfilter_graph_config(filterGraph, NULL) < 0)
    {
        std::cout << __FUNCTION__ << "avfilter_graph_config failed!" << std::endl;
        return -1;
    }

    inited = true;
    return 0;
}

int AudioMixer::exit()
{
    std::lock_guard<std::mutex> locker(mutex);

    if (inited)
    {
        //�ͷ�����
        for (auto iter : audio_input_infos)
        {
            if (iter.second.filterCtx != nullptr)
            {
                avfilter_free(iter.second.filterCtx);
            }
        }

        audio_input_infos.clear();
        //�ͷŸ�ʽת��
        if (audio_output_info_ptr && audio_output_info_ptr->filterCtx)
        {
            avfilter_free(audio_output_info_ptr->filterCtx);
            audio_output_info_ptr->filterCtx = nullptr;
        }
        //�ͷŻ���
        if (audio_mix_info_ptr->filterCtx)
        {
            avfilter_free(audio_mix_info_ptr->filterCtx);
            audio_mix_info_ptr->filterCtx = nullptr;
        }
        //�ͷ����
        if (audio_sink_info_ptr->filterCtx)
        {
            avfilter_free(audio_sink_info_ptr->filterCtx);
            audio_sink_info_ptr->filterCtx = nullptr;
        }

        avfilter_graph_free(&filterGraph);
        filterGraph = nullptr;
        inited = false;
    }
    return 0;
}

//���һ֡������index�����Ӧ��������
int AudioMixer::addFrame(uint32_t index, uint8_t *inBuf, uint32_t size)
{
    std::lock_guard<std::mutex> locker(mutex);
    if (!inited)
    {
        std::cout << __FUNCTION__ << "inited return -1!" << std::endl;
        return -1;
    }

    auto iter = audio_input_infos.find(index);
    if (iter == audio_input_infos.end())
    {
        std::cout << __FUNCTION__ << "audio_input_infos.find(index) return -1!" << std::endl;
        return -1;
    }

    if (inBuf && size > 0)
    {
        std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame *ptr) {av_frame_free(&ptr); });
        //������Ƶ����
        avFrame->sample_rate = iter->second.samplerate;
        avFrame->format = iter->second.format;
        avFrame->channel_layout = av_get_default_channel_layout(iter->second.channels);
        avFrame->nb_samples = size * 8 / iter->second.bitsPerSample / iter->second.channels;
        //������Ƶ��������ռ�
        av_frame_get_buffer(avFrame.get(), 1);
        memcpy(avFrame->data[0], inBuf, size);
        if (av_buffersrc_add_frame(iter->second.filterCtx, avFrame.get()) != 0)
        {
            return -1;
        }
    }
    else
    {
        //��ˢ
        if (av_buffersrc_add_frame(iter->second.filterCtx, NULL) != 0)
        {
            return -1;
        }
    }

    return 0;
}

//��ȡһ֡
int AudioMixer::getFrame(uint8_t *outBuf, uint32_t maxOutBufSize)
{
    std::lock_guard<std::mutex> locker(mutex);
    if (!inited)
    {
        std::cout << __FUNCTION__ << "inited return -1!" << std::endl;
        return -1;
    }

    std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame *ptr) { av_frame_free(&ptr); });

    //��ȡ���֡
    int ret = av_buffersink_get_frame(audio_sink_info_ptr->filterCtx, avFrame.get());
    if (ret < 0)
    {
        return -1;
    }

    //������Ƶ����һ֡���ݵ�����
    int size = av_samples_get_buffer_size(NULL, avFrame->channels, avFrame->nb_samples, (AVSampleFormat)avFrame->format, 1);
    if (size > (int) maxOutBufSize)
    {
        return 0;
    }
    //����֡����
    memcpy(outBuf, avFrame->data[0], size);
    return size;
}

