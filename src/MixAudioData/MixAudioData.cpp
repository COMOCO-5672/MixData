#include <iostream>
#include "audiomixer.h"

using namespace std;

#define PCM1_FRAME_SIZE (4096*2)
#define PCM2_FRAME_SIZE (4096)
#define PCM_OUT_FRAME_SIZE (40000)


int main()
{
    cout << "Hello Audiomixer!" << endl;

    AudioMixer audioMix;

    //添加输入流
    //参数根据实际pcm数据设置
    audioMix.addAudioInput(0, 44100, 2, 16, AV_SAMPLE_FMT_S16);
    audioMix.addAudioInput(1, 44100, 2, 16, AV_SAMPLE_FMT_S16);

    //添加输出流
    audioMix.addAudioOutput(44100, 2, 16, AV_SAMPLE_FMT_S16);

    AVFormatContext *pFmtCtx1 { nullptr };
    pFmtCtx1 = avformat_alloc_context();
    avformat_open_input(&pFmtCtx1, "", nullptr, nullptr);


    //初始化
    if (audioMix.init() < 0)
    {
        printf("audioMix.init() failed!\n");
        return -1;
    }

    int len1, len2 = 0;
    uint8_t buf1[PCM1_FRAME_SIZE];
    uint8_t buf2[PCM2_FRAME_SIZE];
    FILE *file1 = fopen("1.pcm", "rb");
    if (!file1) {
        printf("fopen pcm1.pcm failed\n");
        return -1;
    }

    FILE *file2 = fopen("2.pcm", "rb");
    if (!file2) {
        printf("fopen pcm2.pcm failed\n");
        return -1;
    }

    FILE *file_out = fopen("output.pcm", "wb");
    if (!file_out) {
        printf("fopen output.pcm failed\n");
        return -1;
    }

    uint8_t out_buf[PCM_OUT_FRAME_SIZE];
    uint32_t out_size = 0;

    int file1_finish = 0;
	int file2_finish = 0;
    while (1)
    {
        len1 = fread(buf1, 1, PCM1_FRAME_SIZE, file1);
        len2 = fread(buf2, 1, PCM2_FRAME_SIZE, file2);
        if (len1 > 0 || len2 > 0)
        {
            //通道1
            if (len1 > 0)
            {
                if (audioMix.addFrame(0, buf1, len1) < 0)
                {
                    printf("audioMix.addFrame(0, buf1, len1) failed!\n");
                    break;
                }
            }
            else
            {
                if (file1_finish == 0)
                {
                    file1_finish = 1;
                    if (audioMix.addFrame(0, NULL, 0) < 0)
                    {
                        printf("audioMix.addFrame(0, NULL, 0) failed!\n");
                    }
                }
            }

            //通道2
            if (len2 > 0)
            {
                if (audioMix.addFrame(1, buf2, len2) < 0)
                {
                    printf("audioMix.addFrame(1, buf2, len2) failed!\n");
                    break;
                }
            }
            else
            {
                if (file2_finish == 0)
                {
                    file2_finish = 1;
                    if (audioMix.addFrame(1, NULL, 0) < 0)
                    {
                        printf("audioMix.addFrame(1, NULL, 0) failed!\n");
                    }
                }
            }

            int ret = 0;
            while ((ret = audioMix.getFrame(out_buf, 10240)) >= 0)
            {
                //写入文件
                fwrite(out_buf, 1, ret, file_out);
            }
        }
        else
        {
            printf("read file end!\n");
            break;
        }
    }

    audioMix.exit();
    if (file_out)
        fclose(file_out);
    if (file1)
        fclose(file1);
    if (file2)
        fclose(file2);
    cout << "End Audiomixer!" << endl;
    return 0;
}

