#include <iomanip>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <locale>
#include <string>
#include <vector>

struct Measurement {
    using Value = float;
    using Counter = std::size_t;
    Value min, max;
    std::size_t count;
    Value sum;

    Measurement(Value min_, Value max_, Counter count_, Value sum_) : min(min_), max(max_), count(count_), sum(sum_) {}
};

using MeasurementMap = std::unordered_map<std::string, Measurement>;

static auto ParseNumber(const std::string& number)-> float {
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
    std::string line;
    
    MeasurementMap measurements;
    while (std::getline(infile, line)) {
        // Split the line by ';'
        const auto semicolonPosition = [&]() -> std::size_t {
            for (std::size_t i = 0; i < line.size(); ++i) {
                if (line[i] == ';') {
                    return i;
                }
            }
            return 0;
        }();

        const std::string name = std::string(line.begin(), line.begin() + semicolonPosition);
        const std::string number = std::string(line.begin() + semicolonPosition + 1, line.end());

        const auto numberParsed = ParseNumber(number);

        auto it = measurements.find(name);
        if (it != measurements.end()) {
            auto& existing = it->second;
            existing.min = std::min(existing.min, numberParsed);
            existing.max = std::max(existing.min, numberParsed);
            existing.count++;
            existing.sum += numberParsed;
        } else {
            measurements.emplace(name, Measurement(numberParsed, numberParsed, 1, numberParsed));
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