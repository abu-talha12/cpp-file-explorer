
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <system_error>

namespace fs = std::filesystem;
using namespace std;

void pause() {
    cout << "\nPress Enter to continue...";
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

string inputPath(const string& prompt = "Enter path: ") {
    string p;
    cout << prompt;
    getline(cin, p);
    if (p.empty()) {
        p = ".";
    }
    return p;
}

void listFiles(const string& path) {
    try {
        cout << "\nContents of: " << path << "\n";
        for (const auto& entry : fs::directory_iterator(path)) {
            auto fn = entry.path().filename().string();
            cout << (fs::is_directory(entry) ? "[DIR]  " : "       ") << fn << "\n";
        }
    } catch (fs::filesystem_error& e) {
        cout << "Error: " << e.what() << "\n";
    }
}

string permsToString(fs::perms p) {
    string s = "---------";
    if ((p & fs::perms::owner_read) != fs::perms::none) s[0] = 'r';
    if ((p & fs::perms::owner_write) != fs::perms::none) s[1] = 'w';
    if ((p & fs::perms::owner_exec) != fs::perms::none) s[2] = 'x';
    if ((p & fs::perms::group_read) != fs::perms::none) s[3] = 'r';
    if ((p & fs::perms::group_write) != fs::perms::none) s[4] = 'w';
    if ((p & fs::perms::group_exec) != fs::perms::none) s[5] = 'x';
    if ((p & fs::perms::others_read) != fs::perms::none) s[6] = 'r';
    if ((p & fs::perms::others_write) != fs::perms::none) s[7] = 'w';
    if ((p & fs::perms::others_exec) != fs::perms::none) s[8] = 'x';
    return s;
}

void listDetailed(const string& path) {
    try {
        cout << left << setw(40) << "Name" << setw(10) << "Type" << setw(12) << "Size" << setw(22) << "Last Modified" << "Perms\n";
        cout << string(100, '-') << "\n";
        for (const auto& entry : fs::directory_iterator(path)) {
            auto p = entry.path();
            string name = p.filename().string();
            string type = fs::is_directory(entry) ? "DIR" : "FILE";
            uintmax_t size = 0;
            if (!fs::is_directory(entry)) {
                error_code ec;
                size = fs::file_size(p, ec);
                if (ec) size = 0;
            }
            auto ftime = fs::last_write_time(p);
            auto st = chrono::time_point_cast<chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now()
                + chrono::system_clock::now());
            time_t cftime = chrono::system_clock::to_time_t(st);
            cout << left << setw(40) << name << setw(10) << type << setw(12) << size;
            cout << setw(22) << put_time(localtime(&cftime), "%Y-%m-%d %H:%M:%S");
            cout << permsToString(entry.status().permissions()) << "\n";
        }
    } catch (fs::filesystem_error& e) {
        cout << "Error: " << e.what() << "\n";
    }
}

void createFile() {
    string path = inputPath("Enter new file path: ");
    ofstream ofs(path);
    if (ofs) {
        cout << "File created: " << path << "\n";
    } else {
        cout << "Failed to create file.\n";
    }
}

void createDirectory() {
    string path = inputPath("Enter new directory path: ");
    error_code ec;
    if (fs::create_directories(path, ec)) {
        cout << "Directory created: " << path << "\n";
    } else {
        cout << "Failed to create directory. (" << ec.message() << ")\n";
    }
}

void deletePath() {
    string path = inputPath("Enter file or directory to delete: ");
    error_code ec;
    if (!fs::exists(path)) {
        cout << "Path does not exist.\n";
        return;
    }
    cout << "Are you sure you want to delete '" << path << "'? (yes/no): ";
    string ans;
    getline(cin, ans);
    if (ans == "yes" || ans == "y") {
        uintmax_t removed = fs::remove_all(path, ec);
        if (ec) {
            cout << "Delete failed: " << ec.message() << "\n";
        } else {
            cout << "Deleted " << removed << " entries.\n";
        }
    } else {
        cout << "Delete cancelled.\n";
    }
}

void renamePath() {
    string oldp = inputPath("Enter current path: ");
    string newp = inputPath("Enter new path/name: ");
    error_code ec;
    fs::rename(oldp, newp, ec);
    if (ec) cout << "Rename failed: " << ec.message() << "\n";
    else cout << "Renamed successfully.\n";
}

void copyPath() {
    string src = inputPath("Enter source file/directory: ");
    string dst = inputPath("Enter destination path: ");
    error_code ec;
    if (!fs::exists(src)) {
        cout << "Source does not exist.\n";
        return;
    }
    if (fs::is_directory(src)) {
        fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);
    } else {
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    }
    if (ec) cout << "Copy failed: " << ec.message() << "\n";
    else cout << "Copy completed.\n";
}

