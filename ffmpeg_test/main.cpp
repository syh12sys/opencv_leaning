
#include <iostream>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/utils/logger.hpp>

using namespace cv;
using namespace cv;
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>

#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}

#include <unistd.h>


void AVFrame2Img(AVFrame* pFrame, cv::Mat& img);
void Yuv420p2Rgb32(const uchar* yuvBuffer_in, const uchar* rgbBuffer_out, int width, int height);

int PullRTMPStreamAndDecode()
{
    AVFormatContext* ifmt_ctx = NULL;
    AVPacket pkt;
    AVFrame* pframe = NULL;
    int ret, i;
    int videoindex = -1;

    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;

    const char* in_filename = "rtmp://ns8.indexforce.com/home/mystream";   //芒果台rtmp地址
    const char* out_filename_v = "test.h264";   //Output file URL
    //Register
    av_register_all();
    //Network
    avformat_network_init();
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0)
    {
        printf("Could not open input file.");
        return -1;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
    {
        printf("Failed to retrieve input stream information");
        return -1;
    }

    videoindex = -1;
    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
        }
    }
    //Find H.264 Decoder
    //pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    pCodec = avcodec_find_decoder_by_name("h264_cuvid");
    if (pCodec == NULL) {
        printf("Couldn't find Codec.\n");
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
    pCodecCtx->pix_fmt = /*AV_PIX_FMT_NV12*/AV_PIX_FMT_CUDA;
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Couldn't open codec.\n");
        return -1;
    }

    pframe = av_frame_alloc();
    if (!pframe)
    {
        printf("Could not allocate video frame\n");
        exit(1);
    }

    FILE* fp_video = fopen(out_filename_v, "wb+"); //用于保存H.264

    cv::Mat image_test;

    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");

    while (av_read_frame(ifmt_ctx, &pkt) >= 0)
    {
        if (pkt.stream_index == videoindex) {

            av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoindex]->codec, NULL, &pkt.data, &pkt.size,
                pkt.data, pkt.size, 0);

           // printf("Write Video Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);

            //保存为h.264 该函数用于测试
            //fwrite(pkt.data, 1, pkt.size, fp_video);

            // Decode AVPacket
            if (pkt.size)
            {
                ret = avcodec_send_packet(pCodecCtx, &pkt);
                if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    std::cout << "avcodec_send_packet: " << ret << std::endl;
                    continue;
                }
                //Get AVframe
                ret = avcodec_receive_frame(pCodecCtx, pframe);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    std::cout << "avcodec_receive_frame: " << ret << std::endl;
                    continue;
                }
                //AVframe to rgb
                AVFrame2Img(pframe, image_test);
            }

        }
        //Free AvPacket
        av_packet_unref(&pkt);
    }
    //Close filter
    av_bitstream_filter_close(h264bsfc);
    fclose(fp_video);
    avformat_close_input(&ifmt_ctx);
    if (ret < 0 && ret != AVERROR_EOF)
    {
        printf("Error occurred.\n");
        return -1;
    }
    return 0;
}

void Yuv420p2Rgb32(const uchar* yuvBuffer_in, const uchar* rgbBuffer_out, int width, int height)
{
    uchar* yuvBuffer = (uchar*)yuvBuffer_in;
    uchar* rgb32Buffer = (uchar*)rgbBuffer_out;

    int channels = 3;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = y * width + x;

            int indexY = y * width + x;
            int indexU = width * height + y / 2 * width / 2 + x / 2;
            int indexV = width * height + width * height / 4 + y / 2 * width / 2 + x / 2;

            uchar Y = yuvBuffer[indexY];
            uchar U = yuvBuffer[indexU];
            uchar V = yuvBuffer[indexV];

            int R = Y + 1.402 * (V - 128);
            int G = Y - 0.34413 * (U - 128) - 0.71414 * (V - 128);
            int B = Y + 1.772 * (U - 128);
            R = (R < 0) ? 0 : R;
            G = (G < 0) ? 0 : G;
            B = (B < 0) ? 0 : B;
            R = (R > 255) ? 255 : R;
            G = (G > 255) ? 255 : G;
            B = (B > 255) ? 255 : B;

            rgb32Buffer[(y * width + x) * channels + 2] = uchar(R);
            rgb32Buffer[(y * width + x) * channels + 1] = uchar(G);
            rgb32Buffer[(y * width + x) * channels + 0] = uchar(B);
        }
    }
}

