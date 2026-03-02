#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Open the default camera (0)
    cv::VideoCapture cap(0);
    
    if(!cap.isOpened()) {
        std::cerr << "Error: Could not open camera." << std::endl;
        return -1;
    }

    cv::Mat frame;
    while(true) {
        cap >> frame; // Capture a new frame
        if(frame.empty()) break;

        cv::imshow("OpenCV Test - Press ESC to Exit", frame);
        
        // Wait for 'ESC' key to exit
        if(cv::waitKey(30) == 27) break;
    }

    return 0;
}