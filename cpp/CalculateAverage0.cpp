#include <iomanip>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
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

using MeasurementMap = std::map<std::string, Measurement>;

static auto ReadFile() -> MeasurementMap {
    constexpr const char* const INPUT_FILE = "../measurements.txt";
    std::ifstream infile(INPUT_FILE);
    std::string line;
    
    MeasurementMap measurements;
    while (std::getline(infile, line)) {
        // Split the line by ';'
        std::string name;
        bool didFindSemicolon = false;
        std::string number;
        for (auto c : line) {
            if (c == ';') {
                didFindSemicolon = true;
                continue;
            }

            if (!didFindSemicolon) {
                name += c;
            } else {
                // Number parsing.
                number += c;
            }
        }
        auto numberParsed = std::stof(number);
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