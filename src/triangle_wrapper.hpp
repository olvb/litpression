#pragma once

#include <tuple>
#include <vector>

namespace triangle {

std::vector<double> add_points(std::vector<double>& points_xy, double max_area = 0.0);
std::vector<int> list_edges(std::vector<double>& points_xy);
// std::vector<int> list_neighbors(std::vector<double>& points_xy);

// std::tuple<std::vector<double>, std::vector<int>> triangulate(std::vector<double>& points_xys, double max_area);
}
