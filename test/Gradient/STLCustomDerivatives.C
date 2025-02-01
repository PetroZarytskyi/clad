// XFAIL: asserts
// RUN: %cladclang -std=c++14 %s -I%S/../../include -oSTLCustomDerivatives.out 2>&1 | %filecheck %s
// RUN: ./STLCustomDerivatives.out | %filecheck_exec %s
// RUN: %cladclang -std=c++14 -Xclang -plugin-arg-clad -Xclang -enable-tbr %s -I%S/../../include -oSTLCustomDerivativesWithTBR.out
// RUN: ./STLCustomDerivativesWithTBR.out | %filecheck_exec %s

#include "clad/Differentiator/Differentiator.h"
#include "clad/Differentiator/STLBuiltins.h"
#include "../TestUtils.h"
#include "../PrintOverloads.h"

#include <array>
#include <vector>

double fn23(double u, double v) {
    for (int i = 0; i < 3; ++i) {
      std::vector<double> ls({u, v});
    }
    return u;
}

double fn24(double u, double v) {
    for (int i = 0; i < 2; ++i) {
      std::vector<double> ls({u, v});
      ls[1] += ls[0];
    }
    return u;
}


int main() {
    double d_i, d_j;
    INIT_GRADIENT(fn24);
    INIT_GRADIENT(fn23);

    TEST_GRADIENT(fn24, /*numOfDerivativeArgs=*/2, 161, 39, &d_i, &d_j);  // CHECK-EXEC: {161.00, 39.00}
    TEST_GRADIENT(fn23, /*numOfDerivativeArgs=*/2, 37, 75, &d_i, &d_j);  // CHECK-EXEC: {37.00, 75.00}
}
