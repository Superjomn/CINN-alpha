#include "cinn/hlir/graph_util.h"
#include <algorithm>
#include <utility>

namespace cinn {
namespace hlir {

//
// NodesDFSIterator
//
NodesDFSIterator::NodesDFSIterator(const std::vector<Node *> &source) {
  for (auto *x : source) stack_.push(x);
}

NodesDFSIterator::NodesDFSIterator(NodesDFSIterator &&other) noexcept
    : stack_(std::move(other.stack_)), visited_(std::move(other.visited_)) {}

NodesDFSIterator::NodesDFSIterator(const NodesDFSIterator &other) : stack_(other.stack_), visited_(other.visited_) {}

Node &NodesDFSIterator::operator*() {
  CHECK(!stack_.empty());
  return *stack_.top();
}

NodesDFSIterator &NodesDFSIterator::operator++() {
  CHECK(!stack_.empty()) << "the iterator exceeds range";
  visited_.insert(stack_.top());
  auto *cur = stack_.top();
  stack_.pop();
  for (auto &x : cur->outlinks) {
    if (!visited_.count(x)) {
      stack_.push(x);
    }
  }
  return *this;
}
bool NodesDFSIterator::operator==(const NodesDFSIterator &other) {
  if (stack_.empty()) return other.stack_.empty();
  if ((!stack_.empty()) && (!other.stack_.empty())) {
    return stack_.top() == other.stack_.top();
  }
  return false;
}

NodesDFSIterator &NodesDFSIterator::operator=(const NodesDFSIterator &other) {
  stack_ = other.stack_;
  visited_ = other.visited_;
  return *this;
}
Node *NodesDFSIterator::operator->() { return stack_.top(); }

inline bool CheckNodeIndegreeEquals(const Node &node, size_t n) { return node.inlinks.size() == n; }

NodesTSIterator::NodesTSIterator(const std::vector<Node *> &source) {
  CHECK(!source.empty()) << "Start points of topological sorting should not be empty!";
  // CHECK all the inputs' in-degree is 0
  for (auto *node : source) {
    CHECK(CheckNodeIndegreeEquals(*node, 0));
  }

  std::set<Node *> to_visit{source.begin(), source.end()};
  std::vector<Node *> inlink_sorted;
  while (!to_visit.empty()) {
    std::vector<Node *> queue(to_visit.begin(), to_visit.end());
    for (auto *p : queue) {
      to_visit.erase(p);
      sorted_.push_back(p);
      for (auto *out : p->outlinks) {
        inlink_sorted.clear();
        std::copy_if(out->inlinks.begin(), out->inlinks.end(), std::back_inserter(inlink_sorted), [&](Node *x) -> bool {
          return std::find(sorted_.begin(), sorted_.end(), x) != sorted_.end();
        });
        if (inlink_sorted.size() == out->inlinks.size()) {
          to_visit.insert(out);
        }
      }
    }
  }
}

NodesTSIterator::NodesTSIterator(const NodesTSIterator &other) : sorted_(other.sorted_), cursor_(other.cursor_) {}

Node &NodesTSIterator::operator*() {
  CHECK_LT(cursor_, sorted_.size());
  return *sorted_[cursor_];
}

NodesTSIterator &NodesTSIterator::operator++() {
  if (++cursor_ >= sorted_.size()) {
    sorted_.clear();
    cursor_ = 0;
  }
  return *this;
}
NodesTSIterator &NodesTSIterator::operator=(const NodesTSIterator &other) {
  cursor_ = other.cursor_;
  sorted_ = other.sorted_;
  return *this;
}

bool NodesTSIterator::operator==(const NodesTSIterator &other) {
  return sorted_ == other.sorted_ && cursor_ == other.cursor_;
}

Node *NodesTSIterator::operator->() {
  CHECK_LT(cursor_, sorted_.size());
  return sorted_[cursor_];
}

}  // namespace hlir
}  // namespace cinn
