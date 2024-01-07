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

struct Measurement {
    using Value = float;
    using Counter = std::size_t;
    Value min, max;
    std::size_t count;
    Value sum;

    Measurement(Value min_, Value max_, Counter count_, Value sum_) : min(min_), max(max_), count(count_), sum(sum_) {}
};

using MeasurementMap = std::unordered_map<std::string, Measurement>;

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

static auto ReadFile() -> MeasurementMap {
    constexpr const char* const INPUT_FILE = "../measurements.txt";
    std::ifstream infile(INPUT_FILE);

    MeasurementMap measurements;
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
        auto it = measurements.find(nameCopy);
        if (it != measurements.end()) {
            auto& existing = it->second;
            existing.min = std::min(existing.min, numberParsed);
            existing.max = std::max(existing.min, numberParsed);
            existing.count++;
            existing.sum += numberParsed;
        } else {
            measurements.emplace(nameCopy, Measurement(numberParsed, numberParsed, 1, numberParsed));
        }
    }

    return measurements;
}

static auto PrintMeasurements(const MeasurementMap& map) -> int {
    // Sort outputs.
    std::vector<std::string> keys = [&]() {
        std::vector<std::string> keys;
        for (const auto& [k, v] : map) {
            keys.push_back(k);
        }
        std::sort(keys.begin(), keys.end(), std::locale("en_US.utf8"));
        return keys;
    }();

    std::cout << "{";
    for (const auto& key : keys) {
        const auto& name = key;
        const auto& measurement = map.at(key);
        std::cout << std::fixed << std::setprecision(1) << name << "=" << measurement.min << "/" << (measurement.sum / measurement.count) << "/" << measurement.max << ", ";
    }
    std::cout << "}" << std::endl;

    return 0;
}

int main() {
    return PrintMeasurements(ReadFile());
}