#pragma once

#include <Geode/utils/function.hpp>
#include <Geode/utils/web.hpp>
#include <string>
#include <vector>

struct PCDemon {
    int id = 0;
    int position = 0;
    std::string name;

    bool operator==(const PCDemon& other) const {
        return id == other.id && position == other.position;
    }
};

struct PCRankedPlayer {
    int id = 0;
    int rank = 0;
    std::string name;
    double score = 0.0;
    std::string countryCode;
};

namespace IntegratedPointercrate {
    extern std::vector<PCDemon> demons;
    extern std::vector<PCRankedPlayer> players;
    extern bool demonsLoaded;
    extern bool playersLoaded;

    void loadDemons(geode::async::TaskHolder<geode::utils::web::WebResponse>& listener, geode::Function<void()> success, geode::CopyableFunction<void(int)> failure);
    void loadPlayers(geode::async::TaskHolder<geode::utils::web::WebResponse>& listener, geode::Function<void()> success, geode::CopyableFunction<void(int)> failure);
    void loadPlayerRankByName(geode::async::TaskHolder<geode::utils::web::WebResponse>& listener, std::string const& name, geode::Function<void(PCRankedPlayer)> success, geode::CopyableFunction<void(int)> failure);
}