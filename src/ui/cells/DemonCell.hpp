#pragma once

#include <Geode/Geode.hpp>
#include "api/PointercrateAPI.hpp"

class DemonCell : public cocos2d::CCLayerColor, public LevelDownloadDelegate {
protected:
    pointercrate::Demon m_demon;
    LoadingCircle* m_loadingCircle = nullptr;
    cocos2d::CCMenuItemSpriteExtra* m_nameBtn = nullptr;

    bool init(pointercrate::Demon const& demon, float width);
    void onInfo(cocos2d::CCObject*);
    void onName(cocos2d::CCObject*);

    void levelDownloadFinished(GJGameLevel* level) override;
    void levelDownloadFailed(int code) override;

public:
    static DemonCell* create(pointercrate::Demon const& demon, float width);
};
