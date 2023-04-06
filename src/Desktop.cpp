#include <Geode/DefaultInclude.hpp>

#ifdef GEODE_IS_DESKTOP
#include "Platform.hpp"
#include "../include/API.hpp"
#include <Geode/modify/CCMouseDispatcher.hpp>

using namespace geode::prelude;
using namespace mouse;

struct $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		auto ev = MouseScrollEvent(
			Mouse::get()->getCapturingNode(), y, x,
			getMousePos()
		);
		postMouseEventThroughTouches(ev, CCTOUCHOTHER);
		if (ev.isSwallowed()) {
			return true;
		}
		return CCMouseDispatcher::dispatchScrollMSG(y, x);
	}
};

#endif
