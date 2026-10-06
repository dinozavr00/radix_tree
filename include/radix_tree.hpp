#pragma once

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace lab1 {

class RadixTree {
public:
    RadixTree() = default;
    RadixTree(std::initializer_list<std::string> strings);
    RadixTree(const RadixTree& other);
    RadixTree& operator=(const RadixTree& other);
    ~RadixTree();

    bool contains(const std::string& s) const;
    bool insert(const std::string& s);
	   bool deletestr(const std::string& s);
    std::vector<std::string> get_by_prefix(const std::string& prefix) const;
    std::size_t size() const;
    bool empty() const;

    bool operator==(const RadixTree& other) const;

private:
    struct Node {
        std::string label;
        bool is_terminal = false;
        std::vector<std::unique_ptr<Node>> children;
    };

    static int find_child(const Node& node, char first);
    static std::size_t common_length(const std::string& label,
                                     const std::string& s, std::size_t pos);
    static void split(Node& node, std::size_t at);
    static void merge_with_only_child(Node& node);
    static void collect(const Node& node, std::string& path,
                        std::vector<std::string>& result);
    static std::unique_ptr<Node> clone(const Node& source);

    std::unique_ptr<Node> root_ = std::make_unique<Node>();
    std::size_t size_ = 0;
};



std::ostream& operator<<(std::ostream& os, const RadixTree& tree);

}  // namespace lab1