#pragma once
#include <string>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

struct Config {
    std::string primaryModel;
    std::string secondaryModel;
    std::string operation = "UNION";
    std::string outputPath;
};

namespace detail {

inline std::string trim(const std::string& s) {
    auto b = s.begin();
    while (b != s.end() && std::isspace(static_cast<unsigned char>(*b))) ++b;
    auto e = s.end();
    while (e != b && std::isspace(static_cast<unsigned char>(*(e - 1)))) --e;
    return {b, e};
}

inline std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

// Strip surrounding quotes added by shell drag-and-drop
inline std::string stripQuotes(std::string s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);
    return s;
}

} // namespace detail

inline Config loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + path);

    Config cfg;
    std::string line;
    while (std::getline(file, line)) {
        const auto trimmed = detail::trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
            continue;

        const auto eq = trimmed.find('=');
        if (eq == std::string::npos) continue;

        const auto key = detail::toUpper(detail::trim(trimmed.substr(0, eq)));
        const auto val = detail::trim(trimmed.substr(eq + 1));

        if      (key == "PRIMARY_MODEL")   cfg.primaryModel   = val;
        else if (key == "SECONDARY_MODEL") cfg.secondaryModel = val;
        else if (key == "OPERATION")       cfg.operation      = detail::toUpper(val);
        else if (key == "OUTPUT_PATH")     cfg.outputPath     = val;
    }

    if (cfg.primaryModel.empty())
        throw std::runtime_error("Config missing required key: primary_model");
    if (cfg.secondaryModel.empty())
        throw std::runtime_error("Config missing required key: secondary_model");
    if (cfg.outputPath.empty())
        throw std::runtime_error("Config missing required key: output_path");

    const auto& op = cfg.operation;
    if (op != "UNION" && op != "INTERSECTION" && op != "DIFFERENCE")
        throw std::runtime_error(
            "Unknown operation '" + op + "'. Valid values: UNION, INTERSECTION, DIFFERENCE");

    return cfg;
}
