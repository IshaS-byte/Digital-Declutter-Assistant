#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <ctime>

using namespace std;

// DigitalItem class - stores details about each file/item
class DigitalItem {
private:
    string name;
    string type;
    long size;
    string lastUsedDate;

public:
    // Constructor
    DigitalItem(string n = "", string t = "", long s = 0, string date = "")
        : name(n), type(t), size(s), lastUsedDate(date) {}

    // Getters
    string getName() const { return name; }
    string getType() const { return type; }
    long getSize() const { return size; }
    string getLastUsedDate() const { return lastUsedDate; }

    // Setters
    void setName(string n) { name = n; }
    void setType(string t) { type = t; }
    void setSize(long s) { size = s; }
    void setLastUsedDate(string date) { lastUsedDate = date; }

    // Display item details
    void display() const {
        cout << "Name: " << name 
             << " | Type: " << type 
             << " | Size: " << size << " KB"
             << " | Last Used: " << lastUsedDate << endl;
    }
}