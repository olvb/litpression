#include "triangle_wrapper.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <type_traits>

#define VOID int
#define REAL double
#define NO_TIMER
#define ANSI_DECLARATORS
#include "triangle.h"
#undef VOID
#undef REAL
#undef NO_TIMER
#undef ANSI_DECLARATORS

namespace triangle {

using std::vector;

vector<double> add_points(vector<double>& points_xy, double max_area)
{
    if (max_area < 0 || max_area > 100) {
        std::cerr << "max_area must be in [0, 100] range" << std::endl;
        exit(1);
    }

    // z: zero-indexed
    // Q: quiet
    // B: no boundary markers
    // a: max triangle area
    char tri_switches[100] = "";
    sprintf(tri_switches, "zQBa%0.4f", max_area);

    struct triangulateio in = {};
    in.numberofpoints = points_xy.size() / 2;
    in.numberofpointattributes = 0;
    in.pointlist = points_xy.data();

    struct triangulateio out = {};
    triangulate(tri_switches, &in, &out, NULL);

    static_assert(std::is_same<decltype(out.pointlist), double*>::value, "types do not match");
    vector<double> points_xy_out;
    points_xy_out.resize(out.numberofpoints * 2);
    std::memcpy(points_xy_out.data(), out.pointlist, sizeof(double) * out.numberofpoints * 2);

    free(out.pointlist);
    free(out.pointattributelist);
    free(out.pointmarkerlist);
    free(out.trianglelist);
    free(out.triangleattributelist);
    free(out.neighborlist);
    free(out.segmentlist);
    free(out.segmentmarkerlist);
    free(out.edgelist);
    free(out.edgemarkerlist);

    return points_xy_out;
}

vector<int> list_edges(vector<double>& points_xy)
{
    // z: zero-indexed
    // Q: quiet
    // B: no boundary markers
    // e: provide list of edges
    char tri_switches[] = "zQBe";

    struct triangulateio in = {};
    in.numberofpoints = points_xy.size() / 2;
    in.numberofpointattributes = 0;
    in.pointlist = points_xy.data();

    struct triangulateio out = {};
    triangulate(tri_switches, &in, &out, NULL);

    static_assert(std::is_same<decltype(out.edgelist), int*>::value, "types do not match");
    vector<int> edges_idxs;
    edges_idxs.resize(out.numberofedges * 2);
    std::memcpy(edges_idxs.data(), out.edgelist, sizeof(int) * out.numberofedges * 2);

    free(out.pointlist);
    free(out.pointattributelist);
    free(out.pointmarkerlist);
    free(out.trianglelist);
    free(out.triangleattributelist);
    free(out.neighborlist);
    free(out.segmentlist);
    free(out.segmentmarkerlist);
    free(out.edgelist);
    free(out.edgemarkerlist);

    return edges_idxs;
}

// vector<int> list_neighbors(vector<double>& points_xy)
// {
//     // z: zero-indexed
//     // Q: quiet
//     // B: no boundary markers
//     // e: provide list of edges
//     char tri_switches[] = "zQBe";

//     struct triangulateio in = {};
//     in.numberofpoints = points_xy.size() / 2;
//     in.numberofpointattributes = 0;
//     in.pointlist = points_xy.data();

//     struct triangulateio out = {};
//     triangulate(tri_switches, &in, &out, NULL);

//     static_assert(std::is_same<decltype(out.edgelist), int*>::value, "types do not match");
//     vector<int> neighbors_idxs;
//     neighbors_idxs.resize(out.numberoftriangles * 3);
//     std::memcpy(neighbors_idxs.data(), out.neighborlist, sizeof(int) * out.numberoftriangles * 3);

//     free(out.pointlist);
//     free(out.pointattributelist);
//     free(out.pointmarkerlist);
//     free(out.trianglelist);
//     free(out.triangleattributelist);
//     free(out.neighborlist);
//     free(out.segmentlist);
//     free(out.segmentmarkerlist);
//     free(out.edgelist);
//     free(out.edgemarkerlist);

//     return neighbors_idxs;
// }

// std::tuple<vector<double>, vector<int>> triangulate(vector<double>& points_xy, double max_area)
// {
//     if (max_area < 0 || max_area > 100) {
//         std::cerr << "max_area must be in [0, 100] range" << std::endl;
//         exit(1);
//     }

//     // z: zero-indexed
//     // Q: quiet
//     // B: no boundary markers
//     // a: max triangle area
//     // n: provide list of neighbpprs
//     char tri_switches[100] = "";
//     sprintf(tri_switches, "zQBa%0.4f", max_area);

//     struct triangulateio in = {};
//     in.numberofpoints = points_xy.size() / 2;
//     in.numberofpointattributes = 0;
//     in.pointlist = points_xy.data();

//     struct triangulateio out = {};
//     triangulate(tri_switches, &in, &out, NULL);

//     static_assert(std::is_same<decltype(out.pointlist), double*>::value, "types do not match");
//     vector<double> points_xy_out;
//     points_xy_out.resize(out.numberofpoints * 2);
//     std::memcpy(points_xy_out.data(), out.pointlist, sizeof(double) * out.numberofpoints * 2);

//     static_assert(std::is_same<decltype(out.neighborlist), int*>::value, "types do not match");
//     vector<int> neighbors_idxs;
//     neighbors_idxs.resize(out.numberoftriangles * 2);
//     std::memcpy(neighbors_idxs.data(), out.neighborlist, sizeof(int) * out.numberoftriangles * 3);

//     free(out.pointlist);
//     free(out.pointattributelist);
//     free(out.pointmarkerlist);
//     free(out.trianglelist);
//     free(out.triangleattributelist);
//     free(out.neighborlist);
//     free(out.segmentlist);
//     free(out.segmentmarkerlist);
//     free(out.edgelist);
//     free(out.edgemarkerlist);

//     return {points_xy_out, neighbors_idxs};
// }
}
