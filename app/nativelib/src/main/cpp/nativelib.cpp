#include <jni.h>
#include <string>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <unistd.h>
#include <android/log.h>



extern "C" {
#include "../../../libs/include/libavformat/avformat.h"
#include "../../../libs/include/libavcodec/avcodec.h"
#include "../../../libs/include/libswscale/swscale.h"
#include "../../../libs/include/libavutil/imgutils.h"
#include "../../../libs/include/libavutil/frame.h"
#include "../../../libs/include/libavutil/opt.h"
#include "../../../libs/include/libswresample/swresample.h"
}
//Android 打印log
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,"FFmpeg_log",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"FFmpeg_log",__VA_ARGS__)
extern "C"
JNIEXPORT void JNICALL
Java_cn_lee_nativelib_NativeLib_playLocalVideo(JNIEnv *env, jobject thiz, jstring source,
                                               jobject surface) {
//记录结果

        int result;
        const char *path = env->GetStringUTFChars(source,0);
        //注册FFMPEG组件
      //  av_register_all();
        //初始化AVFormatContext
        AVFormatContext *formatContext = avformat_alloc_context();
        //打开视频文件
        result = avformat_open_input(&formatContext,path,NULL,NULL);
                if(result<0){
                        LOGE("player error :Can  not  open  video file");
                        return;
                }

        //查找视频文件的流信息
        result = avformat_find_stream_info(formatContext,NULL);
        if(result <0){
                LOGE("Player Error : Can not find video file stream info");
                return;
        }
        long long duration;
        if(formatContext->duration != AV_NOPTS_VALUE){
            duration = (long long)formatContext->duration;
            LOGE("durtion = %d",duration);
        }

        //查找视频解码器
        int video_stream_index = -1;
        for (int i = 0;i<formatContext->nb_streams;i++){
                //匹配视频流
                if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
                        video_stream_index = i;
                }
        }
        //没有找到视频
        if(video_stream_index == -1){
                LOGE("Player Error : Can not find video stream");
                return;
        }
        //初始化视频编码器上下文
        AVCodecContext *video_codec_context = avcodec_alloc_context3(NULL);
        avcodec_parameters_to_context(video_codec_context,formatContext->streams[video_stream_index]->codecpar);

        //初始化视频编码器
        const AVCodec *video_codec = avcodec_find_decoder(video_codec_context->codec_id);
      //  av_opt_set(video_codec_context->priv_data, "tune", "zerolatency", 0);
        if(video_codec == NULL){
                LOGE("Player Error : Can not find video codec");
                return;
        }

        //打开视频解码器
        result = avcodec_open2(video_codec_context,video_codec,NULL);
        if(result<0){
                LOGE("Player Error : Can not find video stream");
                return;
        }

        //获取视频宽高
        int videoWidth = video_codec_context->width;
        int videoHeight = video_codec_context->height;
        LOGD("videoWidth and videoHeight = %d,%d",videoWidth,videoHeight);
        //初始化native Window 用于播放
        ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env,surface);
        if(nativeWindow == NULL){
                LOGE("Player Error : Can not create native window");
                return;
        }
        //通过设置宽高限制缓冲区的像素数量，而非屏幕的物理显示尺寸
        //如果缓冲区与物理屏幕的显示尺寸不符，则实际显示可能会拉伸，或者被压缩
        result = ANativeWindow_setBuffersGeometry(nativeWindow,videoWidth,videoHeight,WINDOW_FORMAT_RGBA_8888);
        if(result < 0){
                LOGE("Player Error : Can not set native window buffer");
                ANativeWindow_release(nativeWindow);
                return;
        }
        //定义绘图缓冲区
        ANativeWindow_Buffer windowBuffer;
        //声明数据容器 有3个
        //解码前的数据容器  Packet 编码数据
        AVPacket *packet = av_packet_alloc();
        //解码后数据容器  Frame 像素容器  不能直接播放像素数据  需要转换
        AVFrame *frame = av_frame_alloc();
        //转换后的数据容器  这里的数据可以播放
        AVFrame *rgba_frame = av_frame_alloc();
        //数据转换准备
        //输出Buffer
        int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA,videoWidth,videoHeight,1);
        //申请buffer 内存
        uint8_t *out_buffer = (uint8_t *) av_malloc(buffer_size * sizeof(uint8_t));
        av_image_fill_arrays(rgba_frame->data,rgba_frame->linesize,out_buffer,AV_PIX_FMT_RGBA,videoWidth,videoHeight,1);
        //数据格式转换上下文
        struct SwsContext *data_convert_context = sws_getContext(
                videoWidth,videoHeight,video_codec_context->pix_fmt,
                videoWidth,videoHeight,AV_PIX_FMT_RGBA,
                SWS_BICUBIC,NULL,NULL,NULL
                );
        //开始读取帧
    while (true){

          int k =   av_read_frame(formatContext, packet);
          if(k<0){
                  break;
          }
            //匹配视频流
            if(packet->stream_index == video_stream_index){
                    //解码
                    result = avcodec_send_packet(video_codec_context,packet);
                   // if(result < 0 ){
                    if(result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF){
                            LOGE("Player Error : codec step 1 fail");
                            return;
                    }
                    result = avcodec_receive_frame(video_codec_context,frame);
                    if(result < 0 && result != AVERROR_EOF){
                    //avcodec_receive_frame可能需要调用多次 avcodec_receive_frame才能获取全部的解码音频数据 return 改成 continue
                            LOGE("Player Error : codec step 2 fail result = %d",result);
                            continue;
                    }
                    //数据格式转换
                    result = sws_scale(
                            data_convert_context,
                            (const uint8_t* const*)frame->data,frame->linesize,
                            0,videoHeight,
                            rgba_frame->data,rgba_frame->linesize
                            );
                    if(result < 0){
                            LOGE("Player Error: data convert fail result = %d",result);
                            return;
                    }
                    //播放
                    result = ANativeWindow_lock(nativeWindow,&windowBuffer,NULL);
                    if(result < 0){
                            LOGE("Player Error : Can not lock native window");
                    }else{
                            //将图像绘制到界面上
                            //注意：这里rgba_frame 一行的像素和window——buffer 一行的像素长度可能不一致
                            //需要转换好  否者会花屏
                            uint8_t *bits = (uint8_t *) windowBuffer.bits;
                            for (int h = 0;h < videoHeight;h++){
                                    memcpy(bits + h *windowBuffer.stride * 4,
                                           out_buffer + h * rgba_frame -> linesize[0],
                                           rgba_frame -> linesize[0]);
                            }
                            ANativeWindow_unlockAndPost(nativeWindow);
                    }
            }
            //释放packet引用
            av_packet_unref(packet);
    }

    //释放转换器
    sws_freeContext(data_convert_context);
    //释放输出 buffer
    av_free(out_buffer);
    //释放rgba_frame
    av_frame_free(&rgba_frame);
    //释放frame
    av_frame_free(&frame);
    //释放packet
    av_packet_free(&packet);
    //释放nativeWindow
    ANativeWindow_release(nativeWindow);
    //关闭 video_codec_context
    avcodec_close(video_codec_context);
    //释放format_context
    avformat_close_input(&formatContext);
    //释放 source
    env ->ReleaseStringUTFChars(source,path);

}
extern "C"
JNIEXPORT void JNICALL
Java_cn_lee_nativelib_NativeLib_sound(JNIEnv *env, jobject instance, jstring input_, jstring output_) {
        const char *input = env->GetStringUTFChars(input_,0);
        const char *output = env->GetStringUTFChars(output_,0);

    LOGE("111111");

      //  av_register_all() 废弃不再使用
      AVFormatContext *pFormatContext = avformat_alloc_context();
      if(avformat_open_input(&pFormatContext,input,NULL,NULL) != 0){
              LOGE("打开输入文件失败");
              return;
      }

      if(avformat_find_stream_info(pFormatContext,NULL) < 0){
              LOGE("获取信息失败");
              return;
      }

      int audio_stream_index = -1;

        for (int i =0; i < pFormatContext->nb_streams; ++i){

                if(pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
                    LOGE("找到音频id %d",pFormatContext->streams[i]->codecpar->codec_type);
                    audio_stream_index = i;
                    break;
                }

        }

        //获取解码装置的上下文
  //  AVCodecContext *pCodecCtx = pFormatContext->streams[audio_stream_index]->codecpar->

   //   const  AVCodec *avCodec = avcodec_find_decoder(pFormatContext->streams[audio_stream_index]->codecpar->codec_id);
      AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL)
    {
        LOGE("Could not allocate AVCodecContext ");
        return;
    }
    avcodec_parameters_to_context(pCodecCtx,pFormatContext->streams[audio_stream_index]->codecpar);
    //获取解码器
      const AVCodec *pCodec = avcodec_find_decoder(pCodecCtx ->codec_id);
    //打开解码器
      if(avcodec_open2(pCodecCtx,pCodec,NULL) < 0){
          LOGE("<0");
      }
    //s申请AVPacket 装解码前的数据
       AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //申请avframe 装解码后的数据
      AVFrame *frame = av_frame_alloc();
      //得到swrcontext 进行重采样
      SwrContext * swrContext = swr_alloc();
      //缓冲区
      uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2);
      //输出声道布局
      uint16_t out_ch_layout = AV_CH_LAYOUT_STEREO;
      //输出采样位数
      enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
      //输出采样率必须与输入相同
      int out_sample_rate = pCodecCtx->sample_rate;
      //将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swrContext,out_ch_layout,out_format,out_sample_rate,pCodecCtx->channel_layout,
                       pCodecCtx->sample_fmt,pCodecCtx->sample_rate,0,NULL);
      swr_init(swrContext);
      //获取通道数
      int out_channels_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

