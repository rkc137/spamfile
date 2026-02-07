#include <iostream>
#include <string_view>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    std::optional<size_t> space;
    fs::path source_file_path;
    fs::path target_folder_path;

    for(int i = 1; i < argc; i++)
    {
        std::string_view arg = argv[i];
        if(arg[0] == '-' && i < argc + 1)
        {
            std::string_view arg_value = argv[++i];
            if(arg == "-sf")
                source_file_path = arg_value;
            else if(arg == "-tf")
                target_folder_path = arg_value;
            // TODO: if(arg == "-s") space = arg_value
        }
    }

    struct {
        std::string name, ext, data;
    } source_file;

    {
        using It = std::istreambuf_iterator<char>;
        std::ifstream s{source_file_path, std::ios::binary};
        source_file.data.assign(It(s), It());
        source_file.name = source_file_path.stem().string();
        source_file.ext = source_file_path.extension().string();
    }

    auto file_count =
        fs::space(target_folder_path).available /
        fs::file_size(source_file_path);
    std::clog << "file target count: " << file_count << '\n';

    for(size_t i = 0; i < file_count; i++)
    {
        if(i % 10 == 0)
        {
            auto text = std::to_string(i) + " of " + std::to_string(file_count);
            std::clog << text << '\r';
        }
        std::ostringstream oss;
        oss << std::setw(std::to_string(file_count).size()) << std::setfill('0') << i;
        auto name = source_file.name + oss.str() + source_file.ext;
        std::ofstream os{target_folder_path / name, std::ios::binary};
        os << source_file.data;
    }

    return 0;
}