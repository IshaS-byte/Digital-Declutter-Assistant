#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <windows.h>
#include <filesystem>

using namespace std;

class DigitalItem {
private:
    string name;
    string type;
    long size;
    time_t lastUsed;

public:

    string getName() const
    {
        return name;
    }

    string getType() const
    {
        return type;
    }

    long getSize() const
    {
        return size;
    }

    time_t getLastUsed() const
    {
        return lastUsed;
    }

    void updateLastUsed()
    {
        lastUsed = time(0);
    }

    void displayInfo() const
    {
        cout << "Name: " << name << ", Type: " << type << ", Size: " << size << " bytes, Last Used: " << ctime(&lastUsed);
    }

};
    
string openFile(){
    char filename[MAX_PATH] = "";
    char filter[] = "All Files\0*.*\0";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn))
    {
        return string(filename);
    }
    return "";
}


bool deleteFile(const string& filepath) {
    if (filepath.empty()) {
        cout << "No file selected." << endl;
        return false;
    }

    if (!filesystem::exists(filepath)) {
        cout << "Error: File does not exist." << endl;
        return false;
    }

    // Ask for confirmation
    cout << "Are you sure you want to delete: " << filepath << "? (y/n): ";
    char confirm;
    cin >> confirm;

    if (confirm != 'y' && confirm != 'Y') {
        cout << "Deletion cancelled." << endl;
        return false;
    }

    // Attempt to delete the file
    try {
        if (filesystem::remove(filepath)) {
            cout << "File deleted successfully!" << endl;
            return true;
        } else {
            cout << "Error: Failed to delete file." << endl;
            return false;
        }
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error deleting file: " << e.what() << endl;
        return false;
    }
}

bool addFile() {
    // Select source file
    cout << "Select a directory to add a file: " << endl;
    string sourceFile = openFile();
    
    if (sourceFile.empty()) {
        cout << "No file selected." << endl;
        return false;
    }

    // Check if source file exists
    if (!filesystem::exists(sourceFile)) {
        cout << "Error: Source file does not exist!" << endl;
        return false;
    }

    // Get destination path from user
    cout << "Enter the destination path (including filename):" << endl;
    cout << "Example: C:\\Users\\YourName\\Documents\\newfile.txt" << endl;
    string destPath;
    cin.ignore(); // Clear the input buffer
    getline(cin, destPath);

    if (destPath.empty()) {
        cout << "Error: Destination path cannot be empty!" << endl;
        return false;
    }

    // Check if destination already exists
    if (filesystem::exists(destPath)) {
        cout << "Warning: File already exists at destination. Overwrite? (y/n): ";
        char confirm;
        cin >> confirm;
        if (confirm != 'y' && confirm != 'Y') {
            cout << "Operation cancelled." << endl;
            return false;
        }
    }

    // Ensure destination directory exists
    filesystem::path destPathObj(destPath);
    filesystem::path destDir = destPathObj.parent_path();
    
    if (!destDir.empty() && !filesystem::exists(destDir)) {
        cout << "Destination directory does not exist. Create it? (y/n): ";
        char confirm;
        cin >> confirm;
        if (confirm == 'y' || confirm == 'Y') {
            try {
                filesystem::create_directories(destDir);
                cout << "Directory created successfully." << endl;
            } catch (const filesystem::filesystem_error& e) {
                cout << "Error creating directory: " << e.what() << endl;
                return false;
            }
        } else {
            cout << "Operation cancelled." << endl;
            return false;
        }
    }

    // Copy the file
    try {
        filesystem::copy_file(sourceFile, destPath, filesystem::copy_options::overwrite_existing);
        cout << "File added successfully!" << endl;
        cout << "From: " << sourceFile << endl;
        cout << "To: " << destPath << endl;
        return true;
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error copying file: " << e.what() << endl;
        return false;
    }
}

int main()
{
    cout << "Welcome to the Digital Declutter Assistant!" << endl;
    char action;
    cout << "a - add file, d - delete file, v - view file" << endl;
    cin >> action;

    if (action == 'a') {
        addFile();
    } else if (action == 'd') {
        string selected = openFile();
        deleteFile(selected);
    } else if (action == 'v') {
        string selected = openFile();
        cout << "You selected: " << selected << endl;
    }

    return 0;
}