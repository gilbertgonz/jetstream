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
    int w = 3264, h =1848;
    float fps = 30;
    std::string pipeline = gstreamer_pipeline(
        w, // capture_width
        h,  // capture_height
        1280, // display_width
        720,  // display_height
        fps,   // framerate
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
    writer.open(filename, codec, fps, cv::Size(w, h), true);

    cv::Mat img;
    while (true) {
    	if (!cap.read(img)) {
            std::cout<<"Capture read error"<<std::endl;
            break;
        }

        writer.write(img);
	
        int keycode = cv::waitKey(1) & 0xff; 
        if (keycode == 27) break;
    }

    cap.release();
    writer.release();
    return 0;
}