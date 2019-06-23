#include "litpression.hpp"
#include "triangle_wrapper.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <random>

namespace litpression {

using std::vector;

cv::Mat3b Litpression::process(const cv::Mat3b& color)
{
    this->color = color.clone();

    if (first_frame) {
        width = color.size().width;
        height = color.size().height;
        corners = {
            cv::Point2f(0, 0),
            cv::Point2f(width - 1.0f, 0.0f),
            cv::Point2f(0.0f, height - 1.0f),
            cv::Point2f(width - 1.0f, height - 1.0f)
        };
        flow = cv::Mat::zeros(height, width, CV_32FC2);
    }
    // gray needed for contours and optical flow
    cv::cvtColor(color, gray, cv::COLOR_BGR2GRAY);

    if (settings.clip_thresh > 0) {
        compute_contours();
    }

    if (first_frame) {
        gen_initial_strokes();
    } else {
        flow_alg->calc(gray_prev, gray, flow);
        move_strokes();
        del_strokes_too_close();
        gen_new_strokes();
    }

    if (settings.gradient_orientation) {
        orient_strokes_with_gradients();
    }

    clip_strokes();
    draw_strokes();

    gray_prev = gray.clone();
    first_frame = false;

    return out;
}

void Litpression::compute_contours()
{
    // int blur_size = 11;
    // cv::Mat blur, grad_x, grad_y, abs_grad_x, abs_grad_y;
    // cv::GaussianBlur(src, blur, cv::Size(blur_size, blur_size), 0, 0);
    cv::Laplacian(gray, contours, CV_32F);
}

void Litpression::gen_initial_strokes()
{
    assert(strokes.empty());

    auto centers = triangulate_add();

    std::shuffle(centers.begin(), centers.end(), rng);

    for (const auto& c : centers) {
        auto s = gen_stroke(c);
        strokes.push_back(s);
    }

    // for (int cx = 0; cx < width; cx += STROKE_SPACING) {
    //     for (int cy = 0; cy < width; cy += STROKE_SPACING) {
    //         stroke_centers.emplace_back(cx, cy);
    //         auto sp = gen_stroke_param();
    //         stroke_params.push_back(sp);
    //     }
    // }
}

vector<cv::Point2f> Litpression::triangulate_add()
{
    vector<double> points_xys;
    points_xys.reserve(strokes.size() * 2);
    for (const auto& s : strokes) {
        points_xys.push_back(s.center.x);
        points_xys.push_back(s.center.y);
    }

    // always add corners to be sure that triangles extend to edge of img
    for (const auto& c : corners) {
        points_xys.push_back(c.x);
        points_xys.push_back(c.y);
    }

    auto points_xys_new = triangle::add_points(points_xys, (double) settings.max_triangle_area);

    // Triangle will append coordinates of new points at the end
    vector<cv::Point2f> centers_new;
    centers_new.reserve(points_xys_new.size() - points_xys.size());

    float area_sqrt = std::sqrt((float) settings.max_triangle_area);
    std::uniform_real_distribution<float> distr(-area_sqrt, area_sqrt);

    for (size_t i = points_xys.size(); i < points_xys_new.size(); i += 2) {
        // add random perturbation to avoid patterns/lines created by triangulation
        float cx = points_xys_new[i]; // + distr(rng);
        float cy = points_xys_new[i + 1]; // + distr(rng);

        cx = std::max(0.0f, std::min(width - 1.0f, cx));
        cy = std::max(0.0f, std::min(height - 1.0f, cy));
        // assert(cx >= 0.0f && cx < width && cy >= 0.0f && cy < height);

        cv::Point2f c = cv::Point2f(cx, cy);
        centers_new.push_back(c);
    }

    return centers_new;
}

Stroke Litpression::gen_stroke(const cv::Point2f& center)
{
    std::uniform_int_distribution<int> length_distr(settings.min_length, settings.max_length);
    std::uniform_int_distribution<int> radius_distr(settings.min_radius, settings.max_radius);
    std::uniform_int_distribution<int> theta_delta_distr(settings.min_theta_delta, settings.max_theta_delta);
    std::uniform_int_distribution<int> rgb_delta_distr(settings.min_rgb_delta, settings.max_rgb_delta);

    int length = length_distr(rng);
    int radius = radius_distr(rng);
    int theta_delta = theta_delta_distr(rng);

    int r_delta = rgb_delta_distr(rng);
    int g_delta = rgb_delta_distr(rng);
    int b_delta = rgb_delta_distr(rng);

    auto stroke = Stroke(center, length, radius, settings.theta, theta_delta, r_delta, g_delta, b_delta);

    stroke.center_int = { (int) std::round(center.x), (int) std::round(center.y) };
    return stroke;
}

void Litpression::move_strokes()
{
    vector<size_t> idxs_strokes_to_del;

    for (size_t i = 0; i < strokes.size(); i++) {
        // NB: mutable reference!
        auto& s = strokes[i];

        auto dxy = flow(s.center.y, s.center.x);
        s.center.x += dxy[0];
        s.center.y += dxy[1];
        s.center_int.x = (int) std::round(s.center.x);
        s.center_int.y = (int) std::round(s.center.y);

        // delete stroke if center out of bounds
        if (s.center_int.x < 0 || s.center_int.x > width - 1 || s.center_int.y < 0 || s.center_int.y > height - 1) {
            idxs_strokes_to_del.push_back(i);
        }
    }

    del_strokes(idxs_strokes_to_del);
}

void Litpression::del_strokes(vector<size_t>& idxs_strokes_to_del)
{
    if (idxs_strokes_to_del.empty()) {
        return;
    }

    vector<Stroke> strokes_new;
    for (size_t i = 0; i < strokes.size(); i++) {
        auto it = std::find(idxs_strokes_to_del.begin(), idxs_strokes_to_del.end(), i);
        if (it == idxs_strokes_to_del.end()) {
            strokes_new.push_back(strokes[i]);
        } else {
            idxs_strokes_to_del.erase(it);
        }
    }

    assert(idxs_strokes_to_del.empty());
    strokes = strokes_new;
}

void Litpression::del_strokes_too_close()
{
    vector<double> centers_xys;
    centers_xys.reserve(strokes.size() * 2);
    for (const auto& s : strokes) {
        centers_xys.push_back(s.center.x);
        centers_xys.push_back(s.center.y);
    }

    vector<int> edges_idxs = triangle::list_edges(centers_xys);

    vector<size_t> idxs_strokes_to_del;
    for (size_t i = 0; i < edges_idxs.size(); i += 2) {
        size_t i1 = (size_t) edges_idxs[i];
        size_t i2 = (size_t) edges_idxs[i + 1];

        bool edge_already_del = false;
        for (auto i : idxs_strokes_to_del) {
            if (i == i1 || i == i2) {
                edge_already_del = true;
                break;
            }
        }

        if (edge_already_del) {
            continue;
        }

        float cx1 = centers_xys[i1 * 2];
        float cy1 = centers_xys[i1 * 2 + 1];
        float cx2 = centers_xys[i2 * 2];
        float cy2 = centers_xys[i2 * 2 + 1];
        float dist_sq = (cx1 - cx2) * (cx1 - cx2) + (cy1 - cy2) * (cy1 - cy2);

        if (dist_sq < settings.min_dist_sq) {
            // remove deepest-layered stroke
            if (i1 < i2) {
                idxs_strokes_to_del.push_back(i1);
            } else {
                idxs_strokes_to_del.push_back(i2);
            }
        }
    }

    // NB: edges closer than STROKE_MIN_DIST may remain
    // hopefully they will be deleted at next round

    del_strokes(idxs_strokes_to_del);
}

void Litpression::gen_new_strokes()
{
    auto new_stroke_centers = triangulate_add();
    std::shuffle(new_stroke_centers.begin(), new_stroke_centers.end(), rng);

    vector<Stroke> new_strokes;
    new_strokes.reserve(new_stroke_centers.size());
    for (const auto& c : new_stroke_centers) {
        auto s = gen_stroke(c);
        new_strokes.push_back(s);
    }

    // TODO insert new strokes at random positions among existing strokes
    // for now we insert them at start so they are rendered first
    // thus at a deeper layer, so we have less noise
    strokes.insert(strokes.begin(), new_strokes.begin(), new_strokes.end());
}

void Litpression::clip_strokes()
{
    for (auto& s : strokes) {
        float theta = s.theta + s.theta_delta;
        float theta_cos = std::cos(theta);
        float theta_sin = std::sin(theta);
        float length_half = (float) s.length / 2.0f;
        float start_x = s.center.x - length_half * theta_cos;
        float start_y = s.center.y - length_half * theta_sin;
        float end_x = s.center.x + length_half * theta_cos;
        float end_y = s.center.y + length_half * theta_sin;

        if (settings.clip_thresh > 0) {
            // get clipped ends
            s.start = clip_stroke_half(s.center.x, s.center.y, start_x, start_y);
            s.end = clip_stroke_half(s.center.x, s.center.y, end_x, end_y);
        } else {
            // clamp ends to bounds
            s.start.x = std::max(0, std::min(width - 1, (int) std::round(start_x)));
            s.start.y = std::max(0, std::min(height - 1, (int) std::round(start_y)));
            s.end.x = std::max(0, std::min(width - 1, (int) std::round(end_x)));
            s.end.y = std::max(0, std::min(height - 1, (int) std::round(end_y)));
        }
    }
}

// TODO use interpolation for low magnitudes instead of blur
void Litpression::orient_strokes_with_gradients()
{
    // int blur_size = 7;
    // cv::Mat blur;
    // cv::GaussianBlur(gray, blur, cv::Size(blur_size, blur_size), 0, 0);
    cv::Mat grad_x, grad_y;
    cv::Scharr(gray, grad_x, CV_32F, 1, 0);
    cv::Scharr(gray, grad_y, CV_32F, 0, 1);
    // cv::medianBlur(grad_x, grad_x, 7);
    // cv::medianBlur(grad_y, grad_y, 7);
    cv::Mat1f mags, angles;
    cv::cartToPolar(grad_x, grad_y, mags, angles);


    // for (int i = 0; i < height * width; i++) {
    //     if (grad_x(i) < settings.orientation_mag_thresh) {
    //         // s.theta = angles(s.center_int.y, s.center_int.x) + 1.570; // pi/2
    //         grad_x(i) = 0;
    //     }
    // }

    for (auto& s : strokes) {
        if (mags(s.center_int.y, s.center_int.x) > settings.orientation_mag_thresh) {
            s.theta = angles(s.center_int.y, s.center_int.x) + 1.570; // pi/2
        }
    }
}

void Litpression::draw_strokes()
{
    cv::medianBlur(color, color, 5);
    if (settings.fill_background) {
        out = color.clone();
    } else {
        out = cv::Mat::zeros(height, width, CV_8UC1);
    }

    for (const auto& s : strokes) {
        cv::Vec3b color_val = color(s.center_int.y, s.center_int.x);
        // if (s.radius < 1) {
        //     continue;
        // }
        color_val[0] = std::max(0, std::min(255, color_val[0] + s.color_delta[0]));
        color_val[1] = std::max(0, std::min(255, color_val[1] + s.color_delta[1]));
        color_val[2] = std::max(0, std::min(255, color_val[2] + s.color_delta[2]));

        cv::line(out, s.start, s.end, color_val, s.radius);
    }
}

cv::Point2f Litpression::clip_stroke_half(int cx, int cy, float x, float y)
{
    float dx = cx - x;
    float dy = cy - y;
    if (dx == 0 and dy == 0) {
        return cv::Point2f(cx, cy);
    }

    int nb_steps = int(std::ceil(std::max(std::abs(dx), std::abs(dy))));

    float x_step = dx / nb_steps;
    float y_step = dy / nb_steps;

    x = cx;
    y = cy;
    uint8_t last_sample = contours(cy, cx);

    for (int i = 0; i < nb_steps; i++) {
        float tmp_x = x + x_step;
        float tmp_y = y + y_step;

        int tmp_x_int = (int) std::round(tmp_x);
        int tmp_y_int = (int) std::round(tmp_y);

        if (tmp_x_int < 0 || tmp_x_int > width - 1 || tmp_y_int < 0 || tmp_y_int > height - 1) {
            break;
        }

        uint8_t sample = contours(tmp_y_int, tmp_x_int);
        if (sample < last_sample && (last_sample - sample) > settings.clip_thresh) {
            break;
        }

        x = tmp_x;
        y = tmp_y;
        last_sample = sample;
    }

    // clamp to bounds
    x = std::max(0.0f, std::min(width - 1.0f, x));
    y = std::max(0.0f, std::min(height - 1.0f, y));
    return cv::Point2f(x, y);
}

};
