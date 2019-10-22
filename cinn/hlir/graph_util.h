#pragma once
#include <glog/logging.h>
#include <map>
#include <set>
#include <stack>
#include <utility>
#include <vector>
#include "cinn/hlir/graph.h"

namespace cinn {
namespace hlir {

template <typename IteratorT>
class iterator_range {
  IteratorT begin_, end_;

 public:
  template <typename Container>
  explicit iterator_range(Container &&c) : begin_(c.begin()), end_(c.end()) {}

  iterator_range(const IteratorT &begin, const IteratorT &end) : begin_(begin), end_(end) {}

  const IteratorT &begin() const { return begin_; }
  const IteratorT &end() const { return end_; }
};

// DFS iterator on nodes.
struct NodesDFSIterator : public std::iterator<std::forward_iterator_tag, Node *> {
  NodesDFSIterator() = default;
  explicit NodesDFSIterator(const std::vector<Node *> &source);
  NodesDFSIterator(NodesDFSIterator &&other) noexcept;
  NodesDFSIterator(const NodesDFSIterator &other);

  Node &operator*();
  NodesDFSIterator &operator++();
  // TODO(Superjomn) current implementation just compare the first
  // element, need to compare the graph and all the elements in the queue and
  // set.
  NodesDFSIterator &operator=(const NodesDFSIterator &other);
  bool operator==(const NodesDFSIterator &other);
  bool operator!=(const NodesDFSIterator &other) { return !(*this == other); }
  Node *operator->();

 private:
  std::stack<Node *> stack_;
  std::set<Node *> visited_;
};

// Topological sorting iterator on nodes.
struct NodesTSIterator : public std::iterator<std::forward_iterator_tag, Node *> {
  NodesTSIterator() = default;
  explicit NodesTSIterator(const std::vector<Node *> &source);
  NodesTSIterator(NodesTSIterator &&other) : sorted_(std::move(other.sorted_)), cursor_(other.cursor_) {
    other.cursor_ = 0;
  }
  NodesTSIterator(const NodesTSIterator &other);

  Node &operator*();
  NodesTSIterator &operator++();
  // TODO(Superjomn) current implementation just compare the first
  // element, need to compare the graph and all the elements in the queue and
  // set.
  NodesTSIterator &operator=(const NodesTSIterator &other);
  bool operator==(const NodesTSIterator &other);
  bool operator!=(const NodesTSIterator &other) { return !(*this == other); }
  Node *operator->();

 private:
  std::vector<Node *> sorted_;
  size_t cursor_{0};
};

/*
 * GraphTraits contains some graph traversal algorithms.
 *
 * Usage:
 *
 */
struct GraphTraits {
  static iterator_range<NodesDFSIterator> DFS(const Graph &g) {
    auto start_points = ExtractStartPoints(g);
    NodesDFSIterator x(start_points);
    return iterator_range<NodesDFSIterator>(NodesDFSIterator(start_points), NodesDFSIterator());
  }

  static iterator_range<NodesTSIterator> TS(const Graph &g) {
    auto start_points = ExtractStartPoints(g);
    CHECK(!start_points.empty());
    NodesTSIterator x(start_points);
    return iterator_range<NodesTSIterator>(NodesTSIterator(start_points), NodesTSIterator());
  }

 private:
  // The nodes those have no input will be treated as start points.
  static std::vector<Node *> ExtractStartPoints(const Graph &g) {
    std::vector<Node *> result;
    for (auto &node : g.nodes()) {
      if (node->inlinks.empty()) {
        result.push_back(node.get());
      }
    }
    return result;
  }
};

}  // namespace hlir
}  // namespace cinn
