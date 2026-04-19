#include "NativeIngester.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <regex>

namespace fs = std::filesystem;

bool NativeIngester::GlobMatch(const std::string& pattern, const std::string& text) {
    if (pattern.empty()) return text.empty();
    size_t i = 0, j = 0;
    size_t star_p = std::string::npos, star_t = std::string::npos;
    
    while (j < text.length()) {
        if (i < pattern.length() && (pattern[i] == '?' || pattern[i] == text[j])) {
            i++; j++;
        } else if (i < pattern.length() && pattern[i] == '*') {
            star_p = i++;
            star_t = j;
        } else if (star_p != std::string::npos) {
            i = star_p + 1;
            j = ++star_t;
        } else {
            return false;
        }
    }
    while (i < pattern.length() && pattern[i] == '*') i++;
    return i == pattern.length();
}

std::vector<std::string> NativeIngester::ParseGitignore(const fs::path& rootPath) {
    std::vector<std::string> patterns;
    fs::path gitignorePath = rootPath / ".gitignore";
    if (fs::exists(gitignorePath) && fs::is_regular_file(gitignorePath)) {
        std::ifstream ifs(gitignorePath);
        std::string line;
        while (std::getline(ifs, line)) {
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
            if (!line.empty() && line[0] != '#') {
                if (line.back() == '/') line.pop_back();
                if (line.front() == '/') line.erase(0, 1);
                patterns.push_back(line);
            }
        }
    }
    return patterns;
}

std::vector<std::string> NativeIngester::SplitComma(const std::string& str) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        token.erase(std::find_if(token.rbegin(), token.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), token.end());
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

bool NativeIngester::IsExcluded(const fs::path& path, const std::vector<std::string>& customExcludes, const std::vector<std::string>& gitignores) {
    std::string filename = path.filename().string();
    std::string pstr = path.generic_string();
    
    // Default ignores (matching gitingest logic)
    static const std::set<std::string> defaultIgnores = {
        ".git", "node_modules", "build", "target", "dist", "vendor", "__pycache__", 
        ".idea", ".vscode", ".next", ".gradle", ".settings", ".classpath", ".project",
        "package-lock.json", "yarn.lock", "pnpm-lock.yaml", "Pipfile.lock", "poetry.lock"
    };

    if (defaultIgnores.count(filename) > 0) return true;

    // Check if any part of the path is in defaultIgnores
    for (const auto& part : path) {
        if (defaultIgnores.count(part.string()) > 0) return true;
    }

    // Gitignores
    for (const auto& gi : gitignores) {
        if (filename == gi || GlobMatch(gi, filename)) return true;
        if (pstr.find("/" + gi) != std::string::npos) return true;
    }

    // Custom ignores
    for (const auto& excl : customExcludes) {
        if (pstr.find(excl) != std::string::npos || GlobMatch(excl, filename)) return true;
    }

    return false;
}

bool NativeIngester::IsBinaryOrMedia(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    static const std::set<std::string> binExts = {
        ".exe", ".dll", ".so", ".dylib", ".class", ".jar", ".zip", ".tar", ".gz", ".7z",
        ".png", ".jpg", ".jpeg", ".gif", ".webp", ".svg", ".ico", ".pdf", ".mp4", ".mp3",
        ".avi", ".mov", ".ttf", ".woff", ".woff2", ".eot", ".bin", ".obj", ".o", ".a", ".lib",
        ".db", ".sqlite", ".pdb", ".exp", ".suo", ".user", ".pyc", ".pyd", ".iso", ".img", ".msi"
    };

    return binExts.count(ext) > 0;
}

std::string NativeIngester::ReadFileContent(const fs::path& path, bool addLineNumbers) {
    std::ifstream ifs(path, std::ios::binary); // Use binary to avoid CRLF issues during reading
    if (!ifs) return "Error reading file";
    
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string content = buffer.str();

    // Basic heuristic to skip binary content if not caught by extension
    int nullCount = 0;
    for (size_t i = 0; i < std::min<size_t>(content.size(), 1024); ++i) {
        if (content[i] == '\0') nullCount++;
    }
    if (nullCount > 2) return "[Binary content skipped]";

    if (!addLineNumbers) {
        return content;
    } else {
        std::istringstream iss(content);
        std::ostringstream ss;
        std::string line;
        int lineNum = 1;
        while (std::getline(iss, line)) {
            ss << lineNum++ << " | " << line << "\n";
        }
        return ss.str();
    }
}

