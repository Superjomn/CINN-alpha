# Tunner

A tuner helps to findout the good programs performed with the right transformations. 

## Transforms that can be tuned

Currently, CINN supports several basic transforms:

- Tile
- Interchange
- Tile+Unroll
- Vectorize
- Fuse

The different composition of these transforms performed on the original program can generate different performance, and the best one should contained in these choices.

Different choices forms a search space, a transform composition is a point in this space. To identify a point in the space, we use a `Point`.

```c++
struct Point {
  std::unique_ptr<std::vector<Function>> fns;
  std::vector<std::pair<std::string/*stage name*/, std::string/*too*/>> fuse_pairs;
  std::map<std::string, std::vector<int>> tiles;
  std::map<std::string, std::vector<int>> tile_unrolls;
  std::map<std::string, int> vectorizes;
  
	// Evaluate the performance of this point.
  int Evaluate(std::function<);
  
  bool operator==(const Point&);
  
private:
  // Build the functions according to the transform configurations.
  bool BuildFns();
  
}
```

## Search Space

All the `Point`s will form a search space.

```c++
class SearchSpace {
 public:
	SearchSpace(bool enable_fuse, bool enable_unroll, bool enable_tile, bool enable_vectorize);
  
  // Generate a choice, return a boolean value indicating whether there is choice left.
  bool Generate(Point* point);
  
 protected:
  // Return whether a choice is pruned.
  bool Pruned(const Point& point);
};
```

One can traverse the whole search space by some code like

```c++
Point choice;
SearchSpace space(true, true, true, true);

while (space.Generate(&point)) {
  int latency = point.Evaluate();
}
```
