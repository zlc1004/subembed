#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>

struct Config {
    std::string inputVideo;
    std::string inputSub;
    std::string output = "./";
    std::string outputName = "output-[index].mp4";
};

struct FileMatch {
    std::string videoPath;
    std::string subtitlePath;
    int index;
};

// Check if path is a directory
bool isDirectory(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

// Check if path exists
bool pathExists(const std::string& path) {
    struct stat statbuf;
    return stat(path.c_str(), &statbuf) == 0;
}

// Get filename from path
std::string getFilename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

// Get extension from filename
std::string getExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

// List files in directory
std::vector<std::string> listFiles(const std::string& dirPath) {
    std::vector<std::string> files;
    DIR* dir = opendir(dirPath.c_str());
    
    if (dir == nullptr) {
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            std::string fullPath = dirPath;
            if (fullPath.back() != '/') {
                fullPath += '/';
            }
            fullPath += name;
            
            // Check if it's a regular file
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
                files.push_back(fullPath);
            }
        }
    }
    
    closedir(dir);
    return files;
}

// Create directory (including parent directories)
bool createDirectories(const std::string& path) {
    std::string cmd = "mkdir -p \"" + path + "\"";
    return system(cmd.c_str()) == 0;
}

// Extract numeric index from filename
int extractIndex(const std::string& filename) {
    // Priority 1: Look for common episode patterns (e.g., E01, EP01, Episode01, ep-1, E1)
    std::vector<std::regex> episodePatterns = {
        std::regex(R"([Ee][Pp]?[_\s-]?(\d+))", std::regex::icase),      // E01, EP01, E-01, ep_01
        std::regex(R"([Ee]pisode[_\s-]?(\d+))", std::regex::icase),     // Episode01, episode-01
        std::regex(R"([_\s-](\d+)[_\s-])"),                              // -01-, _01_
        std::regex(R"([_\s-](\d+)\.)"),                                  // -01.ext, _01.ext
    };
    
    std::smatch match;
    
    // Try episode patterns first
    for (const auto& pattern : episodePatterns) {
        if (std::regex_search(filename, match, pattern)) {
            return std::stoi(match[1].str());
        }
    }
    
    // Fallback: Find the first standalone number (not part of resolution like 1080p, 720p)
    std::regex numRegex(R"((\d+))");
    std::string temp = filename;
    
    while (std::regex_search(temp, match, numRegex)) {
        int num = std::stoi(match[1].str());
        
        // Skip common resolution numbers
        if (num != 1080 && num != 720 && num != 480 && num != 2160 && num != 4320) {
            return num;
        }
        
        temp = match.suffix();
    }
    
    return -1;
}