std::string NativeIngester::IngestLocalDirectory(const IngestOptions& options, 
                                                std::function<void(const std::string&)> logCallback,
                                                std::function<bool()> stopCheck) {
    auto excludes = SplitComma(options.customExcludes);
    fs::path rootPath(options.sourcePath);
    if (!fs::exists(rootPath)) {
        if (logCallback) logCallback("Error: Le chemin source n'existe pas.\n");
        return "";
    }
    
    std::vector<std::string> gitignores;
    if (!options.ignoreGitignore) {
        gitignores = ParseGitignore(rootPath);
    }

    uintmax_t maxSizeBytes = static_cast<uintmax_t>(options.maxSizeMb) * 1024 * 1024;
    uintmax_t currentTotalSize = 0;

    std::ostringstream masterDigest;
    std::ostringstream fileContents;
    std::ostringstream treeListing;
    
    treeListing << "Directory structure:\n";

    if (logCallback) logCallback("Parcours de " + rootPath.generic_string() + " ...\n");

    try {
        std::vector<fs::path> collectedFiles;
        
        std::function<void(fs::path, std::string, bool)> WalkPath = [&](fs::path p, std::string prefix, bool isLast) {
            if (stopCheck && stopCheck()) return;
            
            std::string rel = fs::relative(p, rootPath).generic_string();
            if (p != rootPath && (IsExcluded(p, excludes, gitignores) || options.excludedPaths.count(rel) > 0)) {
                return;
            }
            
            if (p != rootPath) {
                treeListing << prefix << (isLast ? "└── " : "├── ") << p.filename().string() << "\n";
            }

            if (fs::is_directory(p)) {
                std::vector<fs::path> entries;
                try {
                    for (const auto& entry : fs::directory_iterator(p)) {
                        entries.push_back(entry.path());
                    }
                } catch (const std::exception& e) {
                    if (logCallback) logCallback(std::string("⚠ Impossible de lire ") + p.generic_string() + " : " + e.what() + "\n");
                }
                
                std::sort(entries.begin(), entries.end());
                
                std::vector<fs::path> filteredEntries;
                for (const auto& entryPath : entries) {
                    if (!IsExcluded(entryPath, excludes, gitignores)) {
                        filteredEntries.push_back(entryPath);
                    }
                }

                for (size_t i = 0; i < filteredEntries.size(); ++i) {
                    bool last = (i == filteredEntries.size() - 1);
                    std::string nextPrefix = prefix + (p == rootPath ? "" : (isLast ? "    " : "│   "));
                    WalkPath(filteredEntries[i], nextPrefix, last);
                }
            } else if (fs::is_regular_file(p)) {
                if (!IsBinaryOrMedia(p)) {
                    collectedFiles.push_back(p);
                }
            }
        };

        WalkPath(rootPath, "", true);
        
        if (stopCheck && stopCheck()) {
            if (logCallback) logCallback("Analyse annulée.\n");
            return "Analyse annulée par l'utilisateur.";
        }

        if (logCallback) logCallback(std::to_string(collectedFiles.size()) + " fichiers trouvés.\n");

        for (const auto& p : collectedFiles) {
            if (stopCheck && stopCheck()) break;

            auto fsize = fs::file_size(p);
            if (!options.ignoreMaxSizeLimit && (currentTotalSize + fsize > maxSizeBytes)) {
                continue;
            }

            currentTotalSize += fsize;
            std::string relativePath = fs::relative(p, rootPath).generic_string(); // POSIX SLASHS
            
            fileContents << "================================================\n";
            fileContents << "File: " << relativePath << "\n";
            fileContents << "================================================\n";
            fileContents << ReadFileContent(p, options.addLineNumbers) << "\n\n";
        }

    } catch (const fs::filesystem_error& err) {
        if (logCallback) logCallback(std::string("Erreur d'acces systeme: ") + err.what() + "\n");
    }

    if (stopCheck && stopCheck()) return "Analyse annulée par l'utilisateur.";

    masterDigest << treeListing.str() << "\n\n" << fileContents.str();
    if (logCallback) logCallback(std::to_string(currentTotalSize / 1024) + " KB générés.\n");

    return masterDigest.str();
}

FileNode NativeIngester::ScanDirectory(const std::string& rootPathStr, bool ignoreGitignore) {
    fs::path rootPath(rootPathStr);
    FileNode root;
    root.name = rootPath.filename().string();
    root.relativePath = ".";
    root.isDirectory = true;

    if (!fs::exists(rootPath) || !fs::is_directory(rootPath)) return root;

    std::vector<std::string> gitignores;
    if (!ignoreGitignore) gitignores = ParseGitignore(rootPath);
    
    // Default excludes as list for IsExcluded
    std::vector<std::string> emptyExcl;

    std::function<void(fs::path, FileNode&)> Walk = [&](fs::path p, FileNode& node) {
        try {
            for (const auto& entry : fs::directory_iterator(p)) {
                fs::path ep = entry.path();
                
                // EARLY FILTERING: Skip bloated folders immediately
                if (IsExcluded(ep, emptyExcl, gitignores)) continue;

                FileNode child;
                child.name = ep.filename().string();
                child.relativePath = fs::relative(ep, rootPath).generic_string();
                child.isDirectory = fs::is_directory(ep);
                child.state = SelectionState::Selected;

                if (child.isDirectory) {
                    Walk(ep, child);
                }
                node.children.push_back(std::move(child));
            }
            // Sort children
            std::sort(node.children.begin(), node.children.end(), [](const FileNode& a, const FileNode& b) {
                if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
                return a.name < b.name;
            });
        } catch (const std::exception& e) {
            // Silently ignore permissions errors during initial UI tree scan to avoid blocking the user
        }
    };

    Walk(rootPath, root);
    return root;
}
