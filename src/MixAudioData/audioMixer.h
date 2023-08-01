#pragma once

#include <memory>
#include "ffmpeg_util.h"
#include <string>
#include <mutex>
#include <map>

class AudioMixer
{
public:
    AudioMixer();
    virtual ~AudioMixer();

    //�����Ƶ����ͨ��
    int addAudioInput(uint32_t index, uint32_t samplerate, uint32_t channels, uint32_t bitsPerSample, AVSampleFormat format);
    //�����Ƶ���ͨ��
    int addAudioOutput(const uint32_t samplerate, const uint32_t channels, const uint32_t bitsPerSample, const AVSampleFormat format);

    //���ͨ��ʱ������������ʱ�����һ��ͨ��Ϊֹ
    int init(const char *duration = "longest");
    int exit();

    int addFrame(uint32_t index, uint8_t *inBuf, uint32_t size);
    int getFrame(uint8_t *outBuf, uint32_t maxOutBufSize);

private:
    struct AudioInfo
    {
        AudioInfo()
        {
            filterCtx = nullptr;
        }

        AVFilterContext *filterCtx;
        uint32_t samplerate;
        uint32_t channels;
        uint32_t bitsPerSample;
        AVSampleFormat format;
        std::string name;
    };

    AVFilterGraph *filterGraph;

    bool inited;
    std::mutex mutex;
    //����
    std::map<uint32_t, AudioInfo> audio_input_infos;
    //ת����ʽ
    std::shared_ptr<AudioInfo> audio_output_info_ptr;
    //���
    std::shared_ptr<AudioInfo> audio_sink_info_ptr;
    //����
    std::shared_ptr<AudioInfo> audio_mix_info_ptr;

};
