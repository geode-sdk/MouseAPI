#include "../include/API.hpp"
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/ranges.hpp>
#include <Geode/modify/CCNode.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>
#include <Geode/cocos/robtop/glfw/glfw3.h>
#include <json/stl_serialize.hpp>

using namespace geode::prelude;
using namespace mouse;

struct _GLFWwindow
{
    struct _GLFWwindow* next;									// 0

    // Window settings and state
    GLboolean           iconified;								// 4
    GLboolean           resizable;
    GLboolean           decorated;
    GLboolean           visible;
    GLboolean           closed;									// 8
    void*               userPointer;							// c
    GLFWvidmode         videoMode;								// 10
    GLFWmonitor*        monitor;								// 28

    // Window input state
    GLboolean           stickyKeys;								// 2c
    GLboolean           stickyMouseButtons;
    double              cursorPosX, cursorPosY;					// 30
    int                 cursorMode;								// 40
    char                mouseButton[GLFW_MOUSE_BUTTON_LAST + 1];// 44
    char                key[GLFW_KEY_LAST + 1];					// 4c

    // OpenGL extensions and context attributes
    int                 clientAPI;								// 1ac
    int                 glMajor, glMinor, glRevision;			// 1b0
    GLboolean           glForward, glDebug;						// 1b4
    int                 glProfile;								// 1b8
    int                 glRobustness;							// 1bc
    PFNGLGETSTRINGIPROC GetStringi;								// 1c0

	PAD(0x20);

    struct {
        GLFWwindowposfun        pos;		// 1ec
        GLFWwindowsizefun       size;		// 1f0
        GLFWwindowclosefun      close;		// 1f4
        GLFWwindowrefreshfun    refresh;	// 1f8
        GLFWwindowfocusfun      focus;		// 1fc
        GLFWwindowiconifyfun    iconify;	// 200
        GLFWframebuffersizefun  fbsize;		// 204
        GLFWmousebuttonfun      mouseButton;// 208
        GLFWcursorposfun        cursorPos;	// 20c
        GLFWcursorenterfun      cursorEnter;// 210
        GLFWscrollfun           scroll;		// 214
        GLFWkeyfun              key;		// 218
        GLFWcharfun             character;	// 21c
    } callbacks;

	// there's other stuff i'm too lazy to include
};

CCPoint convertMouseCoords(double x, double y) {
    auto* director = CCDirector::get();
    auto* gl = director->getOpenGLView();
    auto winSize = director->getWinSize();
    auto frameSize = gl->getFrameSize();
    auto mouse = CCPoint { static_cast<float>(x), static_cast<float>(y) } / frameSize;
    return ccp(mouse.x, 1.f - mouse.y) * winSize;
}

json::Value json::Serialize<MouseButton>::to_json(MouseButton const& button) {
	return static_cast<int>(button);
}

MouseButton json::Serialize<MouseButton>::from_json(json::Value const& json) {
	return static_cast<MouseButton>(json.as_int());
}

using List = std::unordered_set<MouseButton>;

MouseAttributes* MouseAttributes::from(CCNode* node) {
	auto attrs = new MouseAttributes();
	attrs->m_node = node;
	attrs->autorelease();
	return attrs;
}

bool MouseAttributes::isHeld(MouseButton button) const {
	if (auto list = m_node->template getAttribute<List>("held"_spr)) {
		return list.value().contains(button);
	}
	return false;
}

bool MouseAttributes::isHovered() const {
	return m_node->template getAttribute<bool>("hovered"_spr).value_or(false);
}

void MouseAttributes::addHeld(MouseButton button) {
	auto list = m_node->template getAttribute<List>("held"_spr).value_or(List {});
	list.insert(button);
	m_node->setAttribute("held"_spr, list);
}

void MouseAttributes::removeHeld(MouseButton button) {
	auto list = m_node->template getAttribute<List>("held"_spr).value_or(List {});
	list.erase(button);
	m_node->setAttribute("held"_spr, list);
}

void MouseAttributes::clearHeld() {
	m_node->setAttribute("held"_spr, List {});
}

void MouseAttributes::setHovered(bool hovered) {
	m_node->setAttribute("hovered"_spr, hovered);
}

MouseEvent::MouseEvent(CCNode* target, CCPoint const& position)
  : m_target(target),
	m_position(position) {}

void MouseEvent::swallow() {
	m_swallow = true;
}

bool MouseEvent::isSwallowed() const {
	return m_swallow;
}

CCPoint MouseEvent::getPosition() const {
	return m_position;
}

