#include <Geode/utils/function.hpp>
#include <Geode/utils/web.hpp>

struct IDListDemon {
    int id = 0;
    int position = 0;
    std::string name;
    std::string verifier;

    bool operator==(const IDListDemon& other) const {
        return id == other.id && position == other.position;
    }
};

namespace IntegratedDemonlist {
    extern std::vector<IDListDemon> pointercrate;
    extern std::vector<IDListDemon> pemonlist;
    extern bool pointercrateLoaded;
    extern bool pemonlistLoaded;

    void loadPointercrate(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadPemonlist(geode::async::TaskHolder<geode::utils::web::WebResponse>&, geode::Function<void()>, geode::CopyableFunction<void(int)>);
}