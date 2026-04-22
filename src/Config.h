#pragma once
#include <string>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

// ---------------------------------------------------------------------------
// Supported modes
//   BOOLEAN        — two-mesh boolean op (original behaviour)
//   BATCH_MERGE    — union a collection of meshes from a folder or index range
//   TRANSFORM_SWEEP — union copies of a base mesh at each pose in a CSV
// ---------------------------------------------------------------------------
struct Config {
    std::string mode = "BOOLEAN";

    // --- BOOLEAN mode -------------------------------------------------------
    std::string primaryModel;
    std::string secondaryModel;
    std::string operation = "UNION"; // UNION | INTERSECTION | DIFFERENCE

    // --- BATCH_MERGE mode ---------------------------------------------------
    // Option 1: all supported mesh files found in this directory (sorted)
    std::string inputFolder;
    // Option 2: printf-style filename pattern + index range
    //   e.g. input_pattern = sample/models/mesh_%03d.obj
    //        input_start   = 0
    //        input_end     = 9
    std::string inputPattern;
    int inputStart = 0;
    int inputEnd   = -1;

    // --- TRANSFORM_SWEEP mode -----------------------------------------------
    std::string baseModel;      // single base mesh to stamp
    std::string transformsCsv;  // CSV of row-major 4x4 matrices (16 values/row)

    // --- common -------------------------------------------------------------
    std::string outputPath;
};

// ---------------------------------------------------------------------------

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

} // namespace detail

// ---------------------------------------------------------------------------

inline Config loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + path);

    Config cfg;
    std::string line;
    while (std::getline(file, line)) {
        const auto trimmed = detail::trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') continue;

        const auto eq = trimmed.find('=');
        if (eq == std::string::npos) continue;

        const auto key = detail::toUpper(detail::trim(trimmed.substr(0, eq)));
        const auto val = detail::trim(trimmed.substr(eq + 1));

        // Strip inline comments from value
        auto valClean = val;
        for (char delim : {'#', ';'}) {
            auto pos = valClean.find(delim);
            if (pos != std::string::npos)
                valClean = detail::trim(valClean.substr(0, pos));
        }

        if      (key == "MODE")            cfg.mode           = detail::toUpper(valClean);
        // BOOLEAN
        else if (key == "PRIMARY_MODEL")   cfg.primaryModel   = valClean;
        else if (key == "SECONDARY_MODEL") cfg.secondaryModel = valClean;
        else if (key == "OPERATION")       cfg.operation      = detail::toUpper(valClean);
        // BATCH_MERGE
        else if (key == "INPUT_FOLDER")    cfg.inputFolder    = valClean;
        else if (key == "INPUT_PATTERN")   cfg.inputPattern   = valClean;
        else if (key == "INPUT_START")     cfg.inputStart     = std::stoi(valClean);
        else if (key == "INPUT_END")       cfg.inputEnd       = std::stoi(valClean);
        // TRANSFORM_SWEEP
        else if (key == "BASE_MODEL")      cfg.baseModel      = valClean;
        else if (key == "TRANSFORMS_CSV")  cfg.transformsCsv  = valClean;
        // common
        else if (key == "OUTPUT_PATH")     cfg.outputPath     = valClean;
    }

    // --- validate -----------------------------------------------------------
    if (cfg.outputPath.empty())
        throw std::runtime_error("Config missing required key: output_path");

    if (cfg.mode == "BOOLEAN") {
        if (cfg.primaryModel.empty())
            throw std::runtime_error("BOOLEAN mode requires: primary_model");
        if (cfg.secondaryModel.empty())
            throw std::runtime_error("BOOLEAN mode requires: secondary_model");
        const auto& op = cfg.operation;
        if (op != "UNION" && op != "INTERSECTION" && op != "DIFFERENCE")
            throw std::runtime_error(
                "Unknown operation '" + op + "'. Valid: UNION, INTERSECTION, DIFFERENCE");
    } else if (cfg.mode == "BATCH_MERGE") {
        bool hasFolder  = !cfg.inputFolder.empty();
        bool hasPattern = !cfg.inputPattern.empty() && cfg.inputEnd >= cfg.inputStart;
        if (!hasFolder && !hasPattern)
            throw std::runtime_error(
                "BATCH_MERGE mode requires either input_folder "
                "or (input_pattern + input_start + input_end)");
    } else if (cfg.mode == "TRANSFORM_SWEEP") {
        if (cfg.baseModel.empty())
            throw std::runtime_error("TRANSFORM_SWEEP mode requires: base_model");
        if (cfg.transformsCsv.empty())
            throw std::runtime_error("TRANSFORM_SWEEP mode requires: transforms_csv");
    } else {
        throw std::runtime_error(
            "Unknown mode '" + cfg.mode +
            "'. Valid: BOOLEAN, BATCH_MERGE, TRANSFORM_SWEEP");
    }

    return cfg;
}
