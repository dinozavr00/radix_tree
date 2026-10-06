#include <radix_tree.hpp>

#include <algorithm>
#include <ostream>
#include <utility>

namespace lab1 {

RadixTree::~RadixTree() = default;

std::size_t RadixTree::size() const {
    return size_;
}

bool RadixTree::empty() const {
    return size_ == 0;
}

int RadixTree::find_child(const Node& node, char first) {
    // тут просто детей узла по порядку перебираем 
    for (std::size_t i = 0; i < node.children.size(); i++) {
        // ну и тут понятно что если нашли подходящую первую букву то победа
        if (node.children[i]->label[0] == first) {
            return static_cast<int>(i);
        }
}
    return -1;
}

// сколько символов метки label совпадает со строкой s, начиная с позиции pos
std::size_t RadixTree::common_length(const std::string& s, const std::string& label, std::size_t pos) {
    std::size_t len = 0;
    
    while (pos + len < s.size() && 
           len < label.size() && 
           s[pos + len] == label[len]) {
        len++;
    }
    
    return len;
}

// делит узел на два: node получает первые at букв метки, 
// а всё остальное (метка, дети, терминальность) уезжает в нового ребёнка
void RadixTree::split(Node& node, std::size_t at) {
    auto lower = std::make_unique<Node>();
    lower->label = node.label.substr(at);
    lower->is_terminal = node.is_terminal;
    lower->children = std::move(node.children);

    node.label = node.label.substr(0, at);
    node.is_terminal = false;
    node.children.clear();
    node.children.push_back(std::move(lower));
}

bool RadixTree::contains(const std::string& s) const {
    Node* current = root_.get();
    std::size_t pos = 0;
    
    while (pos < s.size()) {
        int child_idx = find_child(*current, s[pos]);
        
        if (child_idx == -1) {
            return false;
        }
        
        Node* child = current->children[child_idx].get();
        std::size_t common = common_length(s, child->label, pos);
        
        // метка длиннее чем осталось в строке или не совпала
        if (common < child->label.size()) {
            return false;
        }
        
        pos += common;
        current = child;
    }
    
    // дошли до конца строки (для пустой строки это корень)
    return current->is_terminal;
}

bool RadixTree::insert(const std::string& s) {
    Node* current = root_.get();
    std::size_t pos = 0;

    while (pos < s.size()) {
        int child_idx = find_child(*current, s[pos]);

        // не нашли одходящего
        if (child_idx == -1) {
            auto new_node = std::make_unique<Node>();
            new_node->label = s.substr(pos); 
            new_node->is_terminal = true;
            current->children.push_back(std::move(new_node)); 
            
            size_++;
            return true;
        }

        Node* child = current->children[child_idx].get();
        std::size_t common = common_length(s, child->label, pos);

        // for ex. "test", вставляем "team"
        // делим "test" на "te" и "st", дальше идём как обычно
        if (common < child->label.size()) {
            split(*child, common);
        }

        pos += common;
        current = child;
    }

    // строка закончилась на узле current (для пустой строки это корень)
    if (current->is_terminal) {
        return false;
    }
    current->is_terminal = true;
    size_++;
    return true;
}

void RadixTree::collect(const Node& node, std::string& path, std::vector<std::string>& result) {
    // node — текущий узел
    // path — склейка всех меток на пути от корня до текущего узла
    // result — вектор, в который складываем найденные слова
    
    if (node.is_terminal) {
        result.push_back(path);
    }
    
    // рекурсия
    for (std::size_t i = 0; i < node.children.size(); i++) {
        std::size_t old_length = path.size();
        path += node.children[i]->label;
        
        collect(*(node.children[i]), path, result);
        
        // нужно откатиь path к режнему состоянию что хваост не приклеивался
        path.resize(old_length);
    }
}

std::vector<std::string> RadixTree::get_by_prefix(const std::string& prefix) const {
    std::vector<std::string> result;
    
    Node* current = root_.get();
    std::string path = "";
    std::size_t pos = 0;
    
    while (pos < prefix.size()) {
        int child_idx = find_child(*current, prefix[pos]);
        
        if (child_idx == -1) {
            return result;
        }
        
        Node* child = current->children[child_idx].get();
        std::size_t common = common_length(prefix, child->label, pos);
        
        // метка не совпала, а префикс ещё не кончился — префикса нет
        if (common < child->label.size() && pos + common < prefix.size()) {
            return result;
        }
        
        // либо метка совпала целиком, либо префикс закончился внутри метки
        path += child->label;
        pos += common;
        current = child;
    }
    
    // current — узел, от которого начинаются все нужные строки
    collect(*current, path, result);
    return result;
}

void RadixTree::merge_with_only_child(Node& node) {
    // забираем ребёнка себе, чтобы он спокойно жил пока мы его разбираем
    std::unique_ptr<Node> only_child = std::move(node.children[0]);
    
    node.label += only_child->label;
    node.is_terminal = only_child->is_terminal;
    node.children = std::move(only_child->children);
}

bool RadixTree::deletestr(const std::string& s) {
    
    // сначала вниз, запоминаем родителя
    Node* parent = nullptr;
    int idx_in_parent = -1;
    Node* current = root_.get();
    std::size_t pos = 0;
    
    while (pos < s.size()) {
        int child_idx = find_child(*current, s[pos]);
        
        if (child_idx == -1) {
            return false; 
        }
        
        Node* child = current->children[child_idx].get();
        std::size_t common = common_length(s, child->label, pos);
        
        // строка заканчивается внутри метки
        if (common < child->label.size()) {
            return false;
        }
        
        parent = current;
        idx_in_parent = child_idx;
        pos += common;
        current = child;
    }
    
    // строки в дереве не было
    if (!current->is_terminal) {
        return false;
    }
    
    current->is_terminal = false;
    size_--;
    
    // пустая строка, это был корень, чистить нечего
    if (parent == nullptr) {
        return true;
    }
    
    if (current->children.empty()) {
        // детей 0 удаляем узел у родителя
        parent->children.erase(parent->children.begin() + idx_in_parent);
        
        // у родителя мог остаться один ребёнок слепляем (корень не трогаем)
        if (parent != root_.get() && !parent->is_terminal && parent->children.size() == 1) {
            merge_with_only_child(*parent);
        }
    } else if (current->children.size() == 1) {
        // дете 1 слепляем
        merge_with_only_child(*current);
    }
    
    return true;
}

RadixTree::RadixTree(std::initializer_list<std::string> strings) {
    for (const std::string& s : strings) {
        insert(s);
    }
}

std::unique_ptr<RadixTree::Node> RadixTree::clone(const Node& source) {
    auto copy = std::make_unique<Node>();
    copy->label = source.label;
    copy->is_terminal = source.is_terminal;

    for (std::size_t i = 0; i < source.children.size(); i++) {
        copy->children.push_back(clone(*source.children[i]));
    }

    return copy;
}

RadixTree::RadixTree(const RadixTree& other)
    : root_(clone(*other.root_)),
      size_(other.size_) {
}

RadixTree& RadixTree::operator=(const RadixTree& other) {
    if (this == &other) {
        return *this;
    }

    root_ = clone(*other.root_);
    size_ = other.size_;

    return *this;
}

bool RadixTree::operator==(const RadixTree& other) const {
    if (size_ != other.size_) {
        return false;
    }
    
    auto strings_this = get_by_prefix("");
    auto strings_other = other.get_by_prefix("");
    
    std::sort(strings_this.begin(), strings_this.end());
    std::sort(strings_other.begin(), strings_other.end());
    
    return strings_this == strings_other;
}

std::ostream& operator<<(std::ostream& os, const RadixTree& tree) {
    auto strings = tree.get_by_prefix("");
    
    std::sort(strings.begin(), strings.end());
    
    for (std::size_t i = 0; i < strings.size(); i++) {
        os << strings[i];
        if (i + 1 < strings.size()) {
            os << "\n";
        }
    }
    
    return os;
}

}  // namespace lab1