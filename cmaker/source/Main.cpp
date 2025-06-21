#include "Environment.hpp"

#include <argparse/argparse.hpp>
#include <fplus/fplus.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>
#include <libpreprocessor/Processor.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>

struct Feature
{
    std::string name;
    bool optional;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Feature, name, optional);
};

struct Kind
{
    std::string name;
    std::optional<std::vector<Feature>> features;
    std::optional<std::vector<std::string>> inherits;

    friend void to_json(nlohmann::json& json, Kind const& type)
    {
        json["name"] = type.name;
        if (type.features.has_value()) json["features"] = *type.features;
        if (type.inherits.has_value()) json["inherits"] = *type.inherits;
    }

    friend void from_json(nlohmann::json const& json, Kind& type)
    {
        json.at("name").get_to(type.name);
        if (json.count("features")) type.features = json.at("features").get<std::vector<Feature>>();
        if (json.count("inherits")) type.inherits = json.at("inherits").get<std::vector<std::string>>();
    };
};

struct Template
{
    std::string name;
    std::vector<Kind> kinds;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Template, name, kinds);
};

struct Language
{
    std::string name;
    std::vector<int> standards;
    std::vector<Template> templates;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Language, name, standards, templates);
};

liberror::Result<void> sanitize_argument_values(argparse::ArgumentParser const& parser, std::vector<Language> const& languages)
{
    auto maybeLanguage = std::ranges::find(languages, parser.get<std::string>("--lang"), &Language::name);
    if (maybeLanguage == languages.end())
    {
        return liberror::make_error("Language {} is not available.", parser.get<std::string>("--lang"));
    }

    if (std::ranges::find(maybeLanguage->standards, parser.get<int>("--std")) == maybeLanguage->standards.end())
    {
        return liberror::make_error("Standard {} is not available for {}.", parser.get<int>("--std"), parser.get<std::string>("--lang"));
    }

    auto maybeTemplate = std::ranges::find(maybeLanguage->templates, parser.get<std::string>("type"), &Template::name);
    if (maybeTemplate == maybeLanguage->templates.end())
    {
        return liberror::make_error("Template \"{}\" could not be found.", parser.get<std::string>("type"));
    }

    auto maybeTemplateKind = std::ranges::find(maybeTemplate->kinds, parser.get<std::string>("--kind"), &Kind::name);
    if (maybeTemplateKind == maybeTemplate->kinds.end())
    {
        return liberror::make_error("Kind \"{}\" is not avaiable for template \"{}\"", parser.get<std::string>("--kind"), parser.get<std::string>("type"));
    }

    std::ranges::all_of(parser.get<std::vector<std::string>>("--features"), [&] (std::string const& feature) {
        return
            !maybeTemplateKind->features ||
            std::ranges::find(*maybeTemplateKind->features, feature, &Feature::name) != (*maybeTemplateKind->features).end();
    });

    return {};
}

struct Configuration
{
    std::string name;
    std::string language;
    std::string standard;
    std::string type;
    std::string kind;
    std::vector<std::string> features;
};

Configuration configure_project(argparse::ArgumentParser const& parser)
{
    return Configuration {
        .name = parser.get<std::string>("--name"),
        .language = parser.get<std::string>("--lang"),
        .standard = [&] () {
            std::stringstream stream {};
            stream << parser.get<int>("--std");
            return stream.str();
        }(),
        .type = parser.get<std::string>("type"),
        .kind = parser.get<std::string>("--kind"),
        .features = parser.get<std::vector<std::string>>("--features")
    };
}

