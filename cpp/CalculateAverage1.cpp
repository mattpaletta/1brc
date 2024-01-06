#include <iomanip>
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