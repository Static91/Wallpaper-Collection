#include <fstream>
#include <ios>
#include <iostream>
#include <filesystem>
#include <list>
#include <regex>
#include <string>

namespace fs = std::filesystem;

const int MAX_LINKS_IN_FILE = 25;

std::list<std::string> valid_paths;

void fetch_all_paths() {
    try {
        for (auto it = fs::recursive_directory_iterator(fs::current_path());
            it != fs::recursive_directory_iterator(); ++it) {

            if (it->is_directory()) {
                if (it->path().filename() == ".git" || it->path().filename() == "references") {
                    it.disable_recursion_pending();
                    continue;
                }
                valid_paths.push_back(fs::relative(it->path(), fs::current_path()).string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
}

bool check_if_only_directories(std::string path){
    try{
        for (const auto& entry: fs::directory_iterator(path)){
            if(!entry.is_directory()){
                return false;
            }
        }
    }catch (const fs::filesystem_error& e){
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
    return true;
}

std::string get_clean_prefix(std::string path_str) {
    std::string clean = std::regex_replace(path_str, std::regex("[/\\\\]"), "_");
    clean = std::regex_replace(clean, std::regex(" +"), "_");
    return clean;
}


void register_on_main_file(std::string category_name, std::string unique_filename){
    std::ofstream file(fs::current_path() / "README.md", std::ios::app);
    if(file.is_open()){
        std::string fixed_link = std::regex_replace(unique_filename, std::regex(" "), "%20");
        file << "- [" << category_name << " - " << unique_filename << "](references/" << fixed_link << ".md)\n\n";
    }
    file.close();
}

void mini_writer(std::string path_str, std::string file_name, std::list<fs::path> paths){
    std::string prefix = get_clean_prefix(path_str);
    std::string unique_file_name = prefix + "_" + file_name;

    fs::path ref_dir = fs::current_path() / "references";
        fs::path md_path = ref_dir / (unique_file_name + ".md");

    std::ofstream outFile(md_path);
    if(outFile.is_open()){
        for(const auto& path_entry: paths){
            fs::path rel_path = fs::relative(path_entry, md_path.parent_path());
            std::string rel_str = rel_path.generic_string();

            std::string fixed_path = std::regex_replace(rel_str, std::regex(" "), "%20");
            std::string filename = path_entry.filename().string();

            outFile << "!["<< filename <<"]" << "(" << fixed_path << ")\n\n";
        }
    }
    outFile.close();

    register_on_main_file(path_str, unique_file_name);
}

void write_md(std::string path){
    if (check_if_only_directories(path)) return;
    int file_count = 1;
    std::list<fs::path> to_be_append;

    for (auto entry : fs::directory_iterator(path)){
        if (entry.is_directory()) continue;

        std::string filename = entry.path().filename().string();
        if (filename == ".git" || filename == "README.md" || filename.rfind("list", 0) == 0) continue;

        to_be_append.push_back(entry.path());

        if (to_be_append.size() == MAX_LINKS_IN_FILE) {
            std::string file_name = "list" + std::to_string(file_count);
            mini_writer(path, file_name, to_be_append);
            to_be_append.clear();
            file_count++;
        }
    }
    if (!to_be_append.empty()) {
        std::string file_name = "list" + std::to_string(file_count);
        mini_writer(path, file_name, to_be_append);
    }
}

int main() {
    std::cout << "\nIndexing...\n";

    fs::create_directories(fs::current_path() / "references");

    std::ofstream readme(fs::current_path() / "README.md");
    readme << R"(
### Segregated into folders of 1000 files so that github actually loads the images all at once.


* README preview pages generation command
```sh
g++ indexer.cpp -o main
./main
```

### Wallpaper Index

)";
    readme.close();

    fetch_all_paths();

    for (const auto& path : valid_paths) {
        write_md(path);
    }
    std::cout << "\nCompleted!!\n";

    return 0;
}
