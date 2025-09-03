#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <papi.h>

using std::cout;
using std::endl;
using std::uint64_t;
using namespace std::chrono;

#define INP_H (1 << 9)
#define INP_W (1 << 9)
#define INP_D (1 << 9)
#define FIL_H (3)
#define FIL_W (3)
#define FIL_D (3)
#define B_H (1)
#define B_W (1)
#define B_D (256)

/** Naïve 3D cross-correlation (no padding) */
void cc_3d_no_padding(const uint64_t* input,
                      const uint64_t (*kernel)[FIL_W][FIL_D], uint64_t* result,
                      const uint64_t outputHeight, const uint64_t outputWidth,
                      const uint64_t outputDepth) {
  for (uint64_t i = 0; i < outputHeight; i++) {
    for (uint64_t j = 0; j < outputWidth; j++) {
      for (uint64_t k = 0; k < outputDepth; k++) {
        uint64_t sum = 0;
        for (uint64_t ki = 0; ki < FIL_H; ki++) {
          for (uint64_t kj = 0; kj < FIL_W; kj++) {
            for (uint64_t kk = 0; kk < FIL_D; kk++) {
              sum += input[(i + ki) * INP_W * INP_D + (j + kj) * INP_D +
                           (k + kk)] *
                     kernel[ki][kj][kk];
            }
          }
        }
        result[i * outputWidth * outputDepth + j * outputDepth + k] += sum;
      }
    }
  }
}

