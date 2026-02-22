#include <iostream>
#include <string_view>
#include <optional>
#include <fstream>
#include <limits>
#include <charconv>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

void exit_with_failure(std::string msg)
{
    std::cerr << msg << '\n';
    getchar();
    std::exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    constexpr auto byte_per_mb = 1024 * 1024;
    std::optional<size_t> target_size;
    fs::path source_file_path;
    fs::path target_folder_path;

    if(argc < 2)
    {
        std::cout << "path to source file\n>";
        std::cin >> source_file_path;
        std::cout << "path of target folder\n>";
        std::cin >> target_folder_path;

        std::cout <<
            "target size in bytes, or megabytes(add 'mb' at the end of number)\n"
            "if you want to fill all space just press enter\n>";
        std::string input_size;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, input_size);

        if(!input_size.empty())
        {
            size_t value;
            constexpr std::string_view mb_ending = "mb";
            bool is_mb = input_size.ends_with(mb_ending);
            auto result = std::from_chars(
                input_size.data(),
                input_size.data() + input_size.size() - (is_mb ? mb_ending.size() : 0),
                value
            );
            if(result.ec != std::errc{})
                exit_with_failure("bad space\n");
            target_size = value * (is_mb ? byte_per_mb : 1);
        }
    }
    else for(int i = 1; i < argc; i++)
    {
        std::string_view arg = argv[i];
        if(!arg.empty() || arg[0] != '-')
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
            if(arg_value.empty() || arg_value[0] == '-')
                exit_with_failure("missing argument value: " + std::string(arg));

            if(arg == "-sf")
                source_file_path = arg_value;
            else if(arg == "-tf")
                target_folder_path = arg_value;
            else if(arg == "-s" || arg == "-sm")
            {
                size_t value;
                auto result = std::from_chars(
                    arg_value.data(),
                    arg_value.data() + arg_value.size(),
                    value
                );
                if(result.ec != std::errc{})
                    exit_with_failure("bad space\n");
                if(arg == "-sm") value *= byte_per_mb;
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
        uint64_t size;
    } source_file;

    if(std::ifstream stream{source_file_path, std::ios::binary}; stream.is_open())
    {
        auto s = fs::file_size(source_file_path);
        source_file.size = s == 0 ? 1 : s;
        source_file.data.assign(std::istreambuf_iterator<char>{stream}, {});
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
    {
        if(!fs::is_directory(target_folder_path))
            exit_with_failure("target folder is not directory actually");
    }
    else if(!fs::create_directory(target_folder_path))
        exit_with_failure("failed to create target folder\n");

    auto file_count =
        target_size.value_or(fs::space(target_folder_path).available) /
        source_file.size;

    auto total_size = file_count * source_file.size;
    std::cout
        << "file target count: " << file_count << '\n'
        << "total spam size: " << total_size / byte_per_mb << " MBs "
        << total_size % byte_per_mb << " Bytes\n"
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