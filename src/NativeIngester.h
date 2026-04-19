#pragma once
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <set>

enum class SelectionState { Unselected, Partial, Selected };

struct FileNode {
    std::string name;
    std::string relativePath;
    bool isDirectory = false;
    SelectionState state = SelectionState::Selected;
    std::vector<FileNode> children;
};

struct IngestOptions {
    std::string sourcePath;
    std::string customExcludes; // comma separated
    std::string customIncludes; // comma separated
    int maxSizeMb = 10;
    bool ignoreGitignore = false;
    bool addLineNumbers = false;
    bool ignoreMaxSizeLimit = false;
    std::set<std::string> excludedPaths; // Granular paths (relative)
};

class NativeIngester {
public:
    // Process the ingestion locally and returns the Full Digest Text
    static std::string IngestLocalDirectory(const IngestOptions& options, 
                                            std::function<void(const std::string&)> logCallback,
                                            std::function<bool()> stopCheck = nullptr);

    // Build a tree of files for interactive selection
    static FileNode ScanDirectory(const std::string& rootPath, bool ignoreGitignore);

    static std::vector<std::string> SplitComma(const std::string& str);
    static std::vector<std::string> ParseGitignore(const std::filesystem::path& rootPath);
    static bool IsExcluded(const std::filesystem::path& path, const std::vector<std::string>& excludes, const std::vector<std::string>& gitignores);

private:
    static bool IsBinaryOrMedia(const std::filesystem::path& path);
    static std::string ReadFileContent(const std::filesystem::path& path, bool addLineNumbers);
    static bool GlobMatch(const std::string& pattern, const std::string& text);
};
