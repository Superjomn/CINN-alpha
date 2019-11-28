#pragma once
/**
 * CinnContext helps to manage all the global information and avoid process wide singleton.
 */

#include <glog/logging.h>
#include <isl/cpp.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/name_generator.h"

namespace cinn {

class Stage;

/**
 * @brief Base class of all the generators.
 *
 * Generator is an resource manager across the whole thread concerning the code generation phase.
 */
class Generator {
 public:
  // the isl ctx accross the whole thread.
  isl_ctx* ctx() { return isl_utils::global_isl_ctx(); }

  //! Get a stage by its name.
  Stage GetStageByName(const std::string& name);
  //! Register a stage.
  void RegisterStage(const std::string& name, const Stage& x);
  //! Given a domain, collect all the stages those iterator domains intersect it.
  std::vector<Stage> FilterStagesByDomain(const isl::set& domain);

  Stage GetComputationByNode(isl_ast_node* node);

  size_t num_stages() const { return stages_.size(); }

  ~Generator();

 private:
  Generator() : ctx_(isl_ctx_alloc()) {}

  isl::ctx ctx_{nullptr};
  std::map<std::string, Stage*> stages_;

  friend class CINNContext;
};

struct OnceCallStageRegistry {
  void Register(const std::string& stage) { stages_.insert(stage); }
  bool Contains(const std::string& name) const { return stages_.count(name); }
  const std::set<std::string>& stages() const { return stages_; };

 private:
  std::set<std::string> stages_;
};

class CINNContext {
 public:
  NameGenerator& name_generator() { return name_generator_; }
  Generator& generator() { return generator_; }
  OnceCallStageRegistry& once_call_registry() { return once_call_registry_; }

 private:
  NameGenerator name_generator_;
  Generator generator_;
  OnceCallStageRegistry once_call_registry_;
};

extern std::unique_ptr<CINNContext> _g_cinn_context;

void SetGlobalContext(CINNContext* context);

CINNContext& GlobalContext();

}  // namespace cinn