// Get video files from directory
std::vector<std::pair<std::string, int>> getVideoFiles(const std::string& dir) {
    std::vector<std::pair<std::string, int>> files;
    std::vector<std::string> videoExtensions = {".mp4", ".mkv", ".avi", ".mov", ".m4v"};
    
    auto allFiles = listFiles(dir);
    
    for (const auto& path : allFiles) {
        std::string filename = getFilename(path);
        std::string ext = getExtension(filename);
        
        if (std::find(videoExtensions.begin(), videoExtensions.end(), ext) != videoExtensions.end()) {
            int index = extractIndex(filename);
            files.push_back({path, index});
        }
    }
    
    // Sort by index
    std::sort(files.begin(), files.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return files;
}

// Get subtitle files from directory
std::vector<std::pair<std::string, int>> getSubtitleFiles(const std::string& dir) {
    std::vector<std::pair<std::string, int>> files;
    std::vector<std::string> subExtensions = {".vtt", ".srt", ".ass", ".ssa"};
    
    auto allFiles = listFiles(dir);
    
    for (const auto& path : allFiles) {
        std::string filename = getFilename(path);
        std::string ext = getExtension(filename);
        
        if (std::find(subExtensions.begin(), subExtensions.end(), ext) != subExtensions.end()) {
            int index = extractIndex(filename);
            files.push_back({path, index});
        }
    }
    
    // Sort by index
    std::sort(files.begin(), files.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return files;
}

// Match video and subtitle files
std::vector<FileMatch> matchFiles(
    const std::vector<std::pair<std::string, int>>& videos,
    const std::vector<std::pair<std::string, int>>& subtitles) {
    
    std::vector<FileMatch> matches;
    
    // Strategy 1: Match by extracted index
    std::map<int, std::string> videosByIndex;
    std::map<int, std::string> subsByIndex;
    
    for (const auto& [path, idx] : videos) {
        if (idx >= 0) {
            videosByIndex[idx] = path;
        }
    }
    
    for (const auto& [path, idx] : subtitles) {
        if (idx >= 0) {
            subsByIndex[idx] = path;
        }
    }
    
    // Match by index
    for (const auto& [idx, videoPath] : videosByIndex) {
        if (subsByIndex.find(idx) != subsByIndex.end()) {
            matches.push_back({videoPath, subsByIndex[idx], idx});
        }
    }
    
    // If we got good matches by index, return them
    if (matches.size() > 0) {
        return matches;
    }
    
    // Strategy 2: Match by position (if both lists are same size)
    if (videos.size() == subtitles.size()) {
        for (size_t i = 0; i < videos.size(); i++) {
            matches.push_back({videos[i].first, subtitles[i].first, static_cast<int>(i + 1)});
        }
    }
    
    return matches;
}

// Execute ffmpeg command
bool processFile(const FileMatch& match, const std::string& outputDir, const std::string& outputTemplate) {
    // Replace [index] in output template
    std::string outputName = outputTemplate;
    size_t pos = outputName.find("[index]");
    if (pos != std::string::npos) {
        outputName.replace(pos, 7, std::to_string(match.index));
    }
    
    // Ensure output directory exists
    createDirectories(outputDir);
    
    std::string outputPath = outputDir;
    if (outputPath.back() != '/') {
        outputPath += '/';
    }
    outputPath += outputName;
    
    // Build ffmpeg command
    std::string cmd = "ffmpeg -i \"" + match.videoPath + "\" -i \"" + match.subtitlePath + 
                      "\" -c copy -c:s mov_text \"" + outputPath + "\" -y";
    
    std::cout << "Processing: " << getFilename(match.videoPath) << " + " 
              << getFilename(match.subtitlePath) << std::endl;
    std::cout << "Output: " << outputPath << std::endl;
    std::cout << "Command: " << cmd << std::endl << std::endl;
    
    int result = system(cmd.c_str());
    
    if (result != 0) {
        std::cerr << "Error: ffmpeg command failed with code " << result << std::endl;
        return false;
    }
    
    return true;
}

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS]\n"
              << "Options:\n"
              << "  -v, --input-video DIR    Input video directory (required)\n"
              << "  -s, --input-sub DIR      Input subtitle directory (required)\n"
              << "  -o, --output DIR         Output directory (default: ./)\n"
              << "  -n, --output-name NAME   Output filename template (default: output-[index].mp4)\n"
              << "                           Use [index] as placeholder for episode number\n"
              << "  -h, --help               Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    Config config;
    
    static struct option long_options[] = {
        {"input-video", required_argument, 0, 'v'},
        {"input-sub", required_argument, 0, 's'},
        {"output", required_argument, 0, 'o'},
        {"output-name", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "v:s:o:n:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'v':
                config.inputVideo = optarg;
                break;
            case 's':
                config.inputSub = optarg;
                break;
            case 'o':
                config.output = optarg;
                break;
            case 'n':
                config.outputName = optarg;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    // Validate required arguments
    if (config.inputVideo.empty() || config.inputSub.empty()) {
        std::cerr << "Error: Both --input-video and --input-sub are required\n" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Check if directories exist
    if (!pathExists(config.inputVideo) || !isDirectory(config.inputVideo)) {
        std::cerr << "Error: Video directory does not exist: " << config.inputVideo << std::endl;
        return 1;
    }
    
    if (!pathExists(config.inputSub) || !isDirectory(config.inputSub)) {
        std::cerr << "Error: Subtitle directory does not exist: " << config.inputSub << std::endl;
        return 1;
    }
    
    // Get files
    std::cout << "Scanning for video files..." << std::endl;
    auto videos = getVideoFiles(config.inputVideo);
    std::cout << "Found " << videos.size() << " video files" << std::endl << std::endl;
    
    std::cout << "Scanning for subtitle files..." << std::endl;
    auto subtitles = getSubtitleFiles(config.inputSub);
    std::cout << "Found " << subtitles.size() << " subtitle files" << std::endl << std::endl;
    
    if (videos.empty()) {
        std::cerr << "Error: No video files found in " << config.inputVideo << std::endl;
        return 1;
    }
    
    if (subtitles.empty()) {
        std::cerr << "Error: No subtitle files found in " << config.inputSub << std::endl;
        return 1;
    }
    
    // Match files
    std::cout << "Matching files..." << std::endl;
    auto matches = matchFiles(videos, subtitles);
    
    if (matches.empty()) {
        std::cerr << "Error: Could not match video and subtitle files" << std::endl;
        return 1;
    }
    
    std::cout << "Matched " << matches.size() << " pairs" << std::endl << std::endl;
    
    // Show matches
    std::cout << "=== Matched Pairs ===" << std::endl;
    for (const auto& match : matches) {
        std::cout << "[" << match.index << "] " 
                  << getFilename(match.videoPath) << " + "
                  << getFilename(match.subtitlePath) << std::endl;
    }
    std::cout << std::endl;
    
    // Process files
    std::cout << "=== Processing Files ===" << std::endl;
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& match : matches) {
        if (processFile(match, config.output, config.outputName)) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Successfully processed: " << successCount << std::endl;
    std::cout << "Failed: " << failCount << std::endl;
    
    return failCount > 0 ? 1 : 0;
}
