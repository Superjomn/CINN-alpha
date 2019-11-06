#include "cinn/autotuner/point.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace autotuner {

bool Point::operator==(const Point& other) { return false; }

bool Point::BuildFns() {
  BuildFuses();
  BuildTiles();
  BuildTileUnrolls();

  for (auto& fn : *fns) {
    CHECK(!fn.end_definition());
    fn.EndDefinition();
  }
  return true;
}

void Point::CollectResetStages() {
  LOG_INDENT(0);
  for (auto& fn : *fns) {
    fn.ResetDefintion();
    for (auto& stage : fn.stages()) {
      CINN_DEBUG(2) << "add stage " << stage.name();
      stage.ClearTransforms();
      stages_[stage.name()] = &stage;
    }
  }
}

void Point::BuildFuses() {
  for (auto& pair : fuses) {
    auto& stage0 = GetStage(pair.first);
    auto& stage1 = GetStage(pair.second);
    stage0.FuseWith(stage1);
  }
}

void Point::BuildTiles() {
  for (auto& item : tiles) {
    auto& stage = GetStage(item.first);
    stage.Tile(item.second);
  }
}

void Point::BuildTileUnrolls() {
  for (auto& item : tile_unrolls) {
    auto& stage = GetStage(item.first);
    stage.TileUnroll(item.second);
  }
}

Stage& Point::GetStage(const std::string& name) {
  auto it = stages_.find(name);
  CHECK(it != stages_.end()) << "stage " << name << " not exists";
  return *it->second;
}

}  // namespace autotuner
}  // namespace cinn
