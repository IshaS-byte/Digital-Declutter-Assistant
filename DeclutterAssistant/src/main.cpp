#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <ctime>

using namespace std;

class DigitalItem {
    private:
        string name;
        string type;
        long size;
        time_t lastUsed;

    public:
        DigitalItem(string n, string t, long s) : name(n), type(t), size(s) {
            lastUsed = time(0);
        }

        DigitalItem(string n = "", string t = "", long s = 0, time_t date = 0)
        : name(n), type(t), size(s), lastUsed(date) {}

        string getName() const {
            return name;
        }

        string getType() const {
            return type;
        }

        long getSize() const {
            return size;
        }

        time_t getLastUsed() const {
            return lastUsed;
        }

        void updateLastUsed() {
            lastUsed = time(0);
        }

        void displayInfo() const {
            cout << "Name: " << name << ", Type: " << type << ", Size: " << size << " bytes, Last Used: " << ctime(&lastUsed);
        }
};

int main() {
    cout << "Welcome to the Digital Declutter Assistant!" << endl;
};