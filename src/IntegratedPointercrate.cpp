#include "IntegratedPointercrate.hpp"
#include <fmt/format.h>
#include <algorithm>
#include <cctype>

using namespace geode::prelude;

std::vector<PCDemon> IntegratedPointercrate::demons;
std::vector<PCRankedPlayer> IntegratedPointercrate::players;
bool IntegratedPointercrate::demonsLoaded = false;
bool IntegratedPointercrate::playersLoaded = false;

namespace {
    constexpr const char* DEMONS_URL = "https://pointercrate.com/api/v2/demons/listed/?limit=100";
    constexpr const char* PLAYERS_URL = "https://pointercrate.com/api/v1/players/ranking/?limit=100";

    std::string urlEncode(std::string const& s) {
        std::string out;
        const char hex[] = "0123456789ABCDEF";
        for (unsigned char c : s) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                out += c;
            } else {
                out += '%';
                out += hex[c >> 4];
                out += hex[c & 0xF];
            }
        }
        return out;
    }
}

void IntegratedPointercrate::loadDemons(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    demons.clear();
    listener.spawn(
        web::WebRequest().get(DEMONS_URL),
        [success = std::move(success), failure = std::move(failure)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            auto jsonRes = res.json();
            if (!jsonRes.isOk()) return failure(-1);
            auto arr = jsonRes.unwrap().asArray();
            if (!arr.isOk()) return failure(-1);

            for (auto& demonJson : arr.unwrap()) {
                auto id = demonJson["level_id"].asInt().unwrapOr(0);
                auto position = demonJson["position"].asInt().unwrapOr(0);
                auto name = demonJson["name"].asString().unwrapOr("");
                if (id <= 0 || position <= 0 || name.empty()) continue;
                demons.push_back(PCDemon(id, position, std::move(name)));
            }

            std::ranges::sort(demons, [](PCDemon const& a, PCDemon const& b) {
                return a.position < b.position;
            });

            demonsLoaded = true;
            success();
        }
    );
}

void IntegratedPointercrate::loadPlayers(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    players.clear();
    listener.spawn(
        web::WebRequest().get(PLAYERS_URL),
        [success = std::move(success), failure = std::move(failure)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            auto jsonRes = res.json();
            if (!jsonRes.isOk()) return failure(-1);
            auto arr = jsonRes.unwrap().asArray();
            if (!arr.isOk()) return failure(-1);

            for (auto& playerJson : arr.unwrap()) {
                auto rank = playerJson["rank"].asInt().unwrapOr(0);
                auto name = playerJson["name"].asString().unwrapOr("");
                if (rank <= 0 || name.empty()) continue;

                PCRankedPlayer player;
                player.id = playerJson["id"].asInt().unwrapOr(0);
                player.rank = rank;
                player.name = std::move(name);
                player.score = playerJson["score"].asDouble().unwrapOr(0.0);
                if (playerJson["nationality"].isObject()) {
                    player.countryCode = playerJson["nationality"]["country_code"].asString().unwrapOr("");
                }
                players.push_back(std::move(player));
            }

            std::ranges::sort(players, [](PCRankedPlayer const& a, PCRankedPlayer const& b) {
                return a.rank < b.rank;
            });

            playersLoaded = true;
            success();
        }
    );
}

void IntegratedPointercrate::loadPlayerRankByName(TaskHolder<web::WebResponse>& listener, std::string const& name, Function<void(PCRankedPlayer)> success, CopyableFunction<void(int)> failure) {
    auto url = fmt::format("{}/v1/players/ranking/?name_contains={}&limit=1", "https://pointercrate.com/api", urlEncode(name));
    listener.spawn(
        web::WebRequest().get(url),
        [success = std::move(success), failure = std::move(failure)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            auto jsonRes = res.json();
            if (!jsonRes.isOk()) return failure(-1);
            auto arr = jsonRes.unwrap().asArray();
            if (!arr.isOk() || arr.unwrap().empty()) return failure(-1);

            auto playerJson = arr.unwrap().front();
            PCRankedPlayer player;
            player.id = playerJson["id"].asInt().unwrapOr(0);
            player.rank = playerJson["rank"].asInt().unwrapOr(0);
            player.name = playerJson["name"].asString().unwrapOr("");
            player.score = playerJson["score"].asDouble().unwrapOr(0.0);
            if (playerJson["nationality"].isObject()) {
                player.countryCode = playerJson["nationality"]["country_code"].asString().unwrapOr("");
            }

            success(std::move(player));
        }
    );
}