#include "tiles/db/database_stats.h"

#include <iostream>
#include <numeric>

#include "fmt/core.h"
#include "fmt/ostream.h"

#include "lmdb/lmdb.hpp"

#include "tiles/db/tile_database.h"
#include "tiles/db/tile_index.h"
#include "tiles/util.h"

namespace tiles {

void database_stats(tile_db_handle& handle) {
  auto const format_num = [](auto& os, char const* label, double const n) {
    auto const k = n / 1e3;
    auto const m = n / 1e6;
    auto const g = n / 1e9;
    if (n < 1e3) {
      fmt::print(os, "{}: {:>6}  ", label, n);
    } else if (k < 1e3) {
      fmt::print(os, "{}: {:>6.1f}K ", label, k);
    } else if (m < 1e3) {
      fmt::print(os, "{}: {:>6.1f}M ", label, m);
    } else {
      fmt::print(os, "{}: {:>6.1f}G ", label, g);
    }
  };

  auto const format_bytes = [](auto& os, char const* label, double const n) {
    auto const k = n / 1024;
    auto const m = n / (1024 * 1024);
    auto const g = n / (1024 * 1024 * 1024);
    if (n < 1024) {
      fmt::print(os, "{}: {:>7.2f}B  ", label, n);
    } else if (k < 1024) {
      fmt::print(os, "{}: {:>7.2f}KB ", label, k);
    } else if (m < 1024) {
      fmt::print(os, "{}: {:>7.2f}MB ", label, m);
    } else {
      fmt::print(os, "{}: {:>7.2f}GB ", label, g);
    }
  };

  auto const print_stat = [&](auto& os, char const* label, auto const& stat) {
    fmt::print(std::cout, "{:<14} > ", label);
    format_bytes(os, "page", stat.ms_psize);
    format_num(os, "depth", stat.ms_depth);
    format_num(os, "branch", stat.ms_branch_pages);
    format_num(os, "leafs", stat.ms_leaf_pages);
    format_num(os, "oflow", stat.ms_overflow_pages);
    format_num(os, "numdat", stat.ms_entries);
    std::cout << "\n";
  };

  auto const print_sizes = [&](auto const& label, auto& m) {
    auto const sum = std::accumulate(begin(m), end(m), 0.);
    std::sort(begin(m), end(m));

    fmt::print(std::cout, "{:<14} > ", label);
    format_num(std::cout, "cnt", m.size());
    format_bytes(std::cout, "sum", sum);

    if (m.empty()) {
      std::cout << "\n";
      return;
    }

    format_bytes(std::cout, "mean", sum / m.size());
    format_bytes(std::cout, "q95", m[m.size() * .95]);
    format_bytes(std::cout, "max", m.back());
    std::cout << "\n";
  };

  auto txn = lmdb::txn{handle.env_};

  auto features_dbi = handle.features_dbi(txn);
  auto tiles_dbi = handle.tiles_dbi(txn);
  auto meta_dbi = handle.meta_dbi(txn);

  std::cout << ">> lmdb stat:\n";
  print_stat(std::cout, "lmdb:env", handle.env_.stat());
  print_stat(std::cout, " dbi:features", features_dbi.stat());
  print_stat(std::cout, " dbi:tiles", tiles_dbi.stat());
  print_stat(std::cout, " dbi:meta", meta_dbi.stat());
  std::cout << "\n";

  std::vector<size_t> feature_sizes;

  auto fc = lmdb::cursor{txn, features_dbi};
  for (auto el = fc.get<tile_index_t>(lmdb::cursor_op::FIRST); el;
       el = fc.get<tile_index_t>(lmdb::cursor_op::NEXT)) {
    feature_sizes.emplace_back(el->second.size());
  }

  std::cout << ">> payload stats:\n";
  print_sizes("features", feature_sizes);

  auto opt_max_prep = txn.get(meta_dbi, kMetaKeyMaxPreparedZoomLevel);
  if (!opt_max_prep) {
    std::cout << "no tiles prepared!" << std::endl;
    return;
  }

  uint32_t max_prep = std::stoi(std::string{*opt_max_prep});
  std::vector<std::vector<size_t>> tile_sizes(max_prep + 1);

  auto tc = lmdb::cursor{txn, tiles_dbi};
  for (auto el = tc.get<tile_index_t>(lmdb::cursor_op::FIRST); el;
       el = tc.get<tile_index_t>(lmdb::cursor_op::NEXT)) {
    auto const& tile = tile_key_to_tile(el->first);
    verify(tile.z_ <= max_prep, "tile outside prepared range found!");
    tile_sizes.at(tile.z_).emplace_back(el->second.size());
  }

  auto total = std::accumulate(begin(feature_sizes), end(feature_sizes), 0ull);
  for (auto z = 0u; z < tile_sizes.size(); ++z) {
    print_sizes(fmt::format("tiles[z={:0>2}]", z), tile_sizes[z]);
    total += std::accumulate(begin(tile_sizes[z]), end(tile_sizes[z]), 0ull);
  }

  std::cout << "====\n";
  format_bytes(std::cout, "total", total);
  std::cout << "\n\n";
}

}  // namespace tiles
