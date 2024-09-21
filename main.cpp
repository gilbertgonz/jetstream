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
    std::string pipeline = gstreamer_pipeline(
        1280, // capture_width
        720,  // capture_height
        1280, // display_width
        720,  // display_height
        30,   // framerate
        0     // flip_method
    );
    std::cout << "Using pipeline: \n\t" << pipeline << "\n";
 
    // Open camera
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    if(!cap.isOpened()) {
        std::cout<<"Failed to open camera."<<std::endl;
        return (-1);
    }

    // Video params
    cv::VideoWriter writer;
    int codec = cv::VideoWriter::fourcc('M', 'P', '4', 'V');
    double fps = 25.0;
    std::string filename = "./live.mp4";
    cv::Size sizeFrame(640,480);
    writer.open(filename, codec, fps, sizeFrame, true);

    cv::Mat img;
    for (int i = 0; i < 150; i++) {
    	if (!cap.read(img)) {
            std::cout<<"Capture read error"<<std::endl;
            break;
        }

        cv::Mat temp_frame;
        resize(img, temp_frame, sizeFrame);
        writer.write(temp_frame);
	
        int keycode = cv::waitKey(1) & 0xff; 
        if (keycode == 27) break;
    }

    cap.release();
    writer.release();
    return 0;
}