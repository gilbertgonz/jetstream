// reference: https://github.com/JetsonHacksNano/CSI-Camera/blob/master/simple_camera.cpp

#include <opencv2/opencv.hpp>

// Constants
const int MASK_THRESHOLD = 25;
const float PMS_THRESHOLD = 0.35;
const cv::Size BLUR_SIZE(3, 3);
const cv::Size DILATION_SIZE(7, 7);
const bool SAVE_IMAGES = false;

struct Box {
    int x, y, w, h;
    int area;
};

std::string gstreamer_pipeline (int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method) {
    return "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(capture_width) + ", height=(int)" +
           std::to_string(capture_height) + ", framerate=(fraction)" + std::to_string(framerate) +
           "/1 ! nvvidconv flip-method=" + std::to_string(flip_method) + " ! video/x-raw, width=(int)" + std::to_string(display_width) + ", height=(int)" +
           std::to_string(display_height) + ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}

std::vector<Box> detect_motion_clusters(cv::Mat& gray_prev_frame, cv::Mat& gray_frame) {
    cv::Mat diff_frame, thresh_frame, dilated_frame;

    // Compute abs diff and apply threshold
    cv::absdiff(gray_prev_frame, gray_frame, diff_frame);
    cv::threshold(diff_frame, thresh_frame, MASK_THRESHOLD, 255, cv::THRESH_BINARY);

    // Dilate thresholded image
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, DILATION_SIZE);
    cv::dilate(thresh_frame, dilated_frame, element);

    // Find connected components (clusters)
    cv::Mat labels, stats, centroids;
    int num_labels = cv::connectedComponentsWithStats(dilated_frame, labels, stats, centroids, 8, CV_32S);

    std::vector<Box> bounding_boxes;
    for (int i = 1; i < num_labels; i++) {
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        Box box{
            stats.at<int>(i, cv::CC_STAT_LEFT),
            stats.at<int>(i, cv::CC_STAT_TOP),
            stats.at<int>(i, cv::CC_STAT_WIDTH),
            stats.at<int>(i, cv::CC_STAT_HEIGHT),
            area
        };
        bounding_boxes.push_back(box);
    }
    return bounding_boxes;
}

// reference: https://medium.com/@itberrios6/introduction-to-motion-detection-part-1-e031b0bb9bb2
void non_max_suppression(std::vector<Box>& boxes, float thresh) {
    // Remove bboxes that have high Intersection Over Union (IoU)
    for (size_t i = 0; i < boxes.size(); i++) {
        for (size_t j = i + 1; j < boxes.size(); ) {
            float intersection_width = std::max(0, std::min(boxes[i].w + boxes[i].x, boxes[j].w + boxes[j].x) - std::max(boxes[i].x, boxes[j].x));
            float intersection_height = std::max(0, std::min(boxes[i].h + boxes[i].y, boxes[j].w + boxes[j].y) - std::max(boxes[i].y, boxes[j].y));
            float intersection = intersection_width * intersection_height;
            float total_union = (boxes[i].area + boxes[j].area) - intersection;

            float iou = 0.0f;
            if (total_union > 0) {
                iou = intersection / total_union;
            }

            // Check IoU threshold
            if (iou > thresh) {
                if (boxes[i].area > boxes[j].area) {
                    boxes.erase(boxes.begin() + j);
                } else {
                    boxes.erase(boxes.begin() + i);
                    break; // Break since box i was removed
                }
            } else {
                j++; // Only increment j if no box was removed
            }
        }
    }
}

int main() {
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

    cv::Mat gray_prev_frame, gray_frame;

    int frame_count = 0;
    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        // Convert current frame to grayscale and blur it
        cv::cvtColor(frame, gray_frame, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray_frame, gray_frame, BLUR_SIZE, 0);

        if (!gray_prev_frame.empty()) {
            // Compute contours
            std::vector<Box> boxes = detect_motion_clusters(gray_prev_frame, gray_frame);

            // Apply non-maximal suppression to reduce noisy bboxs
            non_max_suppression(boxes, PMS_THRESHOLD);

            cv::Rect bbox;
            for (const auto& box : boxes) {
                cv::Rect bbox(box.x, box.y, box.w, box.h);
                // Draw bbox
                cv::rectangle(frame, bbox, cv::Scalar(0, 255, 0), 2);
            }

            // cv::Mat save_img = blurred_frame;
            // cv::resize(blurred_frame, save_img, cv::Size(d_w, d_h));
            // writer.write(save_img);

            // Show
            cv::imshow("Result", frame);
            cv::waitKey(1);

            if (SAVE_IMAGES) {
                std::stringstream filename;
                filename << "/imgs/frame_" << std::setw(4) << std::setfill('0') << frame_count << ".jpg";
                cv::imwrite(filename.str(), frame);
                frame_count++;
            }
        }

        // Update prev_frame
        gray_prev_frame = gray_frame.clone();
    }

    cap.release();
    cv::destroyAllWindows();

    return 0;
}
