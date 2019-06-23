#include "litpression.hpp"
#include <getopt.h>
#include <opencv2/opencv.hpp>
// #include <opencv2/videoio/videoio_c.h>
#include <string>
#include <utility>
#include <vector>

using std::string;
using std::vector;

std::unique_ptr<litpression::Litpression> lit;
cv::Ptr<cv::DenseOpticalFlow> flow_alg;

cv::Mat3b in_frame;
cv::Mat3b out_frame;

string out_path = "";
cv::VideoWriter writer;
auto write_four_cc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
const int WRITE_FPS = 5;

const char WINDOW_NAME[] = "litpression";
int frame_i = 1;

cv::Ptr<cv::DenseOpticalFlow> init_flow_alg(const string& flow_name) {
    if (flow_name == "dis") {
        return cv::DISOpticalFlow::create(cv::DISOpticalFlow::PRESET_MEDIUM);
    } else if (flow_name == "farneback") {
        // custom presets to try to make Farneback faster
        return cv::FarnebackOpticalFlow::create(5, 0.5, true, 13, 7, 7, 1.5);
    } else if (flow_name == "deep") {
        return cv::optflow::createOptFlow_DeepFlow();
    } else if (flow_name == "dualtvl1") {
        return cv::optflow::createOptFlow_DualTVL1();
    } else if (flow_name == "simple") {
        return cv::optflow::createOptFlow_SimpleFlow();
    } else {
        std::cerr << "Unknown flow algorithm: \"" << flow_name << "\"\n";
        exit(EXIT_FAILURE);
    }
}

void run_webcam()
{
    // init webcam reader
    auto cap = cv::VideoCapture(0);
    if (!cap.isOpened()) {
        cap.open(cv::CAP_ANY);
        if (!cap.isOpened()) {
            std::cerr << "Failed to open webcam" << std::endl;
            exit(1);
        }
    }

    while (true) {
        // read frame
        if (!cap.read(in_frame)) {
            break;
        }

        // init video writer to optional output file on first iteration
        // (once we know frame size)
        if (!out_path.empty() && !writer.isOpened()) {
            writer = cv::VideoWriter(out_path, write_four_cc, WRITE_FPS, in_frame.size());
        }

        // apply process
        out_frame = lit->process(in_frame);

        // write processed frame to optional output file
        if (writer.isOpened()) {
            writer.write(out_frame);
        }

        // show processed frame
        cv::imshow(WINDOW_NAME, out_frame);

        char key = cv::waitKey(1) & 0xFF;
        if (key == 'q') {
            break;
        }
    }
}

// run algorithm on image sequence
void run_seq(const string& path_format)
{
    while (true) {
        // read frame
        char path[1024];
        snprintf(path, 1024, path_format.c_str(), frame_i);
        in_frame = cv::imread(path);
        if (in_frame.data == nullptr) {
            break;
        }

        // init video writer to optional output file on first iteration
        // (once we know frame size)
        if (!out_path.empty() && !writer.isOpened()) {
            writer = cv::VideoWriter(out_path, write_four_cc, WRITE_FPS, in_frame.size());
        }

        // apply process
        out_frame = lit->process(in_frame);

        // write processed frame to optional output file
        if (writer.isOpened()) {
            writer.write(out_frame);
        }

        // show processed frame
        cv::imshow(WINDOW_NAME, out_frame);

        char key = cv::waitKey(1) & 0xFF;
        if (key == 'q') {
            break;
        }

        frame_i++;
    }
}

// run algorithm on video
void run_video(const string& in_path)
{
    // init video file reader
    cv::VideoCapture cap = cv::VideoCapture(in_path);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open video at path: " << in_path << std::endl;
        exit(1);
    }

    while (cap.isOpened()) {
        // read color frame
        if (!cap.read(in_frame)) {
            break;
        }

        // init video writer to optional output file on first iteration
        // (once we know frame size)
        if (!out_path.empty() && !writer.isOpened()) {
            writer = cv::VideoWriter(out_path, write_four_cc, WRITE_FPS, in_frame.size());
        }

        // apply process
        out_frame = lit->process(in_frame);

        // write processed frame to optional output file
        if (writer.isOpened()) {
            writer.write(out_frame);
        }

        // show processed frame
        cv::imshow(WINDOW_NAME, out_frame);

        char key = cv::waitKey(1) & 0xFF;
        if (key == 'q') {
            break;
        }
    }
}


void usage(const char* exec_name)
{
    std::cerr << "Usage: " << exec_name << " [options] (webcam | <path_to_video>)\n";
    std::cerr << "Options:\n";
    std::cerr << "  -f <name>\t\tSelect flow algorithm (dis, farneback, deep, dualtvl1, simple)\n";
    std::cerr << "  -o <path.mp4>\t\tWrite rendered output to mp4 file\n";
}

bool ends_with(string const& value, string const& ending)
{
    if (ending.size() > value.size()) {
        return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


int main(int argc, char* argv[])
{
    string flow_name = "dis";

    char opt;
    while ((opt = getopt(argc, argv, "f:o:")) != -1) {
        switch (opt) {
        case 'f':
            flow_name = string(optarg);
            break;

        case 'o':
            out_path = string(optarg);
            if (!ends_with(out_path, ".mp4")) {
                std::cerr << "Output file must be mp4\n";
                exit(EXIT_FAILURE);
            }
            break;

        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (argc - optind != 1) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    flow_alg = init_flow_alg(flow_name);
    lit = std::make_unique<litpression::Litpression>(flow_alg);

    string arg = string(argv[optind]);
    cv::namedWindow(WINDOW_NAME, cv::WINDOW_NORMAL);

    if (arg == "webcam") {
        run_webcam();
    } else {
        string path = argv[optind];
        if (ends_with(path, ".png")) {
            run_seq(path);
        } else {
            run_video(path);
        }
    }

    return 0;
}
