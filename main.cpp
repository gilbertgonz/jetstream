// reference: https://github.com/JetsonHacksNano/CSI-Camera/blob/master/simple_camera.cpp

#include <opencv2/opencv.hpp>

std::string gstreamer_pipeline (int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method) {
    return "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(capture_width) + ", height=(int)" +
           std::to_string(capture_height) + ", framerate=(fraction)" + std::to_string(framerate) +
           "/1 ! nvvidconv flip-method=" + std::to_string(flip_method) + " ! video/x-raw, width=(int)" + std::to_string(display_width) + ", height=(int)" +
           std::to_string(display_height) + ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}

int main()
{
    int w = 1920, h = 1080;
    int d_w = 1280, d_h = 720;
    float fps = 30.0;
    std::string pipeline = gstreamer_pipeline(
        w,    // capture_width
        h,    // capture_height
        d_w,  // display_width
        d_h,  // display_height
        fps,  // framerate
        0     // flip_method
    );
 
    // Open camera
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    if(!cap.isOpened()) {
        std::cout<<"Failed to open camera."<<std::endl;
        return (-1);
    }

    // Video params
    cv::VideoWriter writer;
    int codec = cv::VideoWriter::fourcc('M', 'P', '4', 'V');
    std::string filename = "./live.mp4";
    writer.open(filename, codec, fps, cv::Size(d_w, d_h), true);

    // Create CUDA Gaussian filter
    cv::Ptr<cv::cuda::Filter> gaussianFilter = cv::cuda::createGaussianFilter(CV_8UC3, CV_8UC3, cv::Size(11, 11), 0);

    cv::Mat frame, blurred_frame;
    cv::cuda::GpuMat d_frame, d_blurred_frame;

    cv::Mat img;
    for (int i = 0; i < 100; i++) {
    	if (!cap.read(img)) {
            std::cout<<"Capture read error"<<std::endl;
            break;
        }

        // Upload frame to GPU
        d_frame.upload(img);

        // Apply Gaussian blur using CUDA
        gaussianFilter->apply(d_frame, d_blurred_frame);

        // Download processed frame from GPU
        d_blurred_frame.download(blurred_frame);


        cv::Mat save_img = blurred_frame;
        resize(blurred_frame, save_img, cv::Size(d_w, d_h));
        writer.write(save_img);
	
        int keycode = cv::waitKey(1) & 0xff; 
        if (keycode == 27) break;
    }

    cap.release();
    writer.release();
    return 0;
}