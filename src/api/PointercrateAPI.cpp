#include "PointercrateAPI.hpp"

using namespace geode::prelude;

namespace pointercrate {

static SimplePlayer parseSimplePlayer(matjson::Value const& json) {
    SimplePlayer player;
    if (!json.isObject()) return player;
    player.id = json["id"].asInt().unwrapOr(0);
    player.name = json["name"].asString().unwrapOr("");
    player.banned = json["banned"].asBool().unwrapOr(false);
    return player;
}

Demon Demon::fromJson(matjson::Value const& json) {
    Demon demon;
    demon.id = json["id"].asInt().unwrapOr(0);
    demon.position = json["position"].asInt().unwrapOr(0);
    demon.name = json["name"].asString().unwrapOr("Unknown");
    demon.requirement = json["requirement"].asInt().unwrapOr(0);
    demon.video = json["video"].isString() ? json["video"].asString().unwrapOr("") : "";
    demon.levelId = json["level_id"].asInt().unwrapOr(0);
    if (json.contains("publisher")) demon.publisher = parseSimplePlayer(json["publisher"]);
    if (json.contains("verifier")) demon.verifier = parseSimplePlayer(json["verifier"]);
    return demon;
}

RankedPlayer RankedPlayer::fromJson(matjson::Value const& json) {
    RankedPlayer player;
    player.id = json["id"].asInt().unwrapOr(0);
    player.name = json["name"].asString().unwrapOr("Unknown");
    player.rank = json["rank"].asInt().unwrapOr(0);
    player.score = json["score"].asDouble().unwrapOr(0.0);
    if (json.contains("nationality") && json["nationality"].isObject()) {
        auto nat = json["nationality"];
        player.nationality.has = true;
        player.nationality.countryCode = nat["country_code"].asString().unwrapOr("");
        player.nationality.nation = nat["nation"].asString().unwrapOr("");
        if (nat.contains("subdivision") && nat["subdivision"].isString()) {
            player.nationality.subdivision = nat["subdivision"].asString().unwrapOr("");
        }
    }
    return player;
}

static std::optional<std::string> extractLink(std::string const& header, std::string const& rel) {
    std::string needle = "rel=\"" + rel + "\"";
    size_t pos = 0;
    while (pos < header.size()) {
        size_t start = header.find('<', pos);
        if (start == std::string::npos) break;
        size_t end = header.find('>', start);
        if (end == std::string::npos) break;
        size_t segEnd = header.find(',', end);
        std::string segment = header.substr(start, (segEnd == std::string::npos ? header.size() : segEnd) - start);
        if (segment.find(needle) != std::string::npos) {
            return header.substr(start + 1, end - start - 1);
        }
        pos = (segEnd == std::string::npos) ? header.size() : segEnd + 1;
    }
    return std::nullopt;
}

PageLinks PageLinks::fromHeader(std::optional<std::string> const& header) {
    PageLinks links;
    if (!header.has_value()) return links;
    links.next = extractLink(header.value(), "next");
    links.prev = extractLink(header.value(), "prev");
    links.first = extractLink(header.value(), "first");
    links.last = extractLink(header.value(), "last");
    return links;
}

std::string demonsListedUrl() {
    return fmt::format("{}/v2/demons/listed?limit={}", BASE_URL, PAGE_SIZE);
}

std::string playersRankingUrl() {
    return fmt::format("{}/v1/players/ranking/?limit={}", BASE_URL, PAGE_SIZE);
}

std::string playerByNameUrl(std::string const& name) {
    return fmt::format("{}/v1/players/ranking/?name_contains={}&limit=1", BASE_URL, web::urlEncode(name));
}

}