CCNode* MouseEvent::getTarget() const {
	return m_target;
}

CCTouch* MouseEvent::createTouch() const {
	auto touch = new CCTouch();
	touch->autorelease();
	touch->m_point = CCDirector::get()->convertToUI(m_position);
	touch->m_prevPoint = (touch->m_startPoint = touch->m_point);
	return touch;
}

CCEvent* MouseEvent::createEvent() const {
	auto event = new CCEvent();
	event->autorelease();
	return event;
}

void MouseEvent::updateTouch(CCTouch* touch) const {
	touch->m_prevPoint = touch->m_point;
	touch->m_point = CCDirector::get()->convertToUI(m_position);
}

MouseClickEvent::MouseClickEvent(
	CCNode* target, MouseButton button,
	bool down, CCPoint const& pos
) : MouseEvent(target, pos), m_button(button), m_down(down) {}

MouseClickEvent::MouseClickEvent(
	MouseButton button, bool down, CCPoint const& pos
) : MouseClickEvent(nullptr, button, down, pos) {}

void MouseClickEvent::dispatchTouch(CCNode* target, CCTouch* touch) const {
	if (m_button == MouseButton::Left) {
		if (auto delegate = typeinfo_cast<CCTouchDelegate*>(target)) {
			if (m_down) {
				delegate->ccTouchBegan(touch, this->createEvent());
			}
			else {
				delegate->ccTouchEnded(touch, this->createEvent());
			}
		}
	}
}

MouseButton MouseClickEvent::getButton() const {
	return m_button;
}

bool MouseClickEvent::isDown() const {
	return m_down;
}

MouseMoveEvent::MouseMoveEvent(CCPoint const& pos)
  : MouseMoveEvent(nullptr, pos) {}

MouseMoveEvent::MouseMoveEvent(CCNode* target, CCPoint const& pos)
  : MouseEvent(target, pos) {}

void MouseMoveEvent::dispatchTouch(CCNode* target, CCTouch* touch) const {
	if (auto delegate = typeinfo_cast<CCTouchDelegate*>(target)) {
		if (MouseAttributes::from(target)->isHeld(MouseButton::Left)) {
			delegate->ccTouchMoved(touch, this->createEvent());
		}
	}
}

MouseScrollEvent::MouseScrollEvent(float deltaY, float deltaX, CCPoint const& pos)
  : MouseScrollEvent(nullptr, deltaY, deltaX, pos) {}

MouseScrollEvent::MouseScrollEvent(
	CCNode* target, float deltaY, float deltaX, CCPoint const& pos
) : MouseEvent(target, pos), m_deltaY(deltaY), m_deltaX(deltaX) {}

void MouseScrollEvent::dispatchTouch(CCNode* target, CCTouch*) const {
	if (auto delegate = typeinfo_cast<CCMouseDelegate*>(target)) {
		delegate->scrollWheel(m_deltaY, m_deltaY);
	}
}

float MouseScrollEvent::getDeltaY() const {
	return m_deltaY;
}

float MouseScrollEvent::getDeltaX() const {
	return m_deltaX;
}

MouseHoverEvent::MouseHoverEvent(CCNode* target, bool enter, CCPoint const& pos)
  : MouseEvent(target, pos), m_enter(enter) {}

void MouseHoverEvent::dispatchTouch(CCNode*, CCTouch*) const {}

bool MouseHoverEvent::isEnter() const {
	return m_enter;
}

bool MouseHoverEvent::isLeave() const {
	return !m_enter;
}

