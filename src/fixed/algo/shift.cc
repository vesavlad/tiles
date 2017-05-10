#include "tiles/fixed/algo/shift.h"

namespace tiles {

void shift(fixed_null_geometry&, uint32_t const) {}

void shift(fixed_xy& point, uint32_t const delta_z) {
  point.x_ = point.x_ >> delta_z;
  point.y_ = point.y_ >> delta_z;
}

void shift(fixed_polyline& polyline, uint32_t const delta_z) {
  for (auto& line : polyline.geometry_) {
    for (auto& point : line) {
      shift(point, delta_z);
    }
  }
}

void shift(fixed_polygon&, uint32_t const) {}

void shift(fixed_geometry& geometry, uint32_t const z) {
  uint32_t delta_z = 20 - z;
  boost::apply_visitor([&](auto& unpacked) { shift(unpacked, delta_z); },
                       geometry);
}

}  // namespace tiles