void AVFrame2Img(AVFrame* pFrame, cv::Mat& img)
{
    int frameHeight = pFrame->height;
    int frameWidth = pFrame->width;
    int channels = 3;
    //输出图像分配内存
    img = cv::Mat::zeros(frameHeight, frameWidth, CV_8UC3);
    Mat output = cv::Mat::zeros(frameHeight, frameWidth, CV_8U);

    //cuda::GpuMat inputMat(1, frameHeight * frameWidth, CV_8U, pFrame->data);
    //cuda::GpuMat outputMat;
    //cv::cuda::cvtColor(inputMat, outputMat, COLOR_YUV420sp2RGB);

    //cv::Mat image;
    //outputMat.download(image);

    //创建保存yuv数据的buffer
    int frameSize = frameHeight * frameWidth * sizeof(uchar) * channels;
    uchar* pDecodedBuffer = (uchar*)malloc(frameSize);
    memset(pDecodedBuffer, 0, frameSize);

    ////从AVFrame中获取yuv420p数据，并保存到buffer
    //int i, j, k;
    ////拷贝y分量
    //for (i = 0; i < frameHeight; i++)
    //{
    //    memcpy(pDecodedBuffer + frameWidth * i,
    //        pFrame->data[0] + pFrame->linesize[0] * i,
    //        frameWidth);
    //}
    ////拷贝u分量
    //for (j = 0; j < frameHeight / 2; j++)
    //{
    //    memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j,
    //        pFrame->data[1] + pFrame->linesize[1] * j,
    //        frameWidth / 2);
    //}
    //拷贝v分量
    //for (k = 0; k < frameHeight / 2; k++)
    //{
    //    memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j + frameWidth / 2 * k,
    //        pFrame->data[2] + pFrame->linesize[2] * k,
    //        frameWidth / 2);
    //}

    //将buffer中的yuv420p数据转换为RGB;
    //Yuv420p2Rgb32(pDecodedBuffer, img.data, frameWidth, frameHeight);

    static int frameCount = 0;
    printf("frameCount=%d\n", ++frameCount);
    //char buf[128] = { 0 };
    //sprintf(buf, "%.png", frameCount);
    //imwrite(buf, img);
    //简单处理，这里用了canny来进行二值化
    //cvtColor(img, output, COLOR_RGB2GRAY);
    //waitKey(2);
    //Canny(img, output, 50, 50 * 2);
    //waitKey(2);
    //imshow("test", output);
    //waitKey(10);
    // 测试函数
    // imwrite("test.jpg",img);
    //释放buffer
    free(pDecodedBuffer);
    img.release();
    output.release();
}


