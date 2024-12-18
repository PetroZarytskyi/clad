// RUN: %cladclang %s -I%S/../../include -oTemplateFunctors.out 2>&1 | %filecheck %s
// RUN: ./TemplateFunctors.out | %filecheck_exec %s
// RUN: %cladclang -Xclang -plugin-arg-clad -Xclang -enable-tbr %s -I%S/../../include -oTemplateFunctors.out
// RUN: ./TemplateFunctors.out | %filecheck_exec %s

#include "clad/Differentiator/Differentiator.h"

template <typename T> struct Experiment {
  mutable T x, y;
  Experiment(T p_x = 0, T p_y = 0) : x(p_x), y(p_y) {}
  T operator()(T i, T j) { return x * i * i + y * j; }
  void setX(T val) { x = val; }
  Experiment& operator=(const Experiment& E) = default;
};

// CHECK: void operator_call_grad(double i, double j, Experiment<double> *_d_this, double *_d_i, double *_d_j) {
// CHECK-NEXT:     {
// CHECK-NEXT:         (*_d_this).x += 1 * i * i;
// CHECK-NEXT:         *_d_i += this->x * 1 * i;
// CHECK-NEXT:         *_d_i += this->x * i * 1;
// CHECK-NEXT:         (*_d_this).y += 1 * j;
// CHECK-NEXT:         *_d_j += this->y * 1;
// CHECK-NEXT:     }
// CHECK-NEXT: }

template <> struct Experiment<long double> {
  mutable long double x, y;
  Experiment(long double p_x = 0, long double p_y = 0) : x(p_x), y(p_y) {}
  long double operator()(long double i, long double j) {
    return x * i * i * j + y * j * i;
  }
  void setX(long double val) { x = val; }
  Experiment& operator=(const Experiment& E) = default;
};

// CHECK: void operator_call_grad(long double i, long double j, Experiment<long double>  *_d_this, long double  *_d_i, long double  *_d_j) {
// CHECK-NEXT:     {
// CHECK-NEXT:         (*_d_this).x += 1 * j * i * i;
// CHECK-NEXT:         *_d_i += this->x * 1 * j * i;
// CHECK-NEXT:         *_d_i += this->x * i * 1 * j;
// CHECK-NEXT:         *_d_j += this->x * i * i * 1;
// CHECK-NEXT:         (*_d_this).y += 1 * i * j;
// CHECK-NEXT:         *_d_j += this->y * 1 * i;
// CHECK-NEXT:         *_d_i += this->y * j * 1;
// CHECK-NEXT:     }
// CHECK-NEXT: }

template <typename T> struct ExperimentConst {
  mutable T x, y;
  ExperimentConst(T p_x = 0, T p_y = 0) : x(p_x), y(p_y) {}
  T operator()(T i, T j) const { return x * i * i + y * j; }
  void setX(T val) const { x = val; }
  ExperimentConst& operator=(const ExperimentConst& E) = default;
};

// CHECK: void operator_call_grad(double i, double j, ExperimentConst<double> *_d_this, double *_d_i, double *_d_j) const {
// CHECK-NEXT:     {
// CHECK-NEXT:         (*_d_this).x += 1 * i * i;
// CHECK-NEXT:         *_d_i += this->x * 1 * i;
// CHECK-NEXT:         *_d_i += this->x * i * 1;
// CHECK-NEXT:         (*_d_this).y += 1 * j;
// CHECK-NEXT:         *_d_j += this->y * 1;
// CHECK-NEXT:     }
// CHECK-NEXT: }

template <> struct ExperimentConst<long double> {
  mutable long double x, y;
  ExperimentConst(long double p_x = 0, long double p_y = 0) : x(p_x), y(p_y) {}
  long double operator()(long double i, long double j) const {
    return x * i * i * j + y * j * i;
  }
  void setX(long double val) const { x = val; }
  ExperimentConst& operator=(const ExperimentConst& E) = default;
};

