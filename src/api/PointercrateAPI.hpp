#pragma once

#include <Geode/Geode.hpp>
#include <matjson.hpp>
#include <string>
#include <vector>
#include <optional>

namespace pointercrate {

constexpr const char* BASE_URL = "https://pointercrate.com/api";
constexpr int PAGE_SIZE = 10;

struct SimplePlayer {
    int id = 0;
    std::string name;
    bool banned = false;
};

struct Demon {
    int id = 0;
    int position = 0;
    std::string name;
    int requirement = 0;
    std::string video;
    SimplePlayer publisher;
    SimplePlayer verifier;
    int levelId = 0;

    static Demon fromJson(matjson::Value const& json);
};

struct Nationality {
    std::string countryCode;
    std::string nation;
    std::string subdivision;
    bool has = false;
};

struct RankedPlayer {
    int id = 0;
    std::string name;
    int rank = 0;
    double score = 0.0;
    Nationality nationality;

    static RankedPlayer fromJson(matjson::Value const& json);
};

struct PageLinks {
    std::optional<std::string> next;
    std::optional<std::string> prev;
    std::optional<std::string> first;
    std::optional<std::string> last;

    static PageLinks fromHeader(std::optional<std::string> const& header);
};

std::string demonsListedUrl();
std::string playersRankingUrl();
std::string playerByNameUrl(std::string const& name);

}
