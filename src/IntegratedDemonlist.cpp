#include "IntegratedDemonlist.hpp"
#include <algorithm>
#include <jasmine/web.hpp>

using namespace geode::prelude;

std::vector<IDListDemon> IntegratedDemonlist::pointercrate;
std::vector<IDListDemon> IntegratedDemonlist::pemonlist;
bool IntegratedDemonlist::pointercrateLoaded = false;
bool IntegratedDemonlist::pemonlistLoaded = false;

void IntegratedDemonlist::loadPointercrate(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    auto loadPage = std::make_shared<std::function<void(int)>>();
    *loadPage = [&listener, success = std::move(success), failure = std::move(failure), loadPage](int after) {
        auto url = after > 0
            ? fmt::format("https://pointercrate.com/api/v2/demons/listed/?limit=100&after={}", after)
            : "https://pointercrate.com/api/v2/demons/listed/?limit=100";
        listener.spawn(
            web::WebRequest().get(url),
            [success, failure, loadPage](web::WebResponse res) mutable {
                if (!res.ok()) return failure(res.code());

                pointercrateLoaded = true;
                auto demons = jasmine::web::getArray(res);
                for (auto& demon : demons) {
                    auto id = demon.get<int>("level_id");
                    if (!id.isOk()) continue;

                    auto position = demon.get<int>("position");
                    if (!position.isOk()) continue;

                    auto name = demon.get<std::string>("name");
                    if (!name.isOk()) continue;

                    std::string verifier;
                    auto verifierRes = demon.get("verifier");
                    if (verifierRes.isOk()) {
                        if (auto verifierName = verifierRes.unwrap().get<std::string>("name"); verifierName.isOk()) {
                            verifier = std::move(verifierName).unwrap();
                        }
                    }

                    IDListDemon listDemon(id.unwrap(), position.unwrap(), std::move(name).unwrap(), verifier);
                    if (!std::ranges::contains(pointercrate, listDemon)) {
                        pointercrate.insert(
                            std::ranges::upper_bound(pointercrate, listDemon, [](const IDListDemon& a, const IDListDemon& b) {
                                return a.position < b.position;
                            }),
                            std::move(listDemon)
                        );
                    }
                }

                if (demons.size() < 100) return success();

                auto lastPosition = demons[demons.size() - 1].get<int>("position");
                if (!lastPosition.isOk()) return success();
                (*loadPage)(lastPosition.unwrap());
            }
        );
    };
    (*loadPage)(0);
}

void IntegratedDemonlist::loadPemonlist(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    listener.spawn(
        web::WebRequest().get("https://pemonlist.com/api/list?limit=150&version=2"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            pemonlistLoaded = true;
            pemonlist.clear();

            for (auto& level : jasmine::web::getArray(res, "data")) {
                auto id = level.get<int>("level_id");
                if (!id.isOk()) continue;

                auto position = level.get<int>("placement");
                if (!position.isOk()) continue;

                auto name = level.get<std::string>("name");
                if (!name.isOk()) continue;

                IDListDemon demon(id.unwrap(), position.unwrap(), std::move(name).unwrap());

                pemonlist.insert(std::ranges::upper_bound(pemonlist, demon, [](const IDListDemon& a, const IDListDemon& b) {
                    return a.position < b.position;
                }), std::move(demon));
            }

            success();
        }
    );
}