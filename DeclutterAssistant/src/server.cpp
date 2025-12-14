#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <filesystem>
#include <shlobj.h>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;
using namespace std::chrono;

// ============================================================================
// JSON Helper Functions
// ============================================================================

string jsonEscape(const string& str) {
    string result;
    for (char c : str) {
        if (c == '\\') result += "\\\\";
        else if (c == '"') result += "\\\"";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

string getCORSHeaders() {
    return "Access-Control-Allow-Origin: *\r\n"
           "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
           "Access-Control-Allow-Headers: Content-Type\r\n";
}

string createHTTPResponse(int statusCode, const string& body, const string& contentType = "application/json") {
    string status;
    switch (statusCode) {
        case 200: status = "200 OK"; break;
        case 400: status = "400 Bad Request"; break;
        case 404: status = "404 Not Found"; break;
        case 500: status = "500 Internal Server Error"; break;
        default: status = "500 Internal Server Error";
    }
    
    stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << getCORSHeaders();
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

// ============================================================================
// Path Normalization Helper
// ============================================================================

string normalizePath(const string& path) {
    string normalized = path;
    
    // Replace forward slashes with backslashes for Windows
    replace(normalized.begin(), normalized.end(), '/', '\\');
    
    // Ensure drive letter is uppercase
    if (normalized.length() >= 2 && normalized[1] == ':') {
        normalized[0] = toupper(normalized[0]);
    }
    
    // Remove trailing backslash unless it's a root directory
    if (normalized.length() > 3 && normalized.back() == '\\') {
        normalized.pop_back();
    }
    
    return normalized;
}

// ============================================================================
// Filesystem Helper Functions
// ============================================================================

string getDrives() {
    stringstream json;
    json << "{\"drives\":[";
    
    bool first = true;
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            if (!first) json << ",";
            char driveLetter = 'A' + i;
            json << "\"" << driveLetter << ":\\\\\"";
            first = false;
        }
    }
    
    json << "]}";
    return json.str();
}

string listDirectories(const string& directory) {
    stringstream json;
    json << "{\"directories\":[";
    
    bool first = true;
    try {
        if (!filesystem::exists(directory) || !filesystem::is_directory(directory)) {
            return "{\"error\":\"Directory does not exist\"}";
        }
        
        for (const auto& entry : filesystem::directory_iterator(directory)) {
            try {
                if (entry.is_directory()) {
                    if (!first) json << ",";
                    json << "{";
                    json << "\"name\":\"" << jsonEscape(entry.path().filename().string()) << "\",";
                    json << "\"path\":\"" << jsonEscape(entry.path().string()) << "\"";
                    json << "}";
                    first = false;
                }
            } catch (...) {
                continue;
            }
        }
    } catch (...) {
        return "{\"error\":\"Failed to list directories\"}";
    }
    
    json << "]}";
    return json.str();
}

string listFiles(const string& directory) {
    stringstream json;
    json << "{\"files\":[";
    
    bool first = true;
    
    try {
        if (!filesystem::exists(directory) || !filesystem::is_directory(directory)) {
            return "{\"error\":\"Directory does not exist or is not accessible\"}";
        }
        
        for (const auto& entry : filesystem::directory_iterator(directory)) {
            try {
                if (entry.is_regular_file()) {
                    if (!first) json << ",";
                    
                    json << "{";
                    json << "\"name\":\"" << jsonEscape(entry.path().filename().string()) << "\",";
                    json << "\"path\":\"" << jsonEscape(entry.path().string()) << "\",";
                    json << "\"type\":\"" << jsonEscape(entry.path().extension().string()) << "\",";
                    json << "\"size\":" << entry.file_size();
                    json << "}";
                    
                    first = false;
                }
            } catch (...) {
                continue;
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        return "{\"error\":\"Access denied or filesystem error\"}";
    } catch (...) {
        return "{\"error\":\"Failed to list files\"}";
    }
    
    json << "]}";
    return json.str();
}

// ============================================================================
// Smart Cleanup Functions (Fixed Version)
// ============================================================================

struct CleanupResult {
    vector<string> matchedFiles;
    uintmax_t totalSize;
    int count;
    bool success;
    string message;
};

/**
 * @brief Converts filesystem time to time_t properly
 */
time_t fileTimeToTimeT(const filesystem::file_time_type& ftime) {
    try {
        // Convert file_time to system_clock time_point
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        return std::chrono::system_clock::to_time_t(sctp);
    } catch (...) {
        // Fallback: return current time if conversion fails
        return time(nullptr);
    }
}

/**
 * @brief Scans directory recursively for files matching cleanup criteria
 */
CleanupResult scanForCleanup(const string& directory, const string& fileType, time_t beforeTimestamp) {
    CleanupResult result;
    result.count = 0;
    result.totalSize = 0;
    result.success = false;
    
    // Normalize the path
    string normalizedDir = normalizePath(directory);
    
    cout << "\n=== SCAN STARTED ===" << endl;
    cout << "Original Directory: " << directory << endl;
    cout << "Normalized Directory: " << normalizedDir << endl;
    cout << "File Type: " << fileType << endl;
    cout << "Before Timestamp: " << beforeTimestamp << endl;
    
    // Convert threshold timestamp to readable date
    char timeBuffer[80];
    struct tm timeInfo;
    localtime_s(&timeInfo, &beforeTimestamp);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    cout << "Threshold Date: " << timeBuffer << " (files OLDER than this will match)" << endl;
    
    try {
        // Check if directory exists
        if (!filesystem::exists(normalizedDir)) {
            result.message = "Directory does not exist: " + normalizedDir;
            cout << "ERROR: " << result.message << endl;
            return result;
        }
        
        if (!filesystem::is_directory(normalizedDir)) {
            result.message = "Path is not a directory: " + normalizedDir;
            cout << "ERROR: " << result.message << endl;
            return result;
        }
        
        // Normalize file type
        string normalizedType = fileType;
        if (!normalizedType.empty() && normalizedType[0] != '.') {
            normalizedType = "." + normalizedType;
        }
        transform(normalizedType.begin(), normalizedType.end(), normalizedType.begin(), ::tolower);
        
        cout << "Looking for files with extension: " << normalizedType << endl;
        cout << "Starting recursive scan..." << endl;
        
        int filesScanned = 0;
        int extensionMatches = 0;
        
        // Recursively iterate through directory
        for (const auto& entry : filesystem::recursive_directory_iterator(
            normalizedDir,
            filesystem::directory_options::skip_permission_denied)) {
            
            try {
                if (entry.is_regular_file()) {
                    filesScanned++;
                    
                    // Get file extension
                    string extension = entry.path().extension().string();
                    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                    
                    // Check if file extension matches
                    if (extension == normalizedType) {
                        extensionMatches++;
                        
                        // Get last write time
                        auto ftime = filesystem::last_write_time(entry.path());
                        time_t fileTime = fileTimeToTimeT(ftime);
                        
                        // Format file time for display
                        struct tm fileTimeInfo;
                        localtime_s(&fileTimeInfo, &fileTime);
                        char fileTimeBuffer[80];
                        strftime(fileTimeBuffer, sizeof(fileTimeBuffer), "%Y-%m-%d %H:%M:%S", &fileTimeInfo);
                        
                        // Debug output for first few matches
                        if (extensionMatches <= 5) {
                            cout << "\n[File #" << extensionMatches << "] " << entry.path().filename().string() << endl;
                            cout << "  Full path: " << entry.path().string() << endl;
                            cout << "  Extension: " << extension << " (matches: " << normalizedType << ")" << endl;
                            cout << "  File modified: " << fileTimeBuffer << " (timestamp: " << fileTime << ")" << endl;
                            cout << "  Threshold:     " << timeBuffer << " (timestamp: " << beforeTimestamp << ")" << endl;
                            cout << "  File time < Threshold? " << (fileTime < beforeTimestamp ? "YES - MATCH!" : "NO - too recent") << endl;
                            cout << "  Difference: " << (beforeTimestamp - fileTime) << " seconds" << endl;
                        }
                        
                        // FIXED: The logic should be fileTime < beforeTimestamp
                        // This means the file is OLDER than the threshold
                        if (fileTime < beforeTimestamp) {
                            result.matchedFiles.push_back(entry.path().string());
                            result.totalSize += entry.file_size();
                            result.count++;
                            
                            if (result.count <= 3) {
                                cout << "  ✓ ADDED TO RESULTS!" << endl;
                            }
                        }
                    }
                    
                    // Progress indicator every 100 files
                    if (filesScanned % 100 == 0) {
                        cout << "  ... scanned " << filesScanned << " files so far ..." << endl;
                    }
                }
            } catch (const exception& e) {
                // Log individual file errors but continue processing
                if (filesScanned < 10) {
                    cerr << "Warning: Failed to process file - " << e.what() << endl;
                }
                continue;
            }
        }
        
        cout << "\n=== SCAN COMPLETED ===" << endl;
        cout << "Total files scanned: " << filesScanned << endl;
        cout << "Files with matching extension (" << normalizedType << "): " << extensionMatches << endl;
        cout << "Files matching date criteria: " << result.count << endl;
        cout << "Total size of matched files: " << result.totalSize << " bytes" << endl;
        
        if (extensionMatches == 0) {
            result.message = "No files found with extension " + normalizedType + " in directory";
        } else if (result.count == 0) {
            result.message = "Found " + to_string(extensionMatches) + " files with extension " + 
                           normalizedType + ", but none are older than the specified date";
        } else {
            result.message = "Found " + to_string(result.count) + " file(s) to delete";
        }
        
        result.success = true;
        
    } catch (const filesystem::filesystem_error& e) {
        result.message = string("Filesystem error: ") + e.what();
        cout << "ERROR: " << result.message << endl;
    } catch (const exception& e) {
        result.message = string("Error: ") + e.what();
        cout << "ERROR: " << result.message << endl;
    }
    
    return result;
}

/**
 * @brief Deletes files from the cleanup result
 */
CleanupResult executeCleanup(const CleanupResult& scanResult) {
    CleanupResult result;
    result.count = 0;
    result.totalSize = 0;
    result.success = false;
    
    int failedCount = 0;
    
    cout << "\n=== DELETION STARTED ===" << endl;
    cout << "Files to delete: " << scanResult.matchedFiles.size() << endl;
    
    for (const auto& filepath : scanResult.matchedFiles) {
        try {
            if (filesystem::exists(filepath)) {
                uintmax_t fileSize = filesystem::file_size(filepath);
                
                if (filesystem::remove(filepath)) {
                    result.matchedFiles.push_back(filepath);
                    result.totalSize += fileSize;
                    result.count++;
                    
                    if (result.count <= 5) {
                        cout << "Deleted: " << filepath << endl;
                    }
                } else {
                    failedCount++;
                    cerr << "Failed to delete: " << filepath << endl;
                }
            } else {
                failedCount++;
                cerr << "File no longer exists: " << filepath << endl;
            }
        } catch (const exception& e) {
            failedCount++;
            cerr << "Error deleting " << filepath << ": " << e.what() << endl;
        }
    }
    
    cout << "\nDeletion completed!" << endl;
    cout << "Successfully deleted: " << result.count << endl;
    cout << "Failed: " << failedCount << endl;
    
    result.success = (result.count > 0);
    
    stringstream msg;
    msg << "Deleted " << result.count << " file(s)";
    if (failedCount > 0) {
        msg << ", " << failedCount << " failed";
    }
    result.message = msg.str();
    
    return result;
}

string handleScanCleanup(const string& directory, const string& fileType, time_t beforeTimestamp) {
    CleanupResult result = scanForCleanup(directory, fileType, beforeTimestamp);
    
    stringstream json;
    json << "{";
    json << "\"success\":" << (result.success ? "true" : "false") << ",";
    json << "\"message\":\"" << jsonEscape(result.message) << "\",";
    json << "\"count\":" << result.count << ",";
    json << "\"totalSize\":" << result.totalSize << ",";
    json << "\"files\":[";
    
    for (size_t i = 0; i < result.matchedFiles.size(); i++) {
        if (i > 0) json << ",";
        json << "\"" << jsonEscape(result.matchedFiles[i]) << "\"";
    }
    
    json << "]}";
    return json.str();
}

string handleExecuteCleanup(const string& directory, const string& fileType, time_t beforeTimestamp) {
    // First scan for files
    CleanupResult scanResult = scanForCleanup(directory, fileType, beforeTimestamp);
    
    if (!scanResult.success || scanResult.count == 0) {
        stringstream json;
        json << "{\"success\":false,\"message\":\"" << jsonEscape(scanResult.message) << "\",\"count\":0}";
        return json.str();
    }
    
    // Execute deletion
    CleanupResult deleteResult = executeCleanup(scanResult);
    
    stringstream json;
    json << "{";
    json << "\"success\":" << (deleteResult.success ? "true" : "false") << ",";
    json << "\"message\":\"" << jsonEscape(deleteResult.message) << "\",";
    json << "\"count\":" << deleteResult.count << ",";
    json << "\"totalSize\":" << deleteResult.totalSize;
    json << "}";
    
    return json.str();
}

// ============================================================================
// File Operation Functions
// ============================================================================

string handleDeleteFile(const string& filepath) {
    try {
        if (filesystem::exists(filepath) && filesystem::remove(filepath)) {
            return "{\"success\":true,\"message\":\"File deleted successfully\"}";
        }
        return "{\"success\":false,\"message\":\"File not found or cannot be deleted\"}";
    } catch (...) {
        return "{\"success\":false,\"message\":\"Error deleting file\"}";
    }
}

string handleCreateFile(const string& directory, const string& filename) {
    try {
        filesystem::path destPath = filesystem::path(directory) / filename;
        ofstream newFile(destPath);
        if (newFile) {
            newFile.close();
            return "{\"success\":true,\"message\":\"File created successfully\"}";
        }
        return "{\"success\":false,\"message\":\"Failed to create file\"}";
    } catch (...) {
        return "{\"success\":false,\"message\":\"Error creating file\"}";
    }
}

// ============================================================================
// HTTP Request Parsing Functions
// ============================================================================

string parseRequestBody(const string& request) {
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != string::npos) {
        return request.substr(bodyPos + 4);
    }
    return "";
}

string extractJSONValue(const string& json, const string& key) {
    string searchKey = "\"" + key + "\":\"";
    size_t pos = json.find(searchKey);
    if (pos == string::npos) return "";
    
    pos += searchKey.length();
    size_t endPos = json.find("\"", pos);
    if (endPos == string::npos) return "";
    
    return json.substr(pos, endPos - pos);
}

long long extractJSONNumber(const string& json, const string& key) {
    string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == string::npos) return 0;
    
    pos += searchKey.length();
    size_t endPos = json.find_first_of(",}", pos);
    if (endPos == string::npos) return 0;
    
    string numStr = json.substr(pos, endPos - pos);
    try {
        return stoll(numStr);
    } catch (...) {
        return 0;
    }
}

string urlDecode(const string& str) {
    string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            string hex = str.substr(i + 1, 2);
            char ch = (char)strtol(hex.c_str(), nullptr, 16);
            result += ch;
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

string extractQueryParam(const string& request, const string& param) {
    string searchKey = param + "=";
    size_t pos = request.find(searchKey);
    if (pos == string::npos) return "";
    
    pos += searchKey.length();
    size_t endPos = request.find_first_of(" &\r\n", pos);
    if (endPos == string::npos) endPos = request.length();
    
    string value = request.substr(pos, endPos - pos);
    return urlDecode(value);
}

// ============================================================================
// Main Request Handler
// ============================================================================

void handleRequest(SOCKET client, const string& request) {
    string response;
    
    // Handle CORS preflight
    if (request.find("OPTIONS") == 0) {
        response = createHTTPResponse(200, "");
        send(client, response.c_str(), response.length(), 0);
        return;
    }
    
    // Route handlers
    if (request.find("GET /drives") == 0) {
        string body = getDrives();
        response = createHTTPResponse(200, body);
    }
    else if (request.find("GET /directories") == 0) {
        string path = extractQueryParam(request, "path");
        if (path.empty()) path = "C:\\";
        string body = listDirectories(path);
        response = createHTTPResponse(200, body);
    }
    else if (request.find("GET /files") == 0) {
        string directory = extractQueryParam(request, "directory");
        if (directory.empty()) directory = "C:\\";
        cout << "Listing files in: " << directory << endl;
        string body = listFiles(directory);
        response = createHTTPResponse(200, body);
    }
    else if (request.find("GET /scan-cleanup") == 0) {
        string directory = extractQueryParam(request, "directory");
        string fileType = extractQueryParam(request, "fileType");
        string timestampStr = extractQueryParam(request, "beforeTimestamp");
        
        if (directory.empty() || fileType.empty() || timestampStr.empty()) {
            response = createHTTPResponse(400, "{\"success\":false,\"message\":\"Missing required parameters\"}");
            send(client, response.c_str(), response.length(), 0);
            return;
        }
        
        time_t beforeTimestamp = 0;
        try {
            beforeTimestamp = stoll(timestampStr);
        } catch (...) {
            response = createHTTPResponse(400, "{\"success\":false,\"message\":\"Invalid timestamp format\"}");
            send(client, response.c_str(), response.length(), 0);
            return;
        }
        
        string body = handleScanCleanup(directory, fileType, beforeTimestamp);
        response = createHTTPResponse(200, body);
    }
    else if (request.find("POST /cleanup") == 0) {
        string body = parseRequestBody(request);
        string directory = extractJSONValue(body, "directory");
        string fileType = extractJSONValue(body, "fileType");
        long long beforeTimestamp = extractJSONNumber(body, "beforeTimestamp");
        
        cout << "Executing cleanup: " << directory << " | Type: " << fileType << endl;
        string responseBody = handleExecuteCleanup(directory, fileType, (time_t)beforeTimestamp);
        response = createHTTPResponse(200, responseBody);
    }
    else if (request.find("DELETE /file") == 0) {
        string body = parseRequestBody(request);
        string filepath = extractJSONValue(body, "filepath");
        string responseBody = handleDeleteFile(filepath);
        response = createHTTPResponse(200, responseBody);
    }
    else if (request.find("POST /file") == 0) {
        string body = parseRequestBody(request);
        string directory = extractJSONValue(body, "directory");
        string filename = extractJSONValue(body, "filename");
        string responseBody = handleCreateFile(directory, filename);
        response = createHTTPResponse(200, responseBody);
    }
    else {
        response = createHTTPResponse(404, "{\"error\":\"Endpoint not found\"}");
    }
    
    send(client, response.c_str(), response.length(), 0);
}

// ============================================================================
// Main Server Entry Point
// ============================================================================

int main() {
    cout << "========================================" << endl;
    cout << " Digital Declutter Assistant - Server" << endl;
    cout << " Enterprise-Grade File Management API" << endl;
    cout << "========================================" << endl;
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "ERROR: WSAStartup failed" << endl;
        return 1;
    }
    
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "ERROR: Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "ERROR: Bind failed (port 8080 may be in use)" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    listen(serverSocket, 5);
    
    cout << endl;
    cout << "✓ Server running on http://localhost:8080" << endl;
    cout << "✓ CORS enabled for development" << endl;
    cout << "✓ Ready to accept connections..." << endl;
    cout << endl;
    
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;
        
        char buffer[4096] = {0};
        recv(clientSocket, buffer, 4096, 0);
        
        handleRequest(clientSocket, string(buffer));
        closesocket(clientSocket);
    }
    
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}