/** Blocked 3D cross-correlation (no padding) */
void cc_3d_input_blocked(const uint64_t* input,
                                    const uint64_t (*kernel)[FIL_W][FIL_D],
                                    uint64_t* result,
                                    const uint64_t outputHeight,
                                    const uint64_t outputWidth,
                                    const uint64_t outputDepth) {
  for (uint64_t i = 0; i < INP_H; i += B_H) {
    for (uint64_t j = 0; j < INP_W; j += B_W) {
      for (uint64_t k = 0; k < INP_D; k += B_D) {
        for (uint64_t bi = i; bi < i + B_H && bi < INP_H; bi++) {
          for (uint64_t bj = j; bj < j + B_W && bj < INP_W; bj++) {
            for (uint64_t bk = k; bk < k + B_D && bk < INP_D; bk++) {
              uint64_t val = input[bi * INP_W * INP_D + bj * INP_D + bk];
              for (uint64_t ki = 0; ki < FIL_H; ki++) {
                for (uint64_t kj = 0; kj < FIL_W; kj++) {
                  for (uint64_t kk = 0; kk < FIL_D; kk++) {
                    uint64_t oi = bi - ki;
                    uint64_t oj = bj - kj;
                    uint64_t ok = bk - kk;
                    if (oi >= 0 && oj >= 0 && ok >=0 && oi < outputHeight && oj < outputWidth && ok < outputDepth) {
                      result[oi * outputWidth * outputDepth + oj * outputDepth + ok] += val * kernel[ki][kj][kk];
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void cc_3d_output_blocked(const uint64_t* input,
  const uint64_t (*kernel)[FIL_W][FIL_D],
  uint64_t* result,
  const uint64_t outputHeight,
  const uint64_t outputWidth,
  const uint64_t outputDepth) {
  for (uint64_t i = 0; i < outputHeight; i += B_H) {
    for (uint64_t j = 0; j < outputWidth; j += B_W) {
      for (uint64_t k = 0; k < outputDepth; k += B_D) {
        for (uint64_t bi = i; bi < i + B_H && bi < outputHeight; bi++) {
          for (uint64_t bj = j; bj < j + B_W && bj < outputWidth; bj++) {
            for (uint64_t bk = k; bk < k + B_D && bk < outputDepth; bk++) {
              uint64_t sum = 0;
              for (uint64_t ki = 0; ki < FIL_H; ki++) {
                for (uint64_t kj = 0; kj < FIL_W; kj++) {
                  for (uint64_t kk = 0; kk < FIL_D; kk++) {
                    sum += input[(bi + ki) * INP_W * INP_D + (bj + kj) * INP_D + (bk + kk)] * kernel[ki][kj][kk];
                  }
                }
              }
              result[bi * outputWidth * outputDepth + bj * outputDepth + bk] += sum;
            }
          }
        }
      }
    }
  }
} 

void check_equal(const uint64_t* a, const uint64_t* b,
                 const uint64_t size) {
  for (uint64_t i = 0; i < size; i++) {
    assert(a[i] == b[i]);
  }
}

int main(int argc, char* argv[]) {
  uint64_t* input = new uint64_t[INP_H * INP_W * INP_D];
  std::fill_n(input, INP_H * INP_W * INP_D, 1);

  uint64_t filter[FIL_H][FIL_W][FIL_D] = {{{2, 1, 3}, {2, 1, 3}, {2, 1, 3}},
                                          {{2, 1, 3}, {2, 1, 3}, {2, 1, 3}},
                                          {{2, 1, 3}, {2, 1, 3}, {2, 1, 3}}};

  uint64_t outputHeight = INP_H - FIL_H + 1;
  uint64_t outputWidth  = INP_W - FIL_W + 1;
  uint64_t outputDepth  = INP_D - FIL_D + 1;

  auto* result_naive   = new uint64_t[outputHeight * outputWidth * outputDepth];
  auto* result_input_blocked = new uint64_t[outputHeight * outputWidth * outputDepth];
  auto* result_output_blocked = new uint64_t[outputHeight * outputWidth * outputDepth];

  const int runs = 5;
  double total_naive = 0.0, total_input_blocked = 0.0, total_output_blocked = 0.0;

  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    std::cerr << "PAPI library init error!" << std::endl;
    return EXIT_FAILURE;
  }

  const int num_events = 3;
  int events[num_events] = {PAPI_L1_DCM, PAPI_L2_DCM, PAPI_TOT_CYC};
  long long values[num_events] = {0, 0, 0};
  long long total_naive_counters[num_events] = {0, 0, 0};
  long long total_input_blocked_counters[num_events] = {0, 0, 0};
  long long total_output_blocked_counters[num_events] = {0, 0, 0};

  int EventSet = PAPI_NULL;
  if (PAPI_create_eventset(&EventSet) != PAPI_OK) {
    std::cerr << "PAPI_create_eventset failed\n";
    return EXIT_FAILURE;
  }
  if (PAPI_add_events(EventSet, events, num_events) != PAPI_OK) {
    std::cerr << "PAPI_add_events failed\n";
    return EXIT_FAILURE;
  }

  for (int r = 0; r < runs; r++) {
    std::cout<<"Run "<<r+1<<std::endl;
    std::fill_n(result_naive, outputHeight * outputWidth * outputDepth, 0);
    if(PAPI_start(EventSet) != PAPI_OK) {
      std::cerr << "PAPI start counters error!" << std::endl;
      return EXIT_FAILURE;
    }
    auto start = high_resolution_clock::now();
    cc_3d_no_padding(input, filter, result_naive, outputHeight, outputWidth, outputDepth);
    auto stop = high_resolution_clock::now();

    if (PAPI_stop(EventSet, values) != PAPI_OK) {
      std::cerr << "PAPI stop counters error!" << std::endl;
      return EXIT_FAILURE;
    }
    total_naive += duration_cast<microseconds>(stop - start).count() / 1000.0;

    for (int i = 0; i < num_events; i++) {
      total_naive_counters[i] += values[i];
    }

    std::cout<<"Naive Counters: L1 DCM="<<values[0]<<", L2 DCM="<<values[1]<<", CPU Cycles="<<values[2]<<std::endl;

    std::fill_n(result_input_blocked, outputHeight * outputWidth * outputDepth, 0);

    if(PAPI_start(EventSet) != PAPI_OK) {
      std::cerr << "PAPI start counters error!" << std::endl;
      return EXIT_FAILURE;
    }
    start = high_resolution_clock::now();
    cc_3d_input_blocked(input, filter, result_input_blocked,
                             outputHeight, outputWidth, outputDepth);
    stop = high_resolution_clock::now();

    if (PAPI_stop(EventSet, values) != PAPI_OK) {
      std::cerr << "PAPI stop counters error!" << std::endl;
      return EXIT_FAILURE;
    }

    total_input_blocked += duration_cast<microseconds>(stop - start).count() / 1000.0;

    for (int i = 0; i < num_events; i++) {
      total_input_blocked_counters[i] += values[i];
    }

    std::cout<<"Input Blocked:  L1 DCM="<<values[0]<<", L2 DCM="<<values[1]<<", CPU Cycles="<<values[2]<<std::endl;
    check_equal(result_naive, result_input_blocked, outputHeight * outputWidth * outputDepth);

    std::fill_n(result_output_blocked, outputHeight * outputWidth * outputDepth, 0);
    if(PAPI_start(EventSet) != PAPI_OK) {
      std::cerr << "PAPI start counters error!" << std::endl;
      return EXIT_FAILURE;
    }
    start = high_resolution_clock::now();
    cc_3d_output_blocked(input, filter, result_output_blocked,
      outputHeight, outputWidth, outputDepth);
    stop = high_resolution_clock::now();

    if (PAPI_stop(EventSet, values) != PAPI_OK) {
      std::cerr << "PAPI stop counters error!" << std::endl;
      return EXIT_FAILURE;
    }

    total_output_blocked += duration_cast<microseconds>(stop - start).count() / 1000.0;

    for (int i = 0; i < num_events; i++) {
      total_output_blocked_counters[i] += values[i];
    }

    std::cout<<"Output Blocked: L1 DCM="<<values[0]<<", L2 DCM="<<values[1]<<", CPU Cycles="<<values[2]<<std::endl;
    check_equal(result_naive, result_output_blocked, outputHeight * outputWidth * outputDepth);
    
    std::cout << endl;
  }

  double avg_naive = total_naive / runs;
  double avg_blocked = total_input_blocked / runs;
  double avg_output_blocked = total_output_blocked / runs;

  cout << "=== Timing Results (averaged over " << runs << " runs) ===\n";
  cout << "Naive   : " << avg_naive   << " ms\n";
  cout << "Input Blocked : " << avg_blocked << " ms\n";
  cout << "Output Blocked : " << avg_output_blocked << " ms\n";

  delete[] input;
  delete[] result_naive;
  delete[] result_input_blocked;

  return EXIT_SUCCESS;
}
