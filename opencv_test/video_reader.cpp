#include <iostream>

#include "opencv2/opencv_modules.hpp"

#define HAVE_OPENCV_CUDACODEC

#if defined(HAVE_OPENCV_CUDACODEC)

#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <opencv2/core.hpp>
#include <opencv2/core/opengl.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/opencv_modules.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>


#include <unistd.h>

int main(int argc, const char* argv[])
{
    std::cout << "pid=" << getpid();
    //const std::string fname("rtmp://ns8.indexforce.com/home/mystream");
    //cv::VideoCapture capture;
    //cv::Mat frame;
    //frame = capture.open(fname, cv::CAP_FFMPEG);
    //if (!capture.isOpened())
    //{
    //    printf("can not open ...\n");
    //    return -1;
    //}

    //while (capture.read(frame))
    //{
    //    static int count = 0;
    //    char fileName[256] = { 0 };
    //    sprintf(fileName, "%d.png", count++);
    //    imwrite(fileName, frame);
    //    printf("process %s image\n", fileName);
    //}
    //return 0;

    const std::string fname("/home/ivs_dev/yingshi/1.mp4");

    //cv::namedWindow("CPU", cv::WINDOW_NORMAL);
    //cv::namedWindow("GPU", cv::WINDOW_OPENGL);
    //cv::cuda::setGlDevice();
    cv::cuda::setDevice(0);
    int deviceCount = cv::cuda::getCudaEnabledDeviceCount();
    //cv::Mat frame;
    //cv::VideoCapture reader(fname);

    cv::cuda::GpuMat d_frame;
    cv::Ptr<cv::cudacodec::VideoReader> d_reader = cv::cudacodec::createVideoReader(fname);
    auto videoFormat = d_reader->format();

    cv::TickMeter tm;
    std::vector<double> cpu_times;
    std::vector<double> gpu_times;

    int gpu_frame_count=0, cpu_frame_count=0;

    //cv::cuda::cvtColor(gpu_frame, gpu_current, COLOR_BGR2GRAY);

    //for (;;)
    //{
    //    tm.reset(); tm.start();
    //    if (!reader.read(frame))
    //        break;
    //    tm.stop();
    //    cpu_times.push_back(tm.getTimeMilli());
    //    cpu_frame_count++;

    //    //cv::imshow("CPU", frame);

    //    //if (cv::waitKey(3) > 0)
    //    //    break;
    //}

    for (;;)
    {
        tm.reset(); tm.start();
        if (!d_reader->nextFrame(d_frame))
            break;
        tm.stop();
        gpu_times.push_back(tm.getTimeMilli());
        gpu_frame_count++;
        printf("frame=%d\n", gpu_frame_count);

        cv::cuda::GpuMat rgbFrame;
        cv::cuda::cvtColor(d_frame, rgbFrame, cv::COLOR_YUV2RGB);

        cv::Mat frame2;
        d_frame.download(frame2);
        //char buf[128] = { 0 };
        //sprintf(buf, "%d.png", gpu_frame_count);
        //cv::imwrite(buf, rgbFrame);

        //cv::imshow("GPU", d_frame);

        //if (cv::waitKey(3) > 0)
        //    break;
    }

    if (!cpu_times.empty() && !gpu_times.empty())
    {
        std::cout << std::endl << "Results:" << std::endl;

        std::sort(cpu_times.begin(), cpu_times.end());
        std::sort(gpu_times.begin(), gpu_times.end());

        double cpu_avg = std::accumulate(cpu_times.begin(), cpu_times.end(), 0.0) / cpu_times.size();
        double gpu_avg = std::accumulate(gpu_times.begin(), gpu_times.end(), 0.0) / gpu_times.size();

        std::cout << "CPU : Avg : " << cpu_avg << " ms FPS : " << 1000.0 / cpu_avg << " Frames " << cpu_frame_count << std::endl;
        std::cout << "GPU : Avg : " << gpu_avg << " ms FPS : " << 1000.0 / gpu_avg << " Frames " << gpu_frame_count << std::endl;
    }

    return 0;
}

#else

int main()
{
    std::cout << "OpenCV was built without CUDA Video decoding support\n" << std::endl;
    return 0;
}

#endif
