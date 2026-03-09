#pragma once

#include "math.hpp"

RAMPAGE_START

template <typename IntT>
float noise2d(IntT x, IntT y, IntT seed) {
  // Fast 64-bit integer mixer (SplitMix64)
  auto mix = [](unsigned long long z) -> unsigned long long {
    z ^= z >> 33;
    z *= 0xff51afd7ed558ccdULL;
    z ^= z >> 33;
    z *= 0xc4ceb9fe1a85ec53ULL;
    z ^= z >> 33;
    return z;
  };

  // Hash grid corner into gradient index [0..11]
  auto get_grad_index = [&](long long ix, long long iy) -> int {
    unsigned long long h = mix(static_cast<unsigned long long>(seed));
    h = mix(h ^ static_cast<unsigned long long>(ix));
    h = mix(h ^ static_cast<unsigned long long>(iy));
    return static_cast<int>(h % 12ULL);
  };

  // Floor for negative numbers
  auto floord = [](double v) -> long long {
    long long i = static_cast<long long>(v);
    return (v < 0.0 && v != static_cast<double>(i)) ? i - 1 : i;
  };

  // 2D Simplex constants
  const double F2 = 0.3660254037844386;
  const double G2 = 0.21132486540518713;

  // 12 gradient directions
  const double grads[12][2] = {{1.0, 1.0}, {-1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0}, {1.0, 0.0},  {-1.0, 0.0},
                               {0.0, 1.0}, {0.0, -1.0}, {1.0, 1.0},  {-1.0, 1.0},  {1.0, -1.0}, {-1.0, -1.0}};

  // ====================== MULTI-OCTAVE fBm (this is the fix!) ======================
  // Scale input so features are large and natural (continents, not speckles)
  double px = static_cast<double>(x) * 0.008; // ← Tweak this number!
  double py = static_cast<double>(y) * 0.008; // 0.005 = huge continents, 0.02 = smaller islands

  const int octaves = 6; // more = more detail
  const double persistence = 0.5; // how fast detail fades
  const double lacunarity = 2.0; // frequency increase per octave

  double total = 0.0;
  double amplitude = 1.0;
  double maxAmp = 0.0;
  double freq = 1.0;

  auto singleSimplex = [&](double X, double Y) -> double {
    double s = (X + Y) * F2;
    double xs = X + s;
    double ys = Y + s;

    long long ii = floord(xs);
    long long jj = floord(ys);

    double t = static_cast<double>(ii + jj) * G2;
    double X0 = static_cast<double>(ii) - t;
    double Y0 = static_cast<double>(jj) - t;
    double x0 = X - X0;
    double y0 = Y - Y0;

    int i1 = (x0 > y0) ? 1 : 0;
    int j1 = (x0 > y0) ? 0 : 1;

    double x1 = x0 - i1 + G2;
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + 2.0 * G2;
    double y2 = y0 - 1.0 + 2.0 * G2;

    int gi0 = get_grad_index(ii, jj);
    int gi1 = get_grad_index(ii + i1, jj + j1);
    int gi2 = get_grad_index(ii + 1, jj + 1);

    double n0 = 0.0, t0 = 0.5 - (x0 * x0 + y0 * y0);
    if (t0 > 0.0) {
      t0 *= t0;
      n0 = t0 * t0 * (grads[gi0][0] * x0 + grads[gi0][1] * y0);
    }

    double n1 = 0.0, t1 = 0.5 - (x1 * x1 + y1 * y1);
    if (t1 > 0.0) {
      t1 *= t1;
      n1 = t1 * t1 * (grads[gi1][0] * x1 + grads[gi1][1] * y1);
    }

    double n2 = 0.0, t2 = 0.5 - (x2 * x2 + y2 * y2);
    if (t2 > 0.0) {
      t2 *= t2;
      n2 = t2 * t2 * (grads[gi2][0] * x2 + grads[gi2][1] * y2);
    }

    return 70.0 * (n0 + n1 + n2); // single octave in ≈[-1, 1]
  };

  // Sum octaves
  for (int i = 0; i < octaves; ++i) {
    total += amplitude * singleSimplex(px * freq, py * freq);
    maxAmp += amplitude;
    amplitude *= persistence;
    freq *= lacunarity;
  }

  return static_cast<float>(total / maxAmp); // normalized to ≈[-1, 1]
}

RAMPAGE_END
