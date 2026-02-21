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
        if(arg[0] != '-')
        {
            std::cerr << "unknow argument: " << arg << "\n-help to see usage";
            return EXIT_FAILURE;
        }
        if(arg == "-help" || arg == "-h")
            std::cout << "usage: spamfile [options]\n"
                "\t-sf: path to source file"
                "\t-tf: path to target folder"
                "\t-s: size in bytes (if missing gonna use all avalible space)"
                "\t-sm: same as -s but in MB"
                "\t-h: this message";
        else if(i < argc + 1)
        {
            std::string_view arg_value = argv[++i];
            if(arg_value[0] == '-')
            {
                std::cerr << "missing argument value: " << arg;
                return EXIT_FAILURE;
            }

            if(arg == "-sf")
                source_file_path = arg_value;
            else if(arg == "-tf")
                target_folder_path = arg_value;
            else if(arg == "-s" || arg == "-sm")
            {
                size_t value;
                if(auto result = std::from_chars(arg_value.data(), arg_value.data() + arg_value.size(), value);
                    result.ec == std::errc::invalid_argument)
                {
                    std::cerr << "bad space\n";
                    return EXIT_FAILURE;
                }
                if(arg == "-sm") value *= 1024 * 1024;
                space = value;
            }
        }
        else
        {
            std::cerr << "missing argument value: " << arg;
            return EXIT_FAILURE;
        }
    }

    struct {
        std::string name, ext, data;
    } source_file;

    if(std::ifstream s{source_file_path, std::ios::binary}; s.is_open())
    {
        source_file.data.assign(std::istreambuf_iterator<char>(s), {});
        source_file.name = source_file_path.stem().string();
        source_file.ext = source_file_path.extension().string();
    }
    else
    {
        std::error_code ec;
        auto status = fs::status(source_file_path, ec);
        std::cerr << "unable to open source file\n" << ec.message() << '\n';
        return EXIT_FAILURE;
    }

    if(!fs::exists(target_folder_path) && !fs::create_directory(target_folder_path))
    {
        std::cerr << "failed to create target folder\n";
        return EXIT_FAILURE;
    }
    else
    {
        std::error_code ec;
        auto status = fs::status(target_folder_path, ec);
        if(ec)
        {
            std::cerr << "unable to open target folder\n" << ec.message() << '\n';
            return EXIT_FAILURE;
        }
    }

    auto file_count =
        space.value_or(fs::space(target_folder_path).available) /
        fs::file_size(source_file_path);

    std::cout
        << "file target count: " << file_count << '\n'
        << "total spam size: " << (file_count * source_file.data.size()) / (1024 * 1024) << "MBs\n"
        << "continue? (N/Y)";
    char YorN;
    std::cin >> YorN;
    if(std::toupper(YorN) != 'Y')
        return EXIT_SUCCESS;

    for(size_t i = 0; i < file_count; i++)
    {
        if(i % 10 == 0)
        {
            auto text = std::to_string(i) + " of " + std::to_string(file_count);
            std::cout << text << '\r';
        }
        std::ostringstream oss;
        static auto length_fmt = std::setw(std::to_string(file_count).size());
        static auto fill_fmt = std::setfill('0');
        oss << length_fmt << fill_fmt << i;
        auto name = source_file.name + oss.str() + source_file.ext;
        if(std::ofstream os{target_folder_path / name, std::ios::binary}; os.is_open())
            os << source_file.data;
        else
        {
            std::cerr << "unable to open file " << i << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}