ListenerResult MouseEventFilter::handle(geode::utils::MiniFunction<Callback> fn, MouseEvent* event) {
	if (m_target.has_value()) {
		if (auto target = m_target.value().lock()) {
			// Events will only be dispatched to nodes in the scene that are visible
			if (nodeIsVisible(target) && target->hasAncestor(nullptr)) {
				auto inside =
					m_ignorePosition ||
					target->boundingBox().containsPoint(
						target->getParent()->convertToNodeSpace(event->getPosition())
					);

				// Events will only be dispatched to the target under the 
				// following conditions: 
				// 1. The target is capturing the mouse
				// 2. When the event is dispatched, if no other target has yet 
				// captured the mouse, another target can "eat" it, in other words 
				// registering themselves as a "weak" target that will still 
				// receive events
				// 3. Nothing is capturing the mouse (event is "edible")
				if (
					// Is this the node the event was meant for?
					event->getTarget() == target ||
					// Is this node eating events?
					m_eaten.data() ||
					// Is this event inside the node and edible?
					(!event->getTarget() && inside)
				) {
					auto attrs = MouseAttributes::from(target);
					// Post hover event
					if (!attrs->isHovered() && inside) {
						attrs->setHovered(true);
						MouseHoverEvent(
							target, true,
							event->getPosition()
						).post();
					}
					else if (attrs->isHovered() && !inside) {
						attrs->setHovered(false);
						MouseHoverEvent(
							target, false,
							event->getPosition()
						).post();
					}
					// Add click to held list (may be something the callback needs 
					// to know, so needs to be set before it's called)
					auto click = typeinfo_cast<MouseClickEvent*>(event);
					if (click) {
						if (click->isDown()) {
							attrs->addHeld(click->getButton());
						}
						else {
							attrs->removeHeld(click->getButton());
						}
					}
					auto s = fn(event);
					if (s == MouseResult::Leave) {
						// If the callback didn't want to capture the mouse, release 
						// the hold attribute immediately
						if (click) {
							attrs->removeHeld(click->getButton());
						}
					}
					else {
						// Eat if clicked
						if (click && click->isDown()) {
							m_eaten = event->createTouch();
						}
					}
					if (m_eaten) {
						event->updateTouch(m_eaten);
						event->dispatchTouch(target, m_eaten);
					}
					// Release eaten only after dispatching the touch event so the 
					// touch event can still access the touch
					if (s != MouseResult::Leave && click && !click->isDown()) {
						m_eaten = nullptr;	
					}
					// If the callback wants to swallow, or has captured the mouse, 
					// capture the mouse and stop propagation here
					if (s == MouseResult::Swallow || event->getTarget() == target) {
						event->swallow();
						if (click) {
							if (click->isDown()) {
								Mouse::capture(target);
							}
							else {
								Mouse::release(target);
							}
						}
						return ListenerResult::Stop;
					}
					return ListenerResult::Propagate;
				}
				// If this target doesn't get the event, propagate onwards
				else {
					auto attrs = MouseAttributes::from(target);
					// Post hover leave event if necessary
					if (attrs->isHovered() && !inside) {
						attrs->setHovered(false);
						attrs->clearHeld();
						m_eaten = nullptr;
						MouseHoverEvent(
							target, false,
							event->getPosition()
						).post();
					}
					return ListenerResult::Propagate;
				}
			}
			else {
				return ListenerResult::Propagate;
			}
		}
		else {
			return ListenerResult::Propagate;
		}
	}
	// Otherwise handle global listeners, which will only be fired if no node 
	// is capturing the mouse
	else if (!event->getTarget()) {
		auto s = fn(event);
		if (s == MouseResult::Swallow) {
			event->swallow();
			return ListenerResult::Stop;
		}
		return ListenerResult::Propagate;
	}
	// Otherwise keep going and find the capturing target
	else {
		return ListenerResult::Propagate;
	}
}

std::optional<geode::WeakRef<cocos2d::CCNode>> MouseEventFilter::getTarget() const {
	return m_target;
}

std::vector<int> MouseEventFilter::getTargetPriority() const {
	if (!m_target.has_value()) return {};
	auto node = m_target.value().lock();
	if (!node) return {};
	std::vector<int> tree {};
	while (auto parent = node->getParent()) {
		tree.insert(tree.begin(), parent->getChildren()->indexOfObject(node));
		node = parent;
	}
	return tree;
}

MouseEventFilter::MouseEventFilter(CCNode* target, bool ignorePosition)
  : m_target(target), m_ignorePosition(ignorePosition)
{}

MouseEventFilter::~MouseEventFilter() {}

void Mouse::updateListeners() {
	if (s_updating) return;
	s_updating = true;
	// update only once per frame at most
	Loader::get()->queueInGDThread([]() {
		// log::info("sorting");
		std::sort(
			Event::listeners().begin(),
			Event::listeners().end(),
			[](EventListenerProtocol* a, EventListenerProtocol* b) {
				auto af = typeinfo_cast<EventListener<MouseEventFilter>*>(a);
				auto bf = typeinfo_cast<EventListener<MouseEventFilter>*>(b);
				if (af && bf) {
					auto ap = af->getFilter().getTargetPriority(); 
					auto bp = bf->getFilter().getTargetPriority();
					for (size_t i = 0; i < ap.size(); i++) {
						if (i < bp.size()) {
							if (ap[i] != bp[i]) {
								return ap[i] > bp[i];
							}
						}
					}
					return ap.size() > bp.size();
				}
				// make sure mouse listeners are at the start of the list
				if (af) return true;
				if (bf) return false;
				// keep everything else in the same order
				return true;
			}
		);
		// for (auto& a : Event::listeners()) {
		// 	if (auto af = typeinfo_cast<EventListener<MouseEventFilter>*>(a)) {
		// 		log::info("{}: {}", af->getFilter().getTargetPriority(), af->getFilter().getTarget());
		// 	}
		// }
		s_updating = false;
	});
}

