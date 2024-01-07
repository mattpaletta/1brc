#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <locale>
#include <string>
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

class MeasurementList {
public:
    constexpr static auto MAX_NUM_ENTRIES = 10'000;
    using MeasurementPair = std::pair<std::string, Measurement>;

    MeasurementList() = default;

    void Emplace(std::string name, Measurement measurement) {
        std::size_t i = 0;
        MeasurementPair newValue = {name, std::move(measurement)};
        for (i = 0; i < this->_size; ++i) {
            // Store in sorted order.
            if (this->locale(name, this->_data[i].first)) {
                // Insert it here.
                // Increment 1 once here for the next insertion.
                std::swap(this->_data[i++], newValue);
                break;
            }
        }

        // Swap the remainder to move them all down by 1.
        for (; i < this->_size; ++i) {
            std::swap(this->_data[i], newValue);
        }

        // Insert the last one.
        this->_data[this->_size++] = std::move(newValue);
    }

    Measurement* Get(const std::string& name) {
        for (std::size_t i = 0; i < this->_size; ++i) {
            // Store in sorted order.
            if (name == this->_data[i].first) {
                return &this->_data[i].second;
            }
        }
        
        return nullptr;
    }

    auto GetList() const -> const std::array<MeasurementPair, MAX_NUM_ENTRIES>& {
        return this->_data;
    }

    auto Size()const {
        return this->_size;
    }

private:
    std::array<MeasurementPair, MAX_NUM_ENTRIES> _data;
    std::size_t _size = 0;
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

static auto ReadFile() -> MeasurementList {
    constexpr const char* const INPUT_FILE = "../measurements.txt";
    std::ifstream infile(INPUT_FILE);

    MeasurementList measurements;
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

static auto PrintMeasurements(const MeasurementList& map) -> int {
    std::cout << "{";
    const auto& list = map.GetList();
    for (std::size_t i = 0; i < map.Size(); ++i) {
        const auto& key = list[i];
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