//int PullRTMPVideoStream()
//{
//    AVOutputFormat* ofmt = NULL;
//    //Input AVFormatContext and Output AVFormatContext
//    AVFormatContext* ifmt_ctx = NULL, * ofmt_ctx = NULL;
//    AVPacket pkt;
//    const char* in_filename, * out_filename;
//    int ret, i;
//    int videoindex = -1;
//    int frame_index = 0;
//    //in_filename = "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov";
//    in_filename = "rtmp://ns8.indexforce.com/home/mystream";
//    out_filename = "receive.flv";
//    av_register_all();
//    //Network
//    avformat_network_init();
//    //Input
//    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
//        printf("Could not open input file.");
//        goto end;
//    }
//    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
//        printf("Failed to retrieve input stream information");
//        goto end;
//    }
//    //？ ctx=context
//    for (i = 0; i < ifmt_ctx->nb_streams; i++)
//        if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//            videoindex = i;
//            break;
//        }
//
//    av_dump_format(ifmt_ctx, 0, in_filename, 0);
//
//    //Output
//    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename); //RTMP
//    if (!ofmt_ctx) {
//        printf("Could not create output context\n");
//        ret = AVERROR_UNKNOWN;
//        goto end;
//    }
//    ofmt = ofmt_ctx->oformat;
//    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
//        //Create output AVStream according to input AVStream
//        AVStream* in_stream = ifmt_ctx->streams[i];
//        //AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
//        AVCodec* codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
//        AVStream* out_stream = avformat_new_stream(ofmt_ctx, codec);
//
//        if (!out_stream) {
//            printf("Failed allocating output stream\n");
//            ret = AVERROR_UNKNOWN;
//            goto end;
//        }
//
//        AVCodecContext* p_codec_ctx = avcodec_alloc_context3(codec);
//        ret = avcodec_parameters_to_context(p_codec_ctx, in_stream->codecpar);
//
//        //Copy the settings of AVCodecContext
//        if (ret < 0) {
//            printf("Failed to copy context from input to output stream codec context\n");
//            goto end;
//        }
//        p_codec_ctx->codec_tag = 0;
//        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
//            p_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//        ret = avcodec_parameters_from_context(out_stream->codecpar, p_codec_ctx);
//        if (ret < 0) {
//            av_log(NULL, AV_LOG_ERROR, "eno:[%d] error to paramters codec paramter \n", ret);
//        }
//    }
//
//    //Dump Format------------------
//    av_dump_format(ofmt_ctx, 0, out_filename, 1);
//    //Open output URL
//    if (!(ofmt->flags & AVFMT_NOFILE)) {
//        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            printf("Could not open output URL '%s'", out_filename);
//            goto end;
//        }
//    }
//    //Write file header
//    ret = avformat_write_header(ofmt_ctx, NULL);
//    if (ret < 0) {
//        printf("Error occurred when opening output URL\n");
//        goto end;
//    }
//    //拉流
//    while (1)
//    {
//        AVStream* in_stream, * out_stream;
//        //Get an AVPacket
//        ret = av_read_frame(ifmt_ctx, &pkt);
//        if (ret < 0)
//            break;
//
//        in_stream = ifmt_ctx->streams[pkt.stream_index];
//        out_stream = ofmt_ctx->streams[pkt.stream_index];
//        /* copy packet */
//        //Convert PTS/DTS
//        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
//        pkt.pos = -1;
//        //Print to Screen
//        if (pkt.stream_index == videoindex) {
//            printf("Receive %8d video frames from input URL\n", frame_index);
//            frame_index++;
//        }
//        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
//        if (ret < 0) {
//            printf("Error muxing packet\n");
//            break;
//        }
//        av_packet_unref(&pkt);
//    }
//
//    //Write file trailer
//    av_write_trailer(ofmt_ctx);
//end:
//    avformat_close_input(&ifmt_ctx);
//    /* close output */
//    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
//        avio_close(ofmt_ctx->pb);
//    avformat_free_context(ofmt_ctx);
//    if (ret < 0 && ret != AVERROR_EOF) {
//        printf("Error occurred.\n");
//        return -1;
//    }
//}

void PullRTMPStreamdWithOpencv()
{
    utils::logging::setLogLevel(utils::logging::LOG_LEVEL_VERBOSE);

    const char* in_filename = "rtmp://ns8.indexforce.com/home/mystream";

    VideoCapture capture;
    bool rtn = capture.open(in_filename, CAP_FFMPEG);
    if (!capture.isOpened())
    {
        printf("can not open ...\n");
        return;
    }
    Mat frame;
    while (capture.read(frame))
    {
        static int count = 0;
        char fileName[256] = { 0 };
        sprintf(fileName, "%d.png", count++);
        imwrite(fileName, frame);
        printf("process %s image\n", fileName);
    }
}

int main(int argc, char** argv)
{
    //PullRTMPStreamAndDecode();

    PullRTMPStreamdWithOpencv();

    return 0;
}