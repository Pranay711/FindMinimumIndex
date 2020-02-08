﻿#include <algorithm>
#include <benchmark/benchmark.h>
#include <random>
#include <stdlib.h>
#include <iostream>
/*
 * Alignment of  64 bytes
 */
constexpr int alignment = 64;
/*
 * create global data
 * a bit hacky way
 */
constexpr size_t nn = 8 << 10;
float* inArray;
class InitArray
{
public:
  InitArray()
  {
    std::mt19937 gen;
    std::uniform_real_distribution<> dis(1.0, 10.0);
    // create buffer of right size,properly aligned
    size_t const size = nn * sizeof(float);
    posix_memalign((void**)&inArray, alignment, size);
    for (size_t i = 0; i < nn; ++i) {
      // Use dis to transform the random unsigned int generated by gen into a
      // double. Each call to dis(gen) generates a new random double
      inArray[i] = dis(gen);
    }
  }
  ~InitArray() { free(inArray); }
};
InitArray initArray;

static void
findMinimumIndexC(benchmark::State& state)
{

  for (auto _ : state) {
    const int n = state.range(0);
    float* distances = (float*)__builtin_assume_aligned(inArray, alignment);
    int minIndex = 0; // always 1e30
    int mini2 = -1;
    float minDistance = distances[0];
    float minDistance2 = std::numeric_limits<float>::max();

    for (int i = 1; i < n; ++i) {
      if (distances[i] < minDistance) {
        mini2 = minIndex;
        minDistance2 = minDistance;
        minIndex = i;
        minDistance = distances[i];
      } else if (distances[i] < minDistance2) {
        mini2 = i;
        minDistance2 = distances[i];
      }
    }

    benchmark::DoNotOptimize(&minIndex);
    benchmark::DoNotOptimize(&mini2);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(findMinimumIndexC)->Range(64, nn);

BENCHMARK_MAIN();

