#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <filesystem>
#include <shlobj.h>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// JSON helper functions
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
    string status = (statusCode == 200) ? "200 OK" : 
                    (statusCode == 404) ? "404 Not Found" : 
                    (statusCode == 500) ? "500 Internal Server Error" : "400 Bad Request";
    
    stringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << getCORSHeaders();
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

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

string listAllFilesRecursive(const string& directory) {
    stringstream json;
    json << "{\"files\":[";
    
    bool first = true;
    
    try {
        if (!filesystem::exists(directory) || !filesystem::is_directory(directory)) {
            return "{\"error\":\"Directory does not exist or is not accessible\"}";
        }
        
        for (const auto& entry : filesystem::recursive_directory_iterator(
            directory,
            filesystem::directory_options::skip_permission_denied)) {
            
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
    } catch (...) {
        return "{\"error\":\"Failed to list files\"}";
    }
    
    json << "]}";
    return json.str();
}

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

void handleRequest(SOCKET client, const string& request) {
    string response;
    
    if (request.find("OPTIONS") == 0) {
        response = createHTTPResponse(200, "");
        send(client, response.c_str(), response.length(), 0);
        return;
    }
    
    if (request.find("GET /drives") == 0) {
        string body = getDrives();
        response = createHTTPResponse(200, body);
    }
    else if (request.find("GET /directories") == 0) {
        size_t pathPos = request.find("path=");
        string path = "C:\\";
        if (pathPos != string::npos) {
            size_t endPos = request.find(" ", pathPos);
            path = request.substr(pathPos + 5, endPos - pathPos - 5);
            size_t pos = 0;
            while ((pos = path.find("%5C", pos)) != string::npos) {
                path.replace(pos, 3, "\\");
                pos += 1;
            }
            pos = 0;
            while ((pos = path.find("%3A", pos)) != string::npos) {
                path.replace(pos, 3, ":");
                pos += 1;
            }
            pos = 0;
            while ((pos = path.find("%20", pos)) != string::npos) {
                path.replace(pos, 3, " ");
                pos += 1;
            }
        }
        string body = listDirectories(path);
        response = createHTTPResponse(200, body);
    }
    else if (request.find("GET /files-recursive") == 0) {
        size_t dirPos = request.find("directory=");
        string directory = "C:\\";
        if (dirPos != string::npos) {
            size_t endPos = request.find(" ", dirPos);
            directory = request.substr(dirPos + 10, endPos - dirPos - 10);
            
            size_t pos = 0;
            while ((pos = directory.find("%5C", pos)) != string::npos) {
                directory.replace(pos, 3, "\\");
                pos += 1;
            }
            pos = 0;
            while ((pos = directory.find("%3A", pos)) != string::npos) {
                directory.replace(pos, 3, ":");
                pos += 1;
            }
            pos = 0;
            while ((pos = directory.find("%20", pos)) != string::npos) {
                directory.replace(pos, 3, " ");
                pos += 1;
            }
        }
        
        cout << "Recursively scanning: " << directory << endl;
        string body = listAllFilesRecursive(directory);
        response = createHTTPResponse(200, body);
    }
    else if (request.find("GET /files") == 0) {
        size_t dirPos = request.find("directory=");
        string directory = "C:\\";
        if (dirPos != string::npos) {
            size_t endPos = request.find(" ", dirPos);
            directory = request.substr(dirPos + 10, endPos - dirPos - 10);
            
            size_t pos = 0;
            while ((pos = directory.find("%5C", pos)) != string::npos) {
                directory.replace(pos, 3, "\\");
                pos += 1;
            }
            pos = 0;
            while ((pos = directory.find("%3A", pos)) != string::npos) {
                directory.replace(pos, 3, ":");
                pos += 1;
            }
            pos = 0;
            while ((pos = directory.find("%20", pos)) != string::npos) {
                directory.replace(pos, 3, " ");
                pos += 1;
            }
        }
        
        cout << "Listing files in: " << directory << endl;
        string body = listFiles(directory);
        response = createHTTPResponse(200, body);
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

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed" << endl;
        return 1;
    }
    
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    listen(serverSocket, 5);
    cout << "Server running on http://localhost:8080" << endl;
    
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