/*    jclass my_player = env->FindClass("cn/lee/nativelib/NativeLib");
    LOGE("class = %d",my_player);

    jmethodID construct = env->GetMethodID(my_player, "<init>", "()V");

    jobject NDKObj = env->NewObject(my_player,construct);
//反射得到方法
    char *cc = "testJni";
    jmethodID createAudio = env->GetMethodID(my_player, cc, "(Ljava/lang/String;I)V");
    jstring mes = env->NewStringUTF("6666");
    env->CallVoidMethod(NDKObj, createAudio,mes,1);*/


    //反射得到class类型
      jclass my_player = env -> FindClass("cn/lee/nativelib/NativeLib");
      //调用构造
    jmethodID construct = env->GetMethodID(my_player, "<init>", "()V");
    //生成实例
    jobject NDKObj = env->NewObject(my_player,construct);

      //反射得到createAudio 方法
      char* cc= "createAudio";
      jmethodID createAudio = env->GetMethodID(my_player,cc,"(II)V");
        LOGE("id = %d",createAudio);
      //反射调用createAudio
      env->CallVoidMethod(NDKObj,createAudio,44100,out_channels_nb);

      jmethodID playTrack = env->GetMethodID(my_player, "playTrack", "([BI)V");

    int got_frame;

    while (av_read_frame(pFormatContext,packet) >= 0){

        if(packet->stream_index == audio_stream_index){
            //解码
       //   avcodec_decode_audio4(pCodecCtx,frame,&got_frame,packet);
            if( avcodec_send_packet(pCodecCtx, packet) < 0 || (got_frame=avcodec_receive_frame(pCodecCtx,frame))<0)
            {
                LOGE("< 0 ");
                return;
            }
            LOGE("got_frame = %d",got_frame);
           // if(got_frame){
                //解码
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
                //缓冲区大小
                int size = av_samples_get_buffer_size(NULL, out_channels_nb, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
                jbyteArray audio_sample_array = env->NewByteArray(size);
                env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
                env->CallVoidMethod(NDKObj,playTrack,audio_sample_array,size);
                env->DeleteLocalRef(audio_sample_array);
             //   LOGE("解码");
           // }

        }

    }

    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatContext);

    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);

    LOGE("STOP");

}