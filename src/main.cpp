#include <iostream>
#include <string_view>
#include <optional>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

void exit_with_failure(std::string msg)
{
    std::cerr << msg << '\n';
    std::exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    std::optional<size_t> target_size;
    fs::path source_file_path;
    fs::path target_folder_path;

    for(int i = 1; i < argc; i++)
    {
        std::string_view arg = argv[i];
        if(arg[0] != '-')
        {
            std::cerr << "unknown argument: " << arg << "\n-help to see usage";
            return EXIT_FAILURE;
        }
        if(arg == "-help" || arg == "-h")
        {
            std::cout << "usage: spamfile [options]\n"
                "\t-sf: path to source file\n"
                "\t-tf: path to target folder\n"
                "\t-s: size in bytes (if missing gonna use all avalible space)\n"
                "\t-sm: same as -s but in MB\n"
                "\t-h: this message\n";
            return EXIT_SUCCESS;
        }
        else if(i + 1 < argc)
        {
            std::string_view arg_value = argv[++i];
            if(arg_value[0] == '-')
                exit_with_failure("missing argument value: " + std::string(arg));

            if(arg == "-sf")
                source_file_path = arg_value;
            else if(arg == "-tf")
                target_folder_path = arg_value;
            else if(arg == "-s" || arg == "-sm")
            {
                size_t value;
                if(auto result = std::from_chars(arg_value.data(), arg_value.data() + arg_value.size(), value);
                    result.ec != std::errc())
                    exit_with_failure("bad space\n");
                if(arg == "-sm") value *= 1024 * 1024;
                target_size = value;
            }
        }
        else
            exit_with_failure("missing argument value: " + std::string(arg));
    }
    if(target_folder_path.empty() || source_file_path.empty())
        exit_with_failure("target folder or source file was not provided");

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
        exit_with_failure("unable to open source file\n" + ec.message());
    }

    if(fs::exists(target_folder_path))
        if(!fs::is_directory(target_folder_path))
            exit_with_failure("target folder is not directory actually");
    else if(!fs::create_directory(target_folder_path))
        exit_with_failure("failed to create target folder\n");

    auto file_size = fs::file_size(source_file_path);
    if(file_size == 0) file_size++;
    auto file_count =
        target_size.value_or(fs::space(target_folder_path).available) /
        file_size;

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
        static auto length_fmt = std::to_string(file_count).size();
        oss << std::setw(length_fmt) << std::setfill('0') << i;
        auto name = source_file.name + oss.str() + source_file.ext;
        if(std::ofstream os{target_folder_path / name, std::ios::binary}; os.is_open())
            os.write(source_file.data.data(), source_file.data.size());
        else
            exit_with_failure("unable to open file " + std::to_string(i));
    }

    return EXIT_SUCCESS;
}