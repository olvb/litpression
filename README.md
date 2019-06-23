## Simple and fast painterly rendering

Litpression implements the algorithm described in [Peter Litwinowicz's article][1]. It relies on OpenCV 4 for optical flow (various algorithms available) and on the [triangle library][2] for Delaunay triangulation.

The algorithm's central idea is to cover the canvas with brush strokes sampling their colors from the original image, and to move these strokes with optical flow in order to maintain time consistency. As strokes move around the canvas, new strokes will be created to keep covering the whole surface, and other will be deleted in overdense areas.

[1]: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.471.9691&rep=rep1&type=pdf
[2]: https://www.cs.cmu.edu/~quake/triangle.html

If a fast optical flow method is chosen, such as [DIS][3], it can achieve real-time or near-real-time processing speed, depending on the video resolution and on the rendering settings (thinner strokes means more strokes to cover the canvas hence more processing time).

[3]: https://arxiv.org/pdf/1603.03590.pdf

This project is still work in progress. One important feature missing is the interpolation of gradient values for pixels with low gradient magnitudes, which should reduce noise when using gradient for stroke orientations.

## Preview

Rendering done with DIS optical flow, hence the noisy background

![](https://raw.githubusercontent.com/olvb/litpression/master/samples/bbb.gif)

*Shot taken from [Big Buck Bunny](https://peach.blender.org/)*

