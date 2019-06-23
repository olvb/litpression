#pragma once

#include <memory>
#include <opencv2/opencv.hpp>
#include <opencv2/optflow.hpp>
#include <random>
#include <vector>

namespace litpression {

struct Settings
{
    // stroke length range (before clip)
    int min_length = 3;
    int max_length = 30;
    // stroke radius range
    int min_radius = 3;
    int max_radius = 9;
    // default stroke orientation if gradient orientation is disabled
    double theta = 2.36; // 135 degree
    // orientation randomization ranges
    double min_theta_delta = 0.05; // 3 degrees
    double max_theta_delta = 1.05; // 60 degrees
    // color randomization range
    int min_rgb_delta = -5;
    int max_rgb_delta = 5;
    // threshold of stroke cliping when comparing contours values
    // set to 0 to disable clipping
    double clip_thresh = 200;
    // fill background with blurred version of image,
    // to hide black patches in areas lacking strokes
    bool fill_background = true;
    // enable orientation of strokes with gradient
    // (otherwise it will set randomly)
    bool gradient_orientation = true;
    double orientation_mag_thresh = 50;

    // maximum area of triangles when adding triangles to fill holes and repopulate strokes
    // (chose in relation with stroke radiuses and maybe stroke lengths)
    int max_triangle_area = 36;
    // strokes closer than this distance will be deleted to avoid overdensity
    // (chose in relation with stroke_area)
    int min_dist_sq = 30;
};

struct Stroke
{
    cv::Point2f center;
    int length;
    int radius;
    double theta;
    double theta_delta;
    cv::Vec3i color_delta;

    cv::Point2i center_int;
    cv::Point2i start, end;

    Stroke(const cv::Point2f& center, int length, int radius, double theta, double theta_delta, int r_delta, int g_delta, int b_delta)
        : center(center),
          length(length),
          radius(radius),
          theta(theta),
          theta_delta(theta_delta),
          color_delta(r_delta, g_delta, b_delta) {}
};

class Litpression
{
public:
    Settings settings;
    cv::Ptr<cv::DenseOpticalFlow> flow_alg;

    Litpression(cv::Ptr<cv::DenseOpticalFlow> flow_alg) : flow_alg(flow_alg) {}
    cv::Mat3b process(const cv::Mat3b& color);

private:
    bool first_frame = true;
    int height = 0;
    int width = 0;
    std::vector<cv::Point2f> corners;

    cv::Mat3b color;
    cv::Mat1b gray;
    cv::Mat1b gray_prev;

    cv::Mat1f contours;
    cv::Mat2f flow;
    cv::Mat3b out;

    std::vector<Stroke> strokes;

    std::mt19937 rng;

    void compute_contours();
    void gen_initial_strokes();
    std::vector<cv::Point2f> triangulate_add();
    Stroke gen_stroke(const cv::Point2f& center);
    void move_strokes();
    void orient_strokes_with_gradients();
    void del_strokes(std::vector<size_t>& idxs_strokes_to_del);
    void gen_new_strokes();
    void del_strokes_too_close();
    void clip_strokes();
    void draw_strokes();
    cv::Point2f clip_stroke_half(int cx, int cy, float x, float y);
};

};
