// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <cmath>
#include <cstddef>

#include "benchmark/benchmark.h"
#include "lib/jxl/cms/transfer_functions.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "lib/jxl/tf_gbench.cc"
#include <hwy/foreach_target.h>
#include <hwy/highway.h>

#include "lib/jxl/cms/transfer_functions-inl.h"

HWY_BEFORE_NAMESPACE();
namespace jxl {
namespace HWY_NAMESPACE {

using hwy::HWY_NAMESPACE::Add;
using hwy::HWY_NAMESPACE::GetLane;

namespace {

#define RUN_BENCHMARK(F)                                             \
  constexpr size_t kNum = 1 << 12;                                   \
  HWY_FULL(float) d;                                                 \
  /* Three parallel runs, as this will run on R, G and B. */         \
  auto sum1 = Zero(d);                                               \
  auto sum2 = Zero(d);                                               \
  auto sum3 = Zero(d);                                               \
  for (auto _ : state) {                                             \
    (void)_;                                                         \
    auto x = Set(d, 1e-5);                                           \
    auto v1 = Set(d, 1e-5);                                          \
    auto v2 = Set(d, 1.1e-5);                                        \
    auto v3 = Set(d, 1.2e-5);                                        \
    for (size_t i = 0; i < kNum; i++) {                              \
      sum1 = Add(sum1, F(d, v1));                                    \
      sum2 = Add(sum2, F(d, v2));                                    \
      sum3 = Add(sum3, F(d, v3));                                    \
      v1 = Add(v1, x);                                               \
      v2 = Add(v2, x);                                               \
      v3 = Add(v3, x);                                               \
    }                                                                \
  }                                                                  \
  /* floats per second */                                            \
  state.SetItemsProcessed(state.iterations() * Lanes(d) * 3 * kNum); \
  benchmark::DoNotOptimize(GetLane(SumOfLanes(d, Add(sum1, Add(sum2, sum3)))));

#define RUN_BENCHMARK_SCALAR(F, I)                           \
  constexpr size_t kNum = 1 << 12;                           \
  /* Three parallel runs, as this will run on R, G and B. */ \
  float sum1 = 0;                                            \
  float sum2 = 0;                                            \
  float sum3 = 0;                                            \
  for (auto _ : state) {                                     \
    (void)_;                                                 \
    float x = 1e-5;                                          \
    float v1 = 1e-5;                                         \
    float v2 = 1.1e-5;                                       \
    float v3 = 1.2e-5;                                       \
    for (size_t i = 0; i < kNum; i++) {                      \
      sum1 += F(I, v1);                                      \
      sum2 += F(I, v2);                                      \
      sum3 += F(I, v3);                                      \
      v1 += x;                                               \
      v2 += x;                                               \
      v3 += x;                                               \
    }                                                        \
  }                                                          \
  /* floats per second */                                    \
  state.SetItemsProcessed(kNum* state.iterations() * 3);     \
  benchmark::DoNotOptimize(sum1 + sum2 + sum3);

HWY_NOINLINE void BM_FastSRGB(benchmark::State& state) {
  RUN_BENCHMARK(FastLinearToSRGB);
}

HWY_NOINLINE void BM_TFSRGB(benchmark::State& state) {
  RUN_BENCHMARK(TF_SRGB().EncodedFromDisplay);
}

HWY_NOINLINE void BM_PQDFE(benchmark::State& state) {
  TF_PQ tf_pq(10000.0);
  RUN_BENCHMARK(tf_pq.DisplayFromEncoded);
}

HWY_NOINLINE void BM_PQEFD(benchmark::State& state) {
  TF_PQ tf_pq(10000.0);
  RUN_BENCHMARK(tf_pq.EncodedFromDisplay);
}

HWY_NOINLINE void BM_PQSlowDFE(benchmark::State& state) {
  RUN_BENCHMARK_SCALAR(TF_PQ_Base::DisplayFromEncoded, 10000.0);
}

HWY_NOINLINE void BM_PQSlowEFD(benchmark::State& state) {
  RUN_BENCHMARK_SCALAR(TF_PQ_Base::EncodedFromDisplay, 10000.0);
}
}  // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace jxl
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace jxl {
namespace {

HWY_EXPORT(BM_FastSRGB);
HWY_EXPORT(BM_TFSRGB);
HWY_EXPORT(BM_PQDFE);
HWY_EXPORT(BM_PQEFD);
HWY_EXPORT(BM_PQSlowDFE);
HWY_EXPORT(BM_PQSlowEFD);

float SRGB_pow(float _, float x) {
  return x < 0.0031308f ? 12.92f * x
                        : 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
}

void BM_FastSRGB(benchmark::State& state) {
  HWY_DYNAMIC_DISPATCH(BM_FastSRGB)(state);
}
void BM_TFSRGB(benchmark::State& state) {
  HWY_DYNAMIC_DISPATCH(BM_TFSRGB)(state);
}
void BM_PQDFE(benchmark::State& state) {
  HWY_DYNAMIC_DISPATCH(BM_PQDFE)(state);
}
void BM_PQEFD(benchmark::State& state) {
  HWY_DYNAMIC_DISPATCH(BM_PQEFD)(state);
}
void BM_PQSlowDFE(benchmark::State& state) {
  HWY_DYNAMIC_DISPATCH(BM_PQSlowDFE)(state);
}
void BM_PQSlowEFD(benchmark::State& state) {
  HWY_DYNAMIC_DISPATCH(BM_PQSlowEFD)(state);
}

void BM_SRGB_pow(benchmark::State& state) { RUN_BENCHMARK_SCALAR(SRGB_pow, 0); }

BENCHMARK(BM_FastSRGB);
BENCHMARK(BM_TFSRGB);
BENCHMARK(BM_SRGB_pow);
BENCHMARK(BM_PQDFE);
BENCHMARK(BM_PQEFD);
BENCHMARK(BM_PQSlowDFE);
BENCHMARK(BM_PQSlowEFD);

}  // namespace
}  // namespace jxl
#endif
