#include "util.h"
#include <fmt/core.h>
#include <set>
#include <string>
#include <vector>
#include <filesystem>

namespace savepatcher::arguments {

struct ArgumentBase {
    constexpr virtual ~ArgumentBase() = default;
    constexpr ArgumentBase() = default;
    constexpr ArgumentBase(const ArgumentBase &) = default;
};

/**
 * @brief The container type for static command line arguments
 */
template <typename T> struct Argument : ArgumentBase {
    const std::string_view name{};
    const std::string_view description{};
    const std::string_view briefDescription{};
    bool set{false};
    T value{};

    constexpr Argument() = default;
    constexpr Argument(std::string_view name) : name(name) {}
    constexpr Argument(std::string_view name, std::string_view description) : name(name), description(description) {}
    constexpr Argument(std::string_view name, std::string_view briefDescription, std::string_view description) : name(name), description(description), briefDescription{briefDescription} {}
};

/**
 * @brief A command line argument parser
 */
class ArgumentParser {
  private:
    const std::vector<std::string_view> rawArguments;             //!< The arguments passed to the program
    std::vector<std::shared_ptr<ArgumentBase>> argumentContainer; //!< The container with all static arguments
    std::string_view programName;                                 //!< The name of the program
    std::string briefUsageDescription;                            //!< A one line description of the arguments
    std::string usageDescription;                                 //!< A full description of all arguments

    /**
     * @brief Check if an argument is set
     */
    constexpr bool isArgued(std::string_view argumentName) const {
        return std::find(rawArguments.begin(), rawArguments.end(), argumentName) == rawArguments.end() ? false : true;
    }

    /**
     * @brief Get the value of the argument after the given name
     */
    constexpr std::string_view getNextArgument(std::string_view argumentName) const {
        const auto nextArgPos{static_cast<size_t>(std::distance(rawArguments.begin(), std::find(rawArguments.begin(), rawArguments.end(), argumentName)) + 1)};
        if (nextArgPos >= rawArguments.size())
            throw exception("Missing argument value for '{}'", argumentName);

        return rawArguments[nextArgPos];
    }

    void parse(Argument<int> &arg) const {
        try {
            arg.value = std::stoi(getNextArgument(arg.name).data());
        } catch (const std::exception) {
            throw exception("Invalid value for '{}'", arg.name);
        }
    }

    constexpr void parse(Argument<bool> &arg) const {
        arg.value = true;
    }

    constexpr void parse(Argument<std::string_view> &arg) const {
        arg.value = getNextArgument(arg.name);
    }

    void parse(Argument<std::filesystem::path> &arg) const {
        auto path{std::filesystem::path(getNextArgument(arg.name))};
        if (!std::filesystem::exists(path))
            throw exception("Invalid argument for '{}': path '{}' does not exist!", arg.name, path.generic_string());
        arg.value = path;
    }

  public:
    ArgumentParser(int argc, char **argv) : rawArguments{argv, argv + argc}, programName{argv[0]} {}

    /**
     * @brief Append an argument to the argument container, and parse it if it is set
     */
    template <typename T> constexpr void add(Argument<T> &arg) {
        if (!arg.description.empty())
            usageDescription += fmt::format("  {}: {}\n", arg.name, arg.description);
        if (!arg.briefDescription.empty())
            briefUsageDescription += fmt::format("{} {} ", arg.name, arg.briefDescription);
        if (isArgued(arg.name)) {
            parse(arg);
            arg.set = true;
        }

        argumentContainer.emplace_back(std::make_shared<Argument<T>>(arg));
    }

    template <typename T> constexpr void add(const std::initializer_list<Argument<T>> arguments) {
        for (auto arg : arguments)
            add(arg);
    }

    /**
     * @brief Find an argument by name
     */
    template <typename T> Argument<T> constexpr find(std::string_view name) const {
        const auto itr{std::find_if(argumentContainer.begin(), argumentContainer.end(), [name](const auto &ptr) { return std::static_pointer_cast<Argument<T>>(ptr)->name == name; })};
        if (itr == argumentContainer.end())
            return Argument<T>{};

        return *std::static_pointer_cast<Argument<T>>(*itr);
    }

    /**
     * @brief Get the brief and full description of all arguments
     */
    constexpr std::tuple<std::string_view, std::string_view, std::string_view> getUsage() const {
        return std::make_tuple(programName.data(), briefUsageDescription.data(), usageDescription.data());
    }
};

} // namespace savepatcher::arguments