void movePath() {
    string src = inputPath("Enter source file/directory: ");
    string dst = inputPath("Enter destination path: ");
    error_code ec;
    fs::rename(src, dst, ec); // move as rename if possible
    if (ec) {
        if (fs::is_directory(src)) {
            fs::copy(src, dst, fs::copy_options::recursive, ec);
            if (!ec) fs::remove_all(src, ec);
        } else {
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
            if (!ec) fs::remove(src, ec);
        }
    }
    if (ec) cout << "Move failed: " << ec.message() << "\n";
    else cout << "Move completed.\n";
}

void viewFile() {
    string path = inputPath("Enter file path to view (shows first 1000 chars): ");
    error_code ec;
    if (!fs::exists(path) || fs::is_directory(path)) {
        cout << "File does not exist or is a directory.\n";
        return;
    }
    ifstream ifs(path);
    if (!ifs) {
        cout << "Can't open file.\n";
        return;
    }
    string line;
    size_t printed = 0;
    while (getline(ifs, line) && printed < 1000) {
        cout << line << "\n";
        printed += line.size();
    }
    if (ifs && !ifs.eof()) cout << "\n[Output truncated]\n";
}

void searchRecursiveImpl(const fs::path& base, const string& pattern, vector<fs::path>& out) {
    error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(base, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) continue;
        string name = entry.path().filename().string();
        if (name.find(pattern) != string::npos) {
            out.push_back(entry.path());
        }
    }
}

void searchRecursive() {
    string base = inputPath("Enter base directory to search: ");
    string pattern;
    cout << "Enter filename pattern (substring): ";
    getline(cin, pattern);
    vector<fs::path> results;
    try {
        searchRecursiveImpl(base, pattern, results);
        cout << "\nFound " << results.size() << " result(s):\n";
        for (auto &p: results) cout << p.string() << "\n";
    } catch (fs::filesystem_error& e) {
        cout << "Search error: " << e.what() << "\n";
    }
}

void fileInfo() {
    string path = inputPath("Enter file/directory path for info: ");
    error_code ec;
    if (!fs::exists(path)) {
        cout << "Path doesn't exist.\n";
        return;
    }
    cout << "Path: " << path << "\n";
    cout << "Type: " << (fs::is_directory(path) ? "Directory" : "File") << "\n";
    if (!fs::is_directory(path)) {
        cout << "Size: " << fs::file_size(path, ec) << " bytes\n";
    }
    auto ftime = fs::last_write_time(path, ec);
    if (!ec) {
        auto st = chrono::time_point_cast<chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now()
            + chrono::system_clock::now());
        time_t cftime = chrono::system_clock::to_time_t(st);
        cout << "Last modified: " << put_time(localtime(&cftime), "%Y-%m-%d %H:%M:%S") << "\n";
    }
    cout << "Permissions: " << permsToString(fs::status(path).permissions()) << "\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    while (true) {
        cout << "\n=== Mini File Explorer ===\n";
        cout << "1) List (names)\n";
        cout << "2) List (detailed)\n";
        cout << "3) Create file\n";
        cout << "4) Create directory\n";
        cout << "5) Delete file/dir\n";
        cout << "6) Rename\n";
        cout << "7) Copy\n";
        cout << "8) Move\n";
        cout << "9) View file content\n";
        cout << "10) Search (recursive)\n";
        cout << "11) File/dir info\n";
        cout << "12) Exit\n";
        cout << "Choose an option [1-12]: ";

        string choice;
        getline(cin, choice);
        if (choice.empty()) continue;

        if (choice == "1") {
            string p = inputPath("Enter directory (default .): ");
            listFiles(p);
            pause();
        } else if (choice == "2") {
            string p = inputPath("Enter directory (default .): ");
            listDetailed(p);
            pause();
        } else if (choice == "3") {
            createFile();
            pause();
        } else if (choice == "4") {
            createDirectory();
            pause();
        } else if (choice == "5") {
            deletePath();
            pause();
        } else if (choice == "6") {
            renamePath();
            pause();
        } else if (choice == "7") {
            copyPath();
            pause();
        } else if (choice == "8") {
            movePath();
            pause();
        } else if (choice == "9") {
            viewFile();
            pause();
        } else if (choice == "10") {
            searchRecursive();
            pause();
        } else if (choice == "11") {
            fileInfo();
            pause();
        } else if (choice == "12") {
            cout << "Goodbye!\n";
            break;
        } else {
            cout << "Invalid choice.\n";
        }
    }

    return 0;
}