liberror::Result<bool> recursive_copy(std::filesystem::path const& source, std::filesystem::path const& destination)
{
    namespace fs = std::filesystem;

    if (!fs::exists(source)) return false;

    try
    {
        fs::copy(source, destination, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    }
    catch (std::exception const& exception)
    {
        return liberror::make_error(exception.what());
    }

    return true;
}

liberror::Result<void> recursive_copy_features(Configuration const& configuration, std::vector<Feature> const& features)
{
    namespace fs = std::filesystem;

    for (auto const& feature : features)
    {
        auto isRequired = !feature.optional;
        auto isPresent = std::ranges::find(configuration.features, feature.name) != configuration.features.end();
        if (isRequired || isPresent)
        {
            if (fs::exists(get_application_data_path() / "features" / feature.name))
                TRY(recursive_copy(get_application_data_path() / "features" / feature.name, configuration.name));
            else
                return liberror::make_error("Could not find feature \"{}\".", feature.name);
        }
    }

    return {};
}

liberror::Result<void> create_project_structure(Configuration const& configuration, std::vector<Language> const& languages)
{
    namespace fs = std::filesystem;

    if (fs::exists(configuration.name))
    {
        return liberror::make_error("Project \"{}\" already exists.", configuration.name);
    }

    auto projectLanguage = *std::ranges::find(languages, configuration.language, &Language::name);
    auto projectTemplate = *std::ranges::find(projectLanguage.templates, configuration.type, &Template::name);
    auto projectKind = *std::ranges::find(projectTemplate.kinds, configuration.kind, &Kind::name);

    if (projectKind.inherits.has_value())
    {
        for (auto const& inherited : *projectKind.inherits)
        {
            TRY(recursive_copy(get_application_data_path() / "templates" / configuration.type / inherited, configuration.name));
        }
    }

    TRY(recursive_copy(get_application_data_path() / "templates" / configuration.type / configuration.kind, configuration.name));

    if (projectKind.inherits.has_value())
    {
        for (auto const& kind : *projectKind.inherits)
        {
            auto parentKind = *std::ranges::find(projectTemplate.kinds, kind, &Kind::name);
            TRY(recursive_copy_features(configuration, *parentKind.features));
        }
    }

    if (projectKind.features.has_value())
    {
        TRY(recursive_copy_features(configuration, *projectKind.features));
    }

    return {};
}

std::string replace(std::string content, std::pair<std::string, std::string> const& wildcard)
{
    for (auto position = content.find(wildcard.first); position != std::string::npos; position = content.find(wildcard.first))
    {
        auto first = std::next(content.begin(), static_cast<int>(position));
        auto last = std::next(first, static_cast<int>(wildcard.first.size()));
        content.replace(first, last, wildcard.second);
    }

    return content;
}

void replace_filename_wildcards(std::filesystem::path const& path, std::unordered_map<std::string, std::string> const& wildcards)
{
    namespace fs = std::filesystem;

    auto fnRename = [](auto&& entry, auto&& wildcard) {
        auto path = entry.parent_path();
        auto name = entry.filename().string();
        fs::rename(entry, path.append(replace(name, wildcard)));
    };

    for (auto const& entry : fs::directory_iterator(path) | std::views::transform(&fs::directory_entry::path))
    {
        if (fs::is_directory(entry))
        {
            replace_filename_wildcards(entry, wildcards);
        }

        auto fnKeepWildcardIfPresent = std::views::filter([&](auto&& wildcard) {
            return entry.filename().string().contains(wildcard.first);
        });

        std::ranges::for_each(wildcards | fnKeepWildcardIfPresent, std::bind_front(fnRename, entry));
    }
}

void replace_file_wildcards(std::filesystem::path const& path, std::unordered_map<std::string, std::string> const& wildcards)
{
    namespace fs = std::filesystem;

    auto fnReplace = [](auto&& entry, auto&& wildcard) {
        std::stringstream contentStream {};
        contentStream << std::ifstream(entry).rdbuf();
        std::ofstream outputStream(entry, std::ios::trunc);
        outputStream << replace(contentStream.str(), wildcard);
    };

    auto fnFilterRegularFile = std::views::filter([](auto&& entry) {
        return fs::is_regular_file(entry);
    });

    for (auto const& entry : fs::recursive_directory_iterator(path) | std::views::transform(&fs::directory_entry::path) | fnFilterRegularFile)
    {
        std::ranges::for_each(wildcards, std::bind_front(fnReplace, entry));
    }
}

liberror::Result<void> preprocess_project_files(Configuration const& configuration)
{
    using namespace std::literals;
    namespace fs = std::filesystem;

    libpreprocessor::PreprocessorContext context {
        .environmentVariables = {
            { "ENV:LANGUAGE", configuration.language },
            { "ENV:STANDARD", configuration.standard },
            { "ENV:KIND", configuration.type },
            { "ENV:MODE", configuration.kind },
            { "ENV:FEATURES", fplus::join(","s, configuration.features) },
        }
    };

    auto fnFilterRegularFile = std::views::filter([](auto&& entry) { return fs::is_regular_file(entry); });
    for (auto const& entry : fs::recursive_directory_iterator(configuration.name) | fnFilterRegularFile)
    {
        auto const content = TRY(libpreprocessor::process(entry.path(), context));
        std::ofstream outputStream(entry.path());
        outputStream << content;
    }

    static std::unordered_map<std::string, std::string> const& wildcards {
        { "!PROJECT!", configuration.name },
        { "!LANGUAGE!", configuration.language },
        { "!STANDARD!", configuration.standard }
    };

    replace_filename_wildcards(configuration.name, wildcards);
    replace_file_wildcards(configuration.name, wildcards);

    return {};
}

liberror::Result<void> create_project(Configuration const& configuration, std::vector<Language> const& languages)
{
    TRY(create_project_structure(configuration, languages));
    TRY(preprocess_project_files(configuration));
    return {};
}

liberror::Result<void> safe_main(std::span<char const*> arguments)
{
    argparse::ArgumentParser parser(PROJECT_NAME);

    parser.add_description("Create C++ and C projects.");

    parser.add_argument("-n", "--name").help("project name").required();
    parser.add_argument("type").help("the type of project to be created").choices("executable", "library").default_value("executable");
    parser.add_argument("-k", "--kind").help("project kind").default_value("common");
    parser.add_argument("-l", "--lang").default_value("c++");
    parser.add_argument("--std").scan<'i', int>().default_value(23);
    parser.add_argument("--features").help("features used in the project").nargs(argparse::nargs_pattern::at_least_one);

    try
    {
        parser.parse_args(static_cast<int>(arguments.size()), arguments.data());
    }
    catch (std::exception const& exception)
    {
        return liberror::make_error(exception.what());
    }

    std::vector<Language> languages {};
    nlohmann::json languagesJson = nlohmann::json::parse(std::ifstream(get_application_data_path() / "languages.json"));
    std::ranges::copy(languagesJson["languages"], std::back_inserter(languages));

    TRY(sanitize_argument_values(parser, languages));
    auto configuration = configure_project(parser);
    TRY(create_project(configuration, languages));

    return {};
}

int main(int argc, char const** argv)
{
    auto result = safe_main(std::span<char const*>(argv, size_t(argc)));

    if (!result.has_value())
    {
        fmt::println("{}", result.error().message());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
