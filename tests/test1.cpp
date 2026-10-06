#include <catch2/catch_test_macros.hpp>
#include <radix_tree.hpp>

#include <algorithm>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace lab1;

// ============================================================================
// Вспомогательные функции для сверки дерева с эталоном
// ============================================================================

namespace {

// Все строки дерева в отсортированном виде
std::vector<std::string> sorted_contents(const RadixTree& tree) {
    auto v = tree.get_by_prefix("");
    std::sort(v.begin(), v.end());
    return v;
}

// Начинается ли s с p
bool starts_with(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

// Эталонный ответ на get_by_prefix: прямой перебор множества
std::vector<std::string> brute_prefix(const std::set<std::string>& ref,
                                      const std::string& p) {
    std::vector<std::string> out;
    for (const auto& s : ref) {          // set перебирается в sorted-порядке
        if (starts_with(s, p)) {
            out.push_back(s);
        }
    }
    return out;
}

}  // namespace

// ============================================================================
// Граничные случаи: пустая строка, одиночные символы
// ============================================================================

TEST_CASE("empty string and single characters", "[edge]") {
    RadixTree tree;

    REQUIRE(tree.insert(""));
    REQUIRE(tree.insert("a"));
    REQUIRE(tree.insert("b"));
    REQUIRE_FALSE(tree.insert(""));   // дубликат пустой строки
    REQUIRE(tree.size() == 3);

    REQUIRE(tree.contains(""));
    REQUIRE(tree.get_by_prefix("").size() == 3);
    REQUIRE(tree.get_by_prefix("a").size() == 1);

    REQUIRE(tree.deletestr(""));
    REQUIRE_FALSE(tree.contains(""));
    REQUIRE(tree.size() == 2);
    REQUIRE(tree.get_by_prefix("").size() == 2);
}

// ============================================================================
// Цепочки строк-префиксов: удаление с обоих концов
// ============================================================================

TEST_CASE("chain of prefixes: delete from both ends", "[edge]") {
    RadixTree tree;

    // Строим цепочку: "a", "ab", "abc", ... каждая следующая длиннее на 1 символ
    std::vector<std::string> chain;
    std::string cur;
    for (int i = 0; i < 12; ++i) {
        cur += static_cast<char>('a' + i % 4);
        chain.push_back(cur);
    }

    for (const auto& w : chain) REQUIRE(tree.insert(w));
    REQUIRE(tree.size() == chain.size());
    for (const auto& w : chain) REQUIRE(tree.contains(w));
    REQUIRE(tree.get_by_prefix(chain[0]).size() == chain.size());

    // Удаляем с КОРОТКОГО конца: длинные обязаны выживать
    for (std::size_t i = 0; i < chain.size(); ++i) {
        REQUIRE(tree.deletestr(chain[i]));
        REQUIRE_FALSE(tree.contains(chain[i]));
        REQUIRE(tree.size() == chain.size() - i - 1);
        for (std::size_t j = i + 1; j < chain.size(); ++j) {
            REQUIRE(tree.contains(chain[j]));
        }
    }
    REQUIRE(tree.empty());

    // Заново и удаляем с ДЛИННОГО конца
    for (const auto& w : chain) tree.insert(w);
    for (std::size_t i = chain.size(); i-- > 0;) {
        REQUIRE(tree.deletestr(chain[i]));
        REQUIRE(tree.size() == i);
        for (std::size_t j = 0; j < i; ++j) {
            REQUIRE(tree.contains(chain[j]));
        }
    }
    REQUIRE(tree.empty());
}

// ============================================================================
// Глубокая цепочка из одинаковых букв: нагрузка на склейку узлов
// ============================================================================

TEST_CASE("same-character deep chain", "[edge]") {
    RadixTree tree;

    for (int k = 1; k <= 30; ++k) {
        REQUIRE(tree.insert(std::string(k, 'a')));
    }
    REQUIRE(tree.size() == 30);
    REQUIRE(tree.get_by_prefix(std::string(3, 'a')).size() == 28);  // длины 3..30

    REQUIRE(tree.deletestr(std::string(5, 'a')));
    REQUIRE(tree.size() == 29);
    REQUIRE_FALSE(tree.contains(std::string(5, 'a')));
    REQUIRE(tree.get_by_prefix(std::string(4, 'a')).size() == 26);  // 4, 6..30
    REQUIRE(tree.get_by_prefix(std::string(6, 'a')).size() == 25);  // 6..30

    REQUIRE(tree.deletestr(std::string(30, 'a')));
    REQUIRE(tree.get_by_prefix(std::string(29, 'a')).size() == 1);
}

// ============================================================================
// Сценарии из методички (рис. 5-6) + склейка при удалении
// ============================================================================

TEST_CASE("lab figures 5-6 and merges on delete", "[lab]") {
    RadixTree tree;

    tree.insert("test");
    tree.insert("team");
    REQUIRE(tree.size() == 2);
    tree.insert("toast");
    REQUIRE(tree.size() == 3);
    REQUIRE(tree.get_by_prefix("te").size() == 2);
    REQUIRE(tree.get_by_prefix("to").size() == 1);
    REQUIRE(tree.get_by_prefix("t").size() == 3);

    REQUIRE(tree.deletestr("toast"));  // узел "t" должен склеиться обратно
    REQUIRE(tree.size() == 2);
    REQUIRE(tree.get_by_prefix("t").size() == 2);
    REQUIRE(tree.contains("test"));
    REQUIRE(tree.contains("team"));

    REQUIRE(tree.deletestr("team"));   // склейка "te" + "st" -> "test"
    REQUIRE(tree.size() == 1);
    REQUIRE(tree.contains("test"));
    REQUIRE(tree.get_by_prefix("te").size() == 1);

    REQUIRE(tree.deletestr("test"));
    REQUIRE(tree.empty());
    REQUIRE(tree.get_by_prefix("t").empty());
}

// ============================================================================
// get_by_prefix против прямого перебора (brute force)
// ============================================================================

TEST_CASE("get_by_prefix brute-force comparison", "[prefix]") {
    const std::vector<std::string> words{"", "a", "ab", "abc", "abcd",
                                         "abd", "b", "ba", "x"};
    RadixTree tree;
    for (const auto& w : words) tree.insert(w);

    const std::vector<std::string> prefixes{
        "", "a", "ab", "abc", "abcd", "abcde", "abce", "abd",
        "abdx", "b", "ba", "bab", "x", "xx", "c"};

    for (const auto& p : prefixes) {
        auto got = tree.get_by_prefix(p);
        std::sort(got.begin(), got.end());

        std::vector<std::string> expected;
        for (const auto& w : words) {
            if (starts_with(w, p)) expected.push_back(w);
        }
        REQUIRE(got == expected);  // words уже отсортирован
    }
}

// ============================================================================
// Удаление во ВСЕХ порядках: после каждого шага сверка с эталоном
// ============================================================================

TEST_CASE("delete in every order leaves correct tree", "[deletestr]") {
    const std::vector<std::string> words{"", "a", "aa", "aab", "aaaa",
                                         "ab", "aba", "abb", "b", "ba"};

    for (unsigned seed = 0; seed < 3; ++seed) {
        std::vector<std::string> order = words;
        if (seed == 1) {
            std::reverse(order.begin(), order.end());
        } else if (seed == 2) {
            std::mt19937 rng(seed);
            std::shuffle(order.begin(), order.end(), rng);
        }

        RadixTree tree;
        std::set<std::string> ref;
        for (const auto& w : words) {
            tree.insert(w);
            ref.insert(w);
        }

        for (const auto& w : order) {
            REQUIRE(tree.deletestr(w) == (ref.erase(w) > 0));
            REQUIRE(tree.size() == ref.size());
            REQUIRE(sorted_contents(tree) ==
                    std::vector<std::string>(ref.begin(), ref.end()));
        }

        REQUIRE(tree.empty());
        REQUIRE(tree.get_by_prefix("").empty());

        // Воскрешение: повторная вставка даёт то же дерево
        for (const auto& w : words) tree.insert(w);
        RadixTree fresh;
        for (const auto& w : words) fresh.insert(w);
        REQUIRE(tree == fresh);
    }
}

// ============================================================================
// Копия обязана пережить жестокие мутации оригинала
// ============================================================================

TEST_CASE("copy survives mutations of original", "[copy]") {
    std::mt19937 rng(7);
    auto rnd_str = [&]() {
        std::string s;
        std::size_t len = 1 + rng() % 6;
        for (std::size_t i = 0; i < len; ++i) {
            s += static_cast<char>('a' + rng() % 4);
        }
        return s;
    };

    RadixTree original;
    for (int i = 0; i < 300; ++i) original.insert(rnd_str());
    const auto snapshot = sorted_contents(original);

    RadixTree copy(original);
    REQUIRE(sorted_contents(copy) == snapshot);

    // Ломаем оригинал: удаляем всё на 'a', добавляем мусор
    for (const auto& w : snapshot) {
        if (!w.empty() && w[0] == 'a') original.deletestr(w);
    }
    for (int i = 0; i < 100; ++i) original.insert(rnd_str() + "zz");

    // Копия не должна измениться
    REQUIRE(sorted_contents(copy) == snapshot);
    REQUIRE_FALSE(original == copy);

    RadixTree third;
    third = copy;
    REQUIRE(sorted_contents(third) == snapshot);

    RadixTree& self = third;
    third = self;  // самоприсваивание
    REQUIRE(sorted_contents(third) == snapshot);
}

// ============================================================================
// operator== не зависит от порядка вставки
// ============================================================================

TEST_CASE("operator== ignores insertion order", "[equality]") {
    const std::vector<std::string> words{"test", "team", "toast", "te", "t", ""};
    RadixTree base;
    for (const auto& w : words) base.insert(w);

    for (unsigned seed = 1; seed <= 5; ++seed) {
        auto order = words;
        std::mt19937 rng(seed);
        std::shuffle(order.begin(), order.end(), rng);

        RadixTree other;
        for (const auto& w : order) other.insert(w);
        REQUIRE(other == base);
        REQUIRE(base == other);
    }

    // удаление + повторная вставка не ломают равенство
    RadixTree mutated = base;
    mutated.deletestr("team");
    mutated.insert("team");
    REQUIRE(mutated == base);
}

// ============================================================================
// operator<<: точная проверка вывода посимвольно
// ============================================================================

TEST_CASE("operator<< prints sorted contents exactly", "[output]") {
    SECTION("empty tree prints nothing") {
        RadixTree t;
        std::ostringstream oss;
        oss << t;
        REQUIRE(oss.str().empty());
    }

    SECTION("tree with empty string inside") {
        RadixTree t{"b", "a", ""};
        std::ostringstream oss;
        oss << t;
        REQUIRE(oss.str() == "\na\nb");  // отсортировано: "", "a", "b"
    }

    SECTION("single word without trailing newline") {
        RadixTree t{"x"};
        std::ostringstream oss;
        oss << t;
        REQUIRE(oss.str() == "x");
    }
}

// ============================================================================
// Мульти-seed стресс: сверка с std::set после КАЖДОЙ операции
// ============================================================================

TEST_CASE("multi-seed stress vs std::set", "[stress]") {
    for (unsigned seed : {1u, 2u, 3u, 42u}) {
        std::mt19937 rng(seed);
        RadixTree tree;
        std::set<std::string> ref;

        auto rnd_str = [&]() {
            std::string s;
            std::size_t len = rng() % 7;  // иногда пустая строка
            for (std::size_t i = 0; i < len; ++i) {
                s += static_cast<char>('a' + rng() % 3);
            }
            return s;
        };

        for (int op = 0; op < 3000; ++op) {
            int kind = static_cast<int>(rng() % 100);
            std::string s = rnd_str();

            if (kind < 45) {
                REQUIRE(tree.insert(s) == ref.insert(s).second);
            } else if (kind < 80) {
                REQUIRE(tree.deletestr(s) == (ref.erase(s) > 0));
            } else {
                REQUIRE(tree.contains(s) == (ref.count(s) == 1));
            }

            // Инварианты размера — после каждой операции
            REQUIRE(tree.size() == ref.size());
            REQUIRE(tree.empty() == ref.empty());

            // Периодически полное содержимое + отсутствие дублей
            if (op % 97 == 0) {
                auto contents = sorted_contents(tree);
                REQUIRE(contents == std::vector<std::string>(ref.begin(), ref.end()));
                REQUIRE(tree.size() == contents.size());
            }
        }

        // Финальная сверка содержимого
        REQUIRE(sorted_contents(tree) ==
                std::vector<std::string>(ref.begin(), ref.end()));

        // Финальная сверка префиксов против brute force
        for (int i = 0; i < 40; ++i) {
            std::string p = rnd_str();
            auto got = tree.get_by_prefix(p);
            std::sort(got.begin(), got.end());
            REQUIRE(got == brute_prefix(ref, p));
        }
    }
}