// CHECK: void operator_call_grad(long double i, long double j, ExperimentConst<long double> *_d_this, long double  *_d_i, long double  *_d_j) const {
// CHECK-NEXT:     {
// CHECK-NEXT:         (*_d_this).x += 1 * j * i * i;
// CHECK-NEXT:         *_d_i += this->x * 1 * j * i;
// CHECK-NEXT:         *_d_i += this->x * i * 1 * j;
// CHECK-NEXT:         *_d_j += this->x * i * i * 1;
// CHECK-NEXT:         (*_d_this).y += 1 * i * j;
// CHECK-NEXT:         *_d_j += this->y * 1 * i;
// CHECK-NEXT:         *_d_i += this->y * j * 1;
// CHECK-NEXT:     }
// CHECK-NEXT: }

#define INIT(E)                                                                \
  auto d_##E = clad::gradient(&E);                                             \
  auto d_##E##Ref = clad::gradient(E);

#define TEST_DOUBLE(E, dE, ...)                                                \
  di = dj = 0;                                                                 \
  dE = decltype(dE)();                                                         \
  d_##E.execute(__VA_ARGS__, &dE, &di, &dj);                                   \
  printf("{%.2f, %.2f} ", di, dj);                                             \
  di = dj = 0;                                                                 \
  dE = decltype(dE)();                                                         \
  d_##E##Ref.execute(__VA_ARGS__, &dE, &di, &dj);                              \
  printf("{%.2f, %.2f}\n", di, dj);

#define TEST_LONG_DOUBLE(E, dE, ...)                                           \
  di_ld = dj_ld = 0;                                                           \
  dE = decltype(dE)();                                                         \
  d_##E.execute(__VA_ARGS__, &dE, &di_ld, &dj_ld);                             \
  printf("{%.2Lf, %.2Lf} ", di_ld, dj_ld);                                     \
  di_ld = dj_ld = 0;                                                           \
  dE = decltype(dE)();                                                         \
  d_##E##Ref.execute(__VA_ARGS__, &dE, &di_ld, &dj_ld);                        \
  printf("{%.2Lf, %.2Lf}\n", di_ld, dj_ld);

int main() {
  double di, dj;
  long double di_ld, dj_ld;

  Experiment<double> E_double(3, 5), dE_double;
  Experiment<long double> E_ld(3, 5), dE_ld;
  ExperimentConst<double> EConst_double(3, 5), dEConst_double;
  ExperimentConst<long double> EConst_ld(3, 5), dEConst_ld;

  INIT(E_double);
  INIT(E_ld);
  INIT(EConst_double);
  INIT(EConst_ld);

  TEST_DOUBLE(E_double, dE_double, 7, 9);                               // CHECK-EXEC: {42.00, 5.00} {42.00, 5.00}
  TEST_LONG_DOUBLE(E_ld, dE_ld, 7, 9);                                  // CHECK-EXEC: {423.00, 182.00} {423.00, 182.00}
  TEST_DOUBLE(EConst_double, dEConst_double, 7, 9);                     // CHECK-EXEC: {42.00, 5.00} {42.00, 5.00}
  TEST_LONG_DOUBLE(EConst_ld, dEConst_ld, 7, 9);                        // CHECK-EXEC: {423.00, 182.00} {423.00, 182.00}

  E_double.setX(5);
  E_ld.setX(5);
  EConst_double.setX(5);
  EConst_ld.setX(5);

  TEST_DOUBLE(E_double, dE_double, 7, 9);                               // CHECK-EXEC: {70.00, 5.00} {70.00, 5.00}
  TEST_LONG_DOUBLE(E_ld, dE_ld, 7, 9);                                  // CHECK-EXEC: {675.00, 280.00} {675.00, 280.00}
  TEST_DOUBLE(EConst_double, dEConst_double, 7, 9);                     // CHECK-EXEC: {70.00, 5.00} {70.00, 5.00}
  TEST_LONG_DOUBLE(EConst_ld, dEConst_ld, 7, 9);                        // CHECK-EXEC: {675.00, 280.00} {675.00, 280.00}
}
