#ifndef CLAD_DIFFERENTIATOR_RESTORETRACKER_H
#define CLAD_DIFFERENTIATOR_RESTORETRACKER_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

namespace clad {

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
class restore_tracker {
  std::map<char*, std::vector<uint8_t>> m_data;

public:
  template <typename T> void store(const T& val) {
    // Don't overwrite
    if (m_data.find((char*)&val) != m_data.end())
      return;
    std::vector<uint8_t> buffer(sizeof(T));
    std::memcpy(buffer.data(), &val, sizeof(T));
    m_data.emplace((char*)&val, std::move(buffer));
  }
  void restore() {
    for (auto& pair : m_data) {
      std::vector<uint8_t>& buffer = pair.second;
      std::memcpy(pair.first, buffer.data(), buffer.size());
    }
    m_data.clear();
  }
};
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
} // namespace clad

#endif // CLAD_DIFFERENTIATOR_RESTORETRACKER_H
