#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#pragma region

#include "vector.hpp"

#pragma endregion

//-----------------------------------------------------------------------------
// System Includes
//-----------------------------------------------------------------------------
#pragma region

#include <cstdio>
#include <fstream>
#include <sstream>

#pragma endregion

//-----------------------------------------------------------------------------
// Declarations and Definitions
//-----------------------------------------------------------------------------
namespace smallpt {

  inline void WritePPM(std::uint32_t w, std::uint32_t h, const Vector3* Ls,
                       const char* fname = "cpp-smallpt-d.ppm") noexcept {

    FILE* fp = fopen(fname, "w");

    std::fprintf(fp, "P3\n%u %u\n%u\n", w, h, 255u);
    for (std::size_t i = 0; i < w * h; ++i) {
      std::fprintf(fp, "%u %u %u ", ToByte(Ls[i].m_x), ToByte(Ls[i].m_y), ToByte(Ls[i].m_z));
    }

    std::fclose(fp);
  }

  void ReadPPMToLs(const char* filename, Vector3* Ls, std::size_t count) {
    std::ifstream file(filename);
    if (!file) return;

    std::string line;
    // Skip header lines
    for (int i = 0; i < 3 && std::getline(file, line); ++i);

    std::uint32_t r, g, b;
    std::size_t i = 0;

    while (file >> r >> g >> b && i < count) {
        Ls[i++] = Vector3(
            FromByte(static_cast<std::uint8_t>(r)),
            FromByte(static_cast<std::uint8_t>(g)),
            FromByte(static_cast<std::uint8_t>(b))
        );
    }

    if (i != count) {
        std::cerr << "Warning: Expected " << count << " pixels but got " << i << "\n";
    }
  }

  double ComputePPMDifference(const char* fname1, const char* fname2, std::uint32_t w, std::uint32_t h) {
    std::size_t count = w * h;
    Vector3 Ls1[count], Ls2[count];
    ReadPPMToLs(fname1, Ls1, count);
    ReadPPMToLs(fname2, Ls2, count);

    double sum = 0;
    for (::std::size_t i = 0; i < count; ++i) {
        double dx = Ls1[i].m_x - Ls2[i].m_x;
        double dy = Ls1[i].m_y - Ls2[i].m_y;
        double dz = Ls1[i].m_z - Ls2[i].m_z;
        sum += dx * dx + dy * dy + dz * dz;
    }

    return sum;
  }
} // namespace smallpt

namespace clad {
namespace custom_derivatives {
namespace smallpt {
// namespace class_functions {
// template <typename T, typename U>
inline void WritePPM_pullback(::std::uint32_t w, ::std::uint32_t h, const ::smallpt::Vector3* Ls,
                      const char* fname,
                      ::std::uint32_t* dw, ::std::uint32_t* dh, ::smallpt::Vector3* dLs,
                      char* dfname) noexcept {
  ::smallpt::ReadPPMToLs(("_d_" + ::std::string(fname)).c_str(), dLs, w * h);
}

void ComputePPMDifference_pullback(const char* fname1, const char* fname2, ::std::uint32_t w, ::std::uint32_t h,
                                     double dy, char* d_fname1, char* d_fname2, ::std::uint32_t* dw, ::std::uint32_t* dh) {
  ::std::size_t count = w * h;
  ::smallpt::Vector3 Ls1[count], Ls2[count], _d_Ls1[count];
  ::smallpt::ReadPPMToLs(fname1, Ls1, count);
  ::smallpt::ReadPPMToLs(fname2, Ls2, count);

  for (::std::size_t i = 0; i < count; ++i) {
      _d_Ls1[i].m_x = 2 * (Ls1[i].m_x - Ls2[i].m_x) * dy;
      _d_Ls1[i].m_y = 2 * (Ls1[i].m_y - Ls2[i].m_y) * dy;
      _d_Ls1[i].m_z = 2 * (Ls1[i].m_z - Ls2[i].m_z) * dy;
  }

  ::smallpt::WritePPM(w, h, _d_Ls1, ("_d_" + ::std::string(fname1)).c_str());
}

}

using smallpt::WritePPM_pullback;
using smallpt::ComputePPMDifference_pullback;
}
}
