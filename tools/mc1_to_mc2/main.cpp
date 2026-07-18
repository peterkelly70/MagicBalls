#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;
using Environment = std::unordered_map<std::string, std::string>;

struct Config
{
    fs::path mc1_path;
    fs::path mc2_path;
    fs::path output_path;
    std::optional<int> level;
    bool convert_all = false;
    bool verbose = false;
};

std::string trim(const std::string& value)
{
    const std::string whitespace = " \t\r\n";
    const std::size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos)
    {
        return "";
    }

    const std::size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

std::string remove_quotes(const std::string& value)
{
    if (value.size() < 2)
    {
        return value;
    }

    const bool double_quoted = value.front() == '"' && value.back() == '"';
    const bool single_quoted = value.front() == '\'' && value.back() == '\'';
    if (double_quoted || single_quoted)
    {
        return value.substr(1, value.size() - 2);
    }

    return value;
}

char lower_character(unsigned char character)
{
    return static_cast<char>(std::tolower(character));
}

bool parse_boolean(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), lower_character);
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

Environment load_dotenv(const fs::path& path)
{
    Environment values;
    if (!fs::exists(path))
    {
        return values;
    }

    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Unable to open environment file: " + path.string());
    }

    std::string line;
    while (std::getline(input, line))
    {
        line = trim(line);
        if (line.empty() || line.front() == '#')
        {
            continue;
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = remove_quotes(trim(line.substr(separator + 1)));
        if (!key.empty())
        {
            values[key] = value;
        }
    }

    return values;
}

std::optional<std::string> environment_value(const Environment& values, const std::string& key)
{
    const auto found = values.find(key);
    if (found == values.end() || found->second.empty())
    {
        return std::nullopt;
    }

    return found->second;
}

Config load_environment_config(const fs::path& env_path)
{
    const Environment values = load_dotenv(env_path);
    Config config;

    if (const auto value = environment_value(values, "MC1_PATH"))
    {
        config.mc1_path = *value;
    }
    if (const auto value = environment_value(values, "MC2_PATH"))
    {
        config.mc2_path = *value;
    }
    if (const auto value = environment_value(values, "OUTPUT_PATH"))
    {
        config.output_path = *value;
    }
    if (const auto value = environment_value(values, "DEFAULT_LEVEL"))
    {
        config.level = std::stoi(*value);
    }
    if (const auto value = environment_value(values, "VERBOSE"))
    {
        config.verbose = parse_boolean(*value);
    }

    return config;
}

std::string require_argument(int argc, char* argv[], int& index, const std::string& option)
{
    if (index + 1 >= argc)
    {
        throw std::runtime_error("Missing value for " + option);
    }

    ++index;
    return argv[index];
}

fs::path find_environment_path(int argc, char* argv[])
{
    for (int index = 1; index < argc; ++index)
    {
        if (std::string(argv[index]) == "--env")
        {
            return require_argument(argc, argv, index, "--env");
        }
    }

    return ".env";
}

void apply_command_line(Config& config, int argc, char* argv[])
{
    for (int index = 1; index < argc; ++index)
    {
        const std::string option = argv[index];

        if (option == "--env")
        {
            require_argument(argc, argv, index, option);
        }
        else if (option == "--mc1")
        {
            config.mc1_path = require_argument(argc, argv, index, option);
        }
        else if (option == "--mc2-template")
        {
            config.mc2_path = require_argument(argc, argv, index, option);
        }
        else if (option == "--output")
        {
            config.output_path = require_argument(argc, argv, index, option);
        }
        else if (option == "--level")
        {
            config.level = std::stoi(require_argument(argc, argv, index, option));
            config.convert_all = false;
        }
        else if (option == "--all")
        {
            config.convert_all = true;
            config.level.reset();
        }
        else if (option == "--verbose")
        {
            config.verbose = true;
        }
        else if (option == "--help" || option == "-h")
        {
            std::cout
                << "Usage: mc1_to_mc2 [options]\n"
                << "  --env PATH          Environment file, default .env\n"
                << "  --mc1 PATH          Magic Carpet Plus installation\n"
                << "  --mc2-template PATH Magic Carpet 2 template installation\n"
                << "  --output PATH       Converted installation destination\n"
                << "  --level N           Convert one level\n"
                << "  --all               Convert all supported levels\n"
                << "  --verbose           Print additional details\n";
            std::exit(0);
        }
        else
        {
            throw std::runtime_error("Unknown option: " + option);
        }
    }
}

void require_existing_path(const fs::path& path, const std::string& label)
{
    if (path.empty())
    {
        throw std::runtime_error(label + " is missing");
    }
    if (!fs::exists(path))
    {
        throw std::runtime_error(label + " does not exist: " + path.string());
    }
}

fs::path normalized_absolute_path(const fs::path& path)
{
    return fs::absolute(path).lexically_normal();
}

void validate_config(const Config& config)
{
    require_existing_path(config.mc1_path, "MC1_PATH/--mc1");
    require_existing_path(config.mc2_path, "MC2_PATH/--mc2-template");

    if (config.output_path.empty())
    {
        throw std::runtime_error("OUTPUT_PATH/--output is missing");
    }

    if (!config.convert_all && !config.level.has_value())
    {
        throw std::runtime_error("Select a level with --level, --all, or DEFAULT_LEVEL");
    }

    if (config.level.has_value() && *config.level < 0)
    {
        throw std::runtime_error("Level number cannot be negative");
    }

    const fs::path output = normalized_absolute_path(config.output_path);
    if (output == normalized_absolute_path(config.mc1_path) ||
        output == normalized_absolute_path(config.mc2_path))
    {
        throw std::runtime_error("Output must not overwrite either source installation");
    }
}

void print_config(const Config& config, const fs::path& env_path)
{
    std::cout << "Environment: " << env_path << '\n';
    std::cout << "MC1 source:  " << config.mc1_path << '\n';
    std::cout << "MC2 template:" << config.mc2_path << '\n';
    std::cout << "Output:      " << config.output_path << '\n';
    std::cout << "Selection:   ";
    if (config.convert_all)
    {
        std::cout << "all levels\n";
    }
    else
    {
        std::cout << "level " << *config.level << '\n';
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const fs::path env_path = find_environment_path(argc, argv);
        Config config = load_environment_config(env_path);
        apply_command_line(config, argc, argv);
        validate_config(config);
        print_config(config, env_path);

        std::cout << "Configuration valid. MC1 to MC2 conversion is not implemented yet.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "mc1_to_mc2: " << error.what() << '\n';
        return 1;
    }
}
