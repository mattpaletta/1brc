#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <locale>
#include <string>
#include <memory>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <array>

struct Measurement {
    using Value = float;
    using Counter = std::size_t;
    Value min, max;
    std::size_t count;
    Value sum;

    Measurement() : min(INFINITY), max(-INFINITY), count(0), sum(0) {}
    Measurement(Value min_, Value max_, Counter count_, Value sum_) : min(min_), max(max_), count(count_), sum(sum_) {}
};

class MeasurementTree {
public:
    class MeasurementNode {
    public:
        using MeasurementPair = std::pair<std::string, Measurement>;

        MeasurementNode(MeasurementPair pair) : value(std::move(pair)) {}

        std::unique_ptr<MeasurementNode> l, r;
        MeasurementPair value;
    };

    MeasurementTree() = default;

    void Emplace(std::string name, Measurement measurement) {
        auto newValue = std::make_unique<MeasurementNode>(MeasurementNode::MeasurementPair(name, std::move(measurement)));
        if (!this->_head) {
            this->_head = std::move(newValue);
        }

        // Insert to unbalanced binary tree.
        auto* value = this->_head.get();
        while (true) {
            const auto isLess = this->locale(name, value->value.first);
            if (isLess) {
                if (value->l) {
                    value = value->l.get();
                } else {
                    value->l = std::move(newValue);
                    return;
                }
            } else {
                if (value->r) {
                    value = value->r.get();
                } else {
                    value->r = std::move(newValue);
                    return;
                }
            }
        }
    }

    Measurement* Get(const std::string& name) {
        if (!this->_head) {
            return nullptr;
        }

        // Insert to unbalanced binary tree.
        auto* value = this->_head.get();
        while (true) {
            if (value->value.first == name) {
                return &value->value.second;
            }

            const auto isLess = this->locale(name, value->value.first);
            if (isLess) {
                if (value->l) {
                    value = value->l.get();
                } else {
                    return nullptr;
                }
            } else {
                if (value->r) {
                    value = value->r.get();
                } else {
                    return nullptr;
                }
            }
        }

        return nullptr;
    }

    auto ToArray() const {
        std::vector<MeasurementNode::MeasurementPair> pair;
        if (!this->_head) {
            return pair;
        }

        std::stack<MeasurementNode*> stack;
        stack.push(this->_head.get());

        while (!stack.empty()) {
            auto value = stack.top();
            stack.pop();

            if (value->l) {
                stack.push(value->l.get());
            }
            if (value->r) {
                stack.push(value->r.get());
            }
            pair.push_back(value->value);
        }

        std::sort(pair.begin(), pair.end(), [&](const auto& a, const auto& b) {
            return this->locale(a.first, b.first);
        });
        return pair;
    }

private:
    std::unique_ptr<MeasurementNode> _head;
    std::locale locale = std::locale("en_US.utf8");
};

// using MeasurementMap = std::unordered_map<std::string, Measurement>;

static auto ParseNumber(const std::string_view& number)-> float {
    const bool isNegative = number.at(0) == '-';
    const bool is4Digit = number.size() == (4 + (isNegative ? 1 : 0));

    auto negateValue = [&](int value) {
        return (value ^ -isNegative) + isNegative;
    };

    constexpr static auto ASCII_TO_NUMBER_CONSTANT = 48;
    if (is4Digit) {
        const auto firstCharacter = ((int) number.at(isNegative ? 1 : 0)) - ASCII_TO_NUMBER_CONSTANT;
        const auto secondCharacter = ((int) number.at(1 + (isNegative ? 1 : 0))) - ASCII_TO_NUMBER_CONSTANT;
        const auto lastCharacter = ((int) number.back()) - ASCII_TO_NUMBER_CONSTANT;

        // Put each character in its place. (as an integer)
        const auto x0 = firstCharacter * 100; // _X__.0
        const auto x1 = secondCharacter * 10; // __X.0
        const auto x2 = lastCharacter; // __X.0

        // Add to merge the number.
        // https://graphics.stanford.edu/~seander/bithacks.html#ConditionalNegate
        const auto value = x0 + x1 + x2;
        return negateValue(value) / 10.0f;

    } else {
        const auto firstCharacter = ((int) number.at(isNegative ? 1 : 0)) - ASCII_TO_NUMBER_CONSTANT;
        const auto lastCharacter = ((int) number.back()) - ASCII_TO_NUMBER_CONSTANT;

        // Put each character in its place. (as an integer)
        const auto x0 = firstCharacter * 10; // _X_.0
        const auto x1 = lastCharacter; // __X.0

        // Add to merge the number.
        // https://graphics.stanford.edu/~seander/bithacks.html#ConditionalNegate
        const auto value = x0 + x1;
        return negateValue(value) / 10.0f;
    }
}

static auto ReadFile() -> MeasurementTree {
    constexpr const char* const INPUT_FILE = "../measurements.txt";
    std::ifstream infile(INPUT_FILE);

    MeasurementTree measurements;
    struct stat sb;
    long cntr = 0;
    int fd;
    char *data;
    char* line;
    // map the file
    fd = open(INPUT_FILE, O_RDONLY);
    if (fd == -1) {
        std::cout << "failed to open file." << std::endl;
        exit(EXIT_FAILURE);
    }

    fstat(fd, &sb);
    const auto fileSize = sb.st_size;
    data = (char*) mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }
    madvise(data, fileSize, MADV_SEQUENTIAL);

    // get lines
    while(cntr < fileSize) {
        int lineLen = 0;
        line = data;

        // find the next line and the semicolon
        int semicolonPosition = 0;
        while(*data != '\n' && cntr < fileSize) {
            data++;
            cntr++;
            lineLen++;

            // Set the int when the condition is true.  Do nothing otherwise.
            if (*data == ';') {
                semicolonPosition = lineLen;
            }
        }

        // Skip over the new line.
        data++;
        cntr++;

        // std::string_view does not make a copy of the line.
        // read the line directly from the cache.
        std::string_view lineNotOwned(line, lineLen);
        // std::cout << "Got line: (" << lineNotOwned << ")" << std::endl;

        const auto name = std::string_view(lineNotOwned.begin(), lineNotOwned.begin() + semicolonPosition);
        const auto number = std::string_view(lineNotOwned.begin() + semicolonPosition + 1, lineNotOwned.end());

        // std::cout << "Got name: " << name << " " << number << std::endl;

        const auto numberParsed = ParseNumber(number);
        // std::cout << "Number parsed: " << numberParsed << std::endl;

        // TODO: Only copy the string if the value does not exist in the map.
        const auto nameCopy = std::string(name);
        auto* existing = measurements.Get(nameCopy);
        if (existing != nullptr) {
            existing->min = std::min(existing->min, numberParsed);
            existing->max = std::max(existing->min, numberParsed);
            existing->count++;
            existing->sum += numberParsed;
        } else {
            measurements.Emplace(std::move(nameCopy), Measurement(numberParsed, numberParsed, 1, numberParsed));
        }
    }

    return measurements;
}

static auto PrintMeasurements(const MeasurementTree& map) -> int {
    std::cout << "{";
    for (const auto& key : map.ToArray()) {
        const auto& name = key.first;
        const auto& measurement = key.second;
        std::cout << std::fixed << std::setprecision(1) << name << "=" << measurement.min << "/" << (measurement.sum / measurement.count) << "/" << measurement.max << ", ";
    }
    std::cout << "}" << std::endl;

    return 0;
}

int main() {
    return PrintMeasurements(ReadFile());
}