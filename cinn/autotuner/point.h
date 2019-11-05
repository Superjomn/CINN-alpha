#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "cinn/core/function.h"

namespace cinn {
namespace autotuner {

/**
 * Point represents a choice in the search space.
 */
struct Point {
 private:
  std::vector<Function>* fns;
  // transforms
  std::vector<std::pair<std::string /*stage name*/, std::string /*stage name*/>> fuses;
  std::map<std::string, std::vector<int>> tiles;
  std::map<std::string, std::vector<int>> tile_unrolls;
  std::map<std::string, int> vectorizes;

 public:
  explicit Point(std::vector<Function>* fns) : fns(fns) { CollectResetStages(); }

  void Fuse(const std::string& s0, const std::string& s1) { fuses.push_back(std::make_pair(s0, s1)); }
  void Tile(const std::string& stage, const std::vector<int>& size) { tiles.emplace(stage, size); }
  void TileUnroll(const std::string& stage, const std::vector<int>& size) { tile_unrolls.emplace(stage, size); }
  void Vectorize(const std::string& stage, int size) { vectorizes.emplace(stage, size); }

  //! Build the functions according the transform configurations.
  bool BuildFns();

  bool operator==(const Point& other);

 protected:
  void BuildFuses();
  void BuildTiles();
  void BuildTileUnrolls();

  Stage& GetStage(const std::string& name);

 private:
  //! Collect stages from the functions.
  void CollectResetStages();

  // All the stages.
  std::map<std::string, Stage*> stages_;
};

}  // namespace autotuner
}  // namespace cinn