Mouse* Mouse::get() {
	static auto inst = new Mouse;
	return inst;
}

bool Mouse::isHeld(MouseButton button) const {
	return m_heldButtons.contains(button);
}

CCNode* Mouse::getCapturing() const {
	return m_swallowing;
}

void Mouse::capture(CCNode* target) {
	if (!Mouse::get()->m_swallowing) {
		Mouse::get()->m_swallowing = target;
	}
}

void Mouse::release(CCNode* target) {
	if (Mouse::get()->m_swallowing == target) {
		Mouse::get()->m_swallowing = nullptr;
	}
}

struct MouseEventContainer : public CCEvent {
	MouseEvent& event;
	MouseEventContainer(MouseEvent& event) : event(event) {
		this->autorelease();
	}
};

static void postMouseEventThroughTouches(MouseEvent& event, ccTouchType action) {
	auto set = CCSet::create();
	set->addObject(event.createTouch());
	CCTouchDispatcher::get()->touches(set, new MouseEventContainer(event), action);
}

#ifdef GEODE_IS_DESKTOP
#include <Geode/modify/CCMouseDispatcher.hpp>

struct $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		auto ev = MouseScrollEvent(
			Mouse::get()->getCapturing(), y, x,
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

#ifdef GEODE_IS_WINDOWS
#include <Geode/modify/CCEGLView.hpp>

static GLFWcursorposfun originalCursorPosFun = nullptr;

void __cdecl glfwPosCallback(GLFWwindow* window, double x, double y) {
	originalCursorPosFun(window, x, y);
	Loader::get()->queueInGDThread([=]() {
		auto event = MouseMoveEvent(Mouse::get()->getCapturing(), convertMouseCoords(x, y));
		postMouseEventThroughTouches(
			event,
			(Mouse::get()->isHeld(MouseButton::Left) ?
				CCTOUCHMOVED :
				CCTOUCHOTHER)
		);
	});
}

void setCursorPosCallback(GLFWwindow* window) {
	if (!originalCursorPosFun) {
		originalCursorPosFun = reinterpret_cast<_GLFWwindow*>(window)->callbacks.cursorPos;
		reinterpret_cast<_GLFWwindow*>(window)->callbacks.cursorPos
		= reinterpret_cast<GLFWcursorposfun>(&glfwPosCallback);
	}
}

$on_mod(Loaded) {
	if (CCEGLView::get()->getWindow()) {
		setCursorPosCallback(CCEGLView::get()->getWindow());
	}
};

struct $modify(CCEGLViewModify, CCEGLView) {
	void setupWindow(CCRect size) {
		CCEGLView::setupWindow(size);
		setCursorPosCallback(m_pMainWindow);
	}

	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
		Loader::get()->queueInGDThread([=]() {
			if (action) {
				Mouse::get()->m_heldButtons.insert(static_cast<MouseButton>(button));
			}
			else {
				Mouse::get()->m_heldButtons.erase(static_cast<MouseButton>(button));
			}
			auto event = MouseClickEvent(
				Mouse::get()->getCapturing(),
				static_cast<MouseButton>(button), action,
				getMousePos()
			);
			postMouseEventThroughTouches(
				event,
				(static_cast<MouseButton>(button) == MouseButton::Left ?
					(action ? CCTOUCHBEGAN : CCTOUCHENDED) :
					CCTOUCHOTHER)
			);
		});
	}
};

#else
#error "Not implemented on this platform"
#endif

struct $modify(CCTouchDispatcherModify, CCTouchDispatcher) {
	static void onModify(auto& self) {
		(void)self.setHookPriority("CCTouchDispatcher::touches", 1000);
	}

	void touches(CCSet* set, CCEvent* event, unsigned int type) {
		if (auto me = typeinfo_cast<MouseEventContainer*>(event)) {
			// Update event position in case some touches hook changed it
			me->event.m_position = static_cast<CCTouch*>(set->anyObject())->getLocation();
			me->event.post();
		}
		else {
			CCTouchDispatcher::touches(set, event, type);
		}
	}
};
