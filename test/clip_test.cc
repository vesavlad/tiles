#include "catch.hpp"

#include "tiles/fixed/algo/clip.h"

using namespace tiles;

tile_spec test_tile_spec(int64_t const minx, int64_t const miny,
                         int64_t const maxx, int64_t const maxy) {
  tile_spec spec{0, 0, 0};
  spec.pixel_bounds_ = {minx, miny, maxx, maxy};
  return spec;
}

TEST_CASE("fixed point clip") {
  auto spec = test_tile_spec(10, 10, 20, 20);

  auto const null_index = fixed_geometry{fixed_null_geometry{}}.which();
  auto const point_index = fixed_geometry{fixed_xy{}}.which();

  {
    fixed_xy test_case{42, 23};
    auto result = clip(test_case, spec);
    REQUIRE(result.which() == null_index);
  }

  {
    fixed_xy test_case{15, 15};
    auto result = clip(test_case, spec);
    REQUIRE(result.which() == point_index);
    CHECK(boost::get<fixed_xy>(result) == test_case);
  }

  {
    fixed_xy test_case{10, 10};
    auto result = clip(test_case, spec);
    REQUIRE(result.which() == point_index);
    CHECK(boost::get<fixed_xy>(result) == test_case);
  }

  {
    fixed_xy test_case{20, 12};
    auto result = clip(test_case, spec);
    REQUIRE(result.which() == point_index);
    CHECK(boost::get<fixed_xy>(result) == test_case);
  }
}


TEST_CASE("fixed point polyline") {
  auto spec = test_tile_spec(10, 10, 20, 20);

  auto const null_index = fixed_geometry{fixed_null_geometry{}}.which();
  auto const polyline_index = fixed_geometry{fixed_polyline{}}.which();

  {
    fixed_polyline input{{{{0, 0}, {0, 30}}}};
    auto result = clip(input, spec);
    REQUIRE(result.which() == null_index);
  }

  {
    fixed_polyline input{{{{12, 12}, {18, 18}}}};
    auto result = clip(input, spec);
    REQUIRE(result.which() == polyline_index);
    CHECK(boost::get<fixed_polyline>(result) == input);
  }

  {
    fixed_polyline input{{{{12, 8}, {12, 12}}}};
    auto result = clip(input, spec);
    REQUIRE(result.which() == polyline_index);


    fixed_polyline expected{{{{12, 10}, {12, 12}}}};
    CHECK(boost::get<fixed_polyline>(result) == expected);
  }

}