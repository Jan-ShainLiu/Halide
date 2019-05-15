#include "Halide.h"

using namespace Halide;

int main(int argc, char **argv) {

    if (1) {
        // Parallel in-place
        Func f, g;
        Var x;
        f(x) = x;
        g(x) = f(x) + 3;
        f.compute_root().store_with(g);
        g.vectorize(x, 8, TailStrategy::RoundUp).parallel(x);
        f.vectorize(x, 4, TailStrategy::RoundUp).parallel(x);
        g.realize(128);
    }

    if (1) {
        Func f, g;
        Var x, y;

        f(x, y) = x + y;
        RDom r(0, 99);
        f(r + 1, y) += f(r, y);
        f(98 - r, y) += f(99 - r, y);
        g(x, y) = f(x, y);

        g.unroll(y, 5, TailStrategy::RoundUp);

        f.compute_at(g, y).store_with(g);

        g.realize(100, 100);
    }

    if (1) {
        // Move an array one vector to the left, in-place
        Func f, g, h;
        Var x;

        f(x) = x;
        g(x) = f(x+8);
        h(x) = g(x);

        f.compute_at(g, x).vectorize(x, 8, TailStrategy::GuardWithIf);

        f.store_with(g);
        g.compute_root();
        h.compute_root();

        Buffer<int> result = h.realize(100);

        for (int i = 0; i < 100; i++) {
            int correct = 2*i;
            printf("%d %d\n", result(i), correct);
        }
    }

    if (1) {
        Func f, g;
        Var x;

        f(x) = x;
        g(x) = undef<int>();
        RDom r(10, 80);
        g(2*r) = f(2*r - 1) + f(2*r + 1);

        f.compute_root().store_with(g);
        g.compute_root();

        Buffer<int> result = g.realize(200);
    }

    if (1) {
        // Broadcast
        Func f, g;
        Var x;
        f(x) = 12345;
        g(x) = undef<int>();
        g(0) = f(17);
        g(1) = f(16);
        g(2) = f(15);

        f.compute_root().store_with(g);
        g.realize(100);
    }

    if (1) {
        // Concat
        Func f, g, h;
        Var x, y;

        f(x) = 18701;
        g(x) = 345;
        h(x) = select(x < 100, f(x), g(max(0, x - 100)));

        // TODO: We need to move add_image_checks later for this to
        // work out nicely without the max. Right now it'll be overconservative.

        f.compute_root().store_with(h);
        g.compute_root().store_with(h, {x + 100});
        h.bound(x, 0, 200);
        h.realize(200);
    }

    if (1) {
        // In-place convolution. Shift the producer over a little to
        // avoid being clobbered by the consumer. This would write out
        // of bounds, so g can't be the output.
        Func f, g, h;
        Var x;
        f(x) = x;
        g(x) = f(x-1) + f(x) + f(x+1);
        h(x) = g(x);
        // If f is compute_root, then the realization of f is not
        // within the realization of g, so it's actually an
        // error. Need to add error checking, or place the realization
        // somewhere that includes both. Right now it just produced a
        // missing symbol error.
        f.compute_at(g, Var::outermost()).store_with(g, {x+1});
        g.compute_root();
        h.realize(100);
    }

    if (1) {
        // 2D in-place convolution.
        Func f, g, h;
        Var x, y;
        f(x, y) = x + y;
        g(x, y) = f(x-1, y-1) + f(x+1, y+1);
        h(x, y) = g(x, y);

        g.compute_root();
        // Computation of f must be nested inside computation of g
        f.compute_at(g, Var::outermost()).store_with(g, {x+1, y+1});
        h.realize(100, 100);
    }

    if (1) {
        // 2D in-place convolution computed per scanline
        Func f, g, h;
        Var x, y;
        f(x, y) = x + y;
        g(x, y) = f(x-1, y-1) + f(x+1, y+1);
        h(x, y) = g(x, y);

        g.compute_root();
        // Store slices of f two scanlines down in the as-yet-unused region of g
        f.compute_at(g, y).store_with(g, {x, y+2});
        h.realize(100, 100);
    }

    if (1) {
        // 2D in-place convolution computed per scanline with sliding
        Func f, g, h;
        Var x, y;
        f(x, y) = x + y;
        g(x, y) = f(x-1, y-1) + f(x+1, y+1);
        h(x, y) = g(x, y);

        g.compute_root();
        f.store_root().compute_at(g, y).store_with(g, {x, y+3});
        h.realize(100, 100);
    }

    if (1) {
        // split then merge
        Func f, g, h, out;
        Var x;
        f(x) = x;
        g(x) = f(2*x) + 1;
        h(x) = f(2*x+1) * 2;
        out(x) = select(x % 2 == 0, g(x/2), h(x/2));

        f.compute_root().store_with(out);
        g.compute_root().store_with(out, {2*x}); // Store g at the even spots in out
        h.compute_root().store_with(out, {2*x+1});  // Store h in the odd spots

        out.realize(100);

    }

    if (1) {
        // split then merge, with parallelism
        Func f, g, h, out;
        Var x;
        f(x) = x;
        g(x) = f(2*x) + 1;
        h(x) = f(2*x+1) * 2;
        out(x) = select(x % 2 == 0, g(x/2), h(x/2));

        f.compute_root().vectorize(x, 8).store_with(out);
        g.compute_root().vectorize(x, 8, TailStrategy::RoundUp).store_with(out, {2*x}); // Store g at the even spots in out
        h.compute_root().vectorize(x, 8, TailStrategy::RoundUp).store_with(out, {2*x+1});  // Store h in the odd spots
        out.vectorize(x, 8, TailStrategy::RoundUp);

        f.async();
        g.async();
        h.async();

        out.realize(128);
    }

    if (1) {
        // A double integration in-place
        Func f, g, h;
        Var x;
        f(x) = x;
        RDom r(1, 99);
        g(x) = f(x);
        g(r) += g(r-1);
        h(x) = g(x);
        h(r) += h(r-1);

        f.compute_root().store_with(h);
        g.compute_root().store_with(h);
        h.bound(x, 0, 100);
        Buffer<int> buf = h.realize(100);

        for (int i = 0; i < 100; i++) {
            printf("%d %d\n", i, buf(i));
        }
    }

    if (1) {
        // A tiled pyramid
        Func f, g, h;
        Var x, y;

        f(x, y) = x + y;

        g(x, y) = f(x/2, y/2) + 1;
        h(x, y) = g(x/2, y/2) + 2;

        // Store a 4x4 block of f densely in the top left of every 16x16 tile of h
        f.compute_at(h, Var::outermost()).store_with(h, {16*(x/4) + x%4, 16*(y/4) + y%4}).vectorize(x).unroll(y);

        // Store an 8x8 block of g similarly compacted in the bottom
        // right. It doesn't collide with f, and we're OK to overwrite
        // it when computing h because we compute h serially across x
        // and y.
        g.compute_at(h, Var::outermost()).store_with(h, {16*(x/8) + x%8 + 8, 16*(y/8) + y%8 + 8}).vectorize(x).unroll(y);

        Var xi, yi;
        h.compute_at(h.in(), x).vectorize(x).unroll(y);
        h = h.in();
        h.bound(x, 0, 128).bound(y, 0, 128).tile(x, y, xi, yi, 16, 16, TailStrategy::RoundUp).vectorize(xi).unroll(yi);

        h.realize(128, 128);

    }

    if (0) {
        Func f, g, h;
        Var x;
        f(x) = x;
        g(x) = f(x) * f(x + 1);
        h(x) = g(x) + 8;
        f.compute_at(g, Var::outermost()).store_with(g);
        g.compute_root().parallel(x);
        h.compute_root();
        Buffer<int> out = h.realize(100);
        for (int i = 0; i < 100; i++) {
            int correct = i*(i+1) + 8;
            if (out(i) != correct) {
                printf("out(%d) = %d instead of %d\n", i, out(i), correct);
            }
        }
    }

    printf("Success!\n");

    return 0;
}
