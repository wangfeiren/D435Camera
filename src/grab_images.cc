#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <sys/stat.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <librealsense2/rs.hpp>

#include "convert.hpp"

using namespace std;

char* GetDateTime()
{
	std::time_t result = std::time(nullptr);
	char* tt = std::asctime(std::localtime(&result)) + 4;
	tt[9] = '-';
	tt[12] = '-';
	tt[20] = NULL;
	for (int i = 0; i < 20; i++) {
		if (tt[i] == ' ')
			tt[i] = '_';
	}
	return tt;
}

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}


int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "usage: ./grab_images dataset_path!" << endl;
    }

    string path = string(argv[1]) + "/" + string(GetDateTime());
    string path_color = path + "/color/";
    string path_depth = path + "/depth/";

    if (mkdir(path.c_str(), S_IRWXU) == -1) {
		std::cout << "make dir faile" << path << std::endl;
		return -1;
	}

	if (mkdir(path_color.c_str(), S_IRWXU) < 0) {
		std::cout << "make dir left faile" << std::endl;
		return -1;
	}

	if (mkdir(path_depth.c_str(), S_IRWXU) < 0) {
		std::cout << "make dir right faile" << std::endl;
		return -1;
	}

    using namespace cv;
    const auto window_name = "Display Image";
    namedWindow(window_name, WINDOW_AUTOSIZE);

    // Create a Pipeline - this serves as a top-level API for streaming and processing frames
    rs2::pipeline p;

    rs2::colorizer color_map;

    // Configure and start the pipeline
    p.start();

    while (true)
    {
        // Block program until frames arrive
        rs2::frameset frames = p.wait_for_frames();

        // Try to get a frame of a depth image
        rs2::frame depth = frames.get_depth_frame().apply_filter(color_map);
        rs2::frame color = frames.get_color_frame();

        unsigned long long frame_number = frames.get_frame_number();
        cout << "Camera get frame " << frame_number << "!" << endl;
        // rs2::video_frame color = frames.get_color_frame();
        cv::Mat depth_image = frame_to_mat(depth);
        cv::Mat color_image = frame_to_mat(color);
        stringstream ss_color;
        stringstream ss_depth;
        ss_color << path_color << "/" << setw(6) << setfill('0') << frame_number << ".png";
        ss_depth << path_depth << "/" << setw(6) << setfill('0') << frame_number << ".png";

        string color_file_name = ss_color.str();
        string depth_file_name = ss_depth.str();
        cv::imwrite(color_file_name.c_str(), color_image);
        cv::imwrite(depth_file_name.c_str(), depth_image);
        // Get the depth frame's dimensions
        // const int depth_width = depth.as<rs2::video_frame>().get_width();
        // const int depth_height = depth.as<rs2::video_frame>().get_height();

        //  Mat depth_image(Size(depth_width, depth_height), CV_8UC3, (void*)depth.get_data(), Mat::AUTO_STEP);

        // cv::imshow(window_name, color_image);
        // cv::waitKey(1);
        if (kbhit()) {
            char k = getchar();
            if (k == 'k') {
                break;
            }
        }
    }
    return 1;
}
