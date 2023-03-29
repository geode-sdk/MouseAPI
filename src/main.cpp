#include "../include/API.hpp"
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/ranges.hpp>
#include <Geode/modify/CCNode.hpp>
#include <Geode/cocos/robtop/glfw/glfw3.h>

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

struct $modify(MouseNode, CCNode) {
	MouseAttributes attributes;
};

MouseAttributes* MouseAttributes::from(CCNode* node) {
	return &static_cast<MouseNode*>(node)->m_fields->attributes;
}

bool MouseAttributes::isHeld(MouseButton button) const {
	return m_heldButtons.contains(button);
}

bool MouseAttributes::isHovered() const {
	return m_hovered;
}

MouseEvent::MouseEvent(
	CCNode* target,
	CCPoint const& position,
	CCPoint const& prevPosition
) : m_target(target),
	m_position(position),
	m_prevPosition(prevPosition) {}

void MouseEvent::swallow() {
	m_swallow = true;
}

bool MouseEvent::isSwallowed() const {
	return m_swallow;
}

CCPoint MouseEvent::getPosition() const {
	return m_position;
}

CCPoint MouseEvent::getPrevPosition() const {
	return m_prevPosition;
}

CCNode* MouseEvent::getTarget() const {
	return m_target;
}

CCTouch* MouseEvent::createTouch() const {
	auto touch = new CCTouch();
	touch->autorelease();
	touch->m_point = CCDirector::get()->convertToUI(m_position);
	touch->m_prevPoint = CCDirector::get()->convertToUI(m_prevPosition);
	return touch;
}

CCEvent* MouseEvent::createEvent() const {
	auto event = new CCEvent();
	event->autorelease();
	return event;
}

MouseClickEvent::MouseClickEvent(
	CCNode* target, MouseButton button,
	bool down, CCPoint const& pos, CCPoint const& prev
) : MouseEvent(target, pos, prev), m_button(button), m_down(down) {}

MouseClickEvent::MouseClickEvent(
	MouseButton button, bool down,
	CCPoint const& pos, CCPoint const& prev
) : MouseClickEvent(nullptr, button, down, pos, prev) {}

void MouseClickEvent::dispatchTouch(CCNode* target) const {
	if (m_button == MouseButton::Left) {
		if (auto delegate = typeinfo_cast<CCTouchDelegate*>(target)) {
			if (m_down) {
				delegate->ccTouchBegan(this->createTouch(), this->createEvent());
			}
			else {
				delegate->ccTouchEnded(this->createTouch(), this->createEvent());
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

MouseMoveEvent::MouseMoveEvent(CCPoint const& pos, CCPoint const& prev)
  : MouseMoveEvent(nullptr, pos, prev) {}

MouseMoveEvent::MouseMoveEvent(
	CCNode* target, 
	CCPoint const& pos, CCPoint const& prev
) : MouseEvent(target, pos, prev) {}

void MouseMoveEvent::dispatchTouch(CCNode* target) const {
	if (auto delegate = typeinfo_cast<CCTouchDelegate*>(target)) {
		if (MouseAttributes::from(target)->isHeld(MouseButton::Left)) {
			delegate->ccTouchMoved(this->createTouch(), this->createEvent());
		}
	}
}

MouseScrollEvent::MouseScrollEvent(
	float deltaY, float deltaX,
	CCPoint const& pos, CCPoint const& prev
) : MouseScrollEvent(nullptr, deltaY, deltaX, pos, prev) {}

MouseScrollEvent::MouseScrollEvent(
	CCNode* target, float deltaY, float deltaX,
	CCPoint const& pos, CCPoint const& prev
) : MouseEvent(target, pos, prev), m_deltaY(deltaY), m_deltaX(deltaX) {}

void MouseScrollEvent::dispatchTouch(CCNode* target) const {
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

MouseHoverEvent::MouseHoverEvent(
	CCNode* target, bool enter, 
	CCPoint const& pos, CCPoint const& prev
) : MouseEvent(target, pos, prev), m_enter(enter) {}

void MouseHoverEvent::dispatchTouch(CCNode* target) const {}

bool MouseHoverEvent::isEnter() const {
	return m_enter;
}

bool MouseHoverEvent::isLeave() const {
	return !m_enter;
}

ListenerResult MouseEventFilter::handle(geode::utils::MiniFunction<Callback> fn, MouseEvent* event) {
	if (m_target) {
		if (nodeIsVisible(m_target) && m_target->hasAncestor(nullptr)) {
			auto inside = m_ignorePosition || m_target->boundingBox().containsPoint(event->getPosition());
			if (event->getTarget() == m_target || (!event->getTarget() && inside)) {
				if (!m_hovered && inside) {
					m_hovered = true;
					MouseAttributes::from(m_target)->m_hovered = true;
					MouseHoverEvent(
						m_target, true,
						event->getPosition(),
						event->getPrevPosition()
					).post();
				}
				auto click = typeinfo_cast<MouseClickEvent*>(event);
				if (click) {
					if (click->isDown()) {
						MouseAttributes::from(m_target)->m_heldButtons.insert(click->getButton());
					}
					else {
						MouseAttributes::from(m_target)->m_heldButtons.erase(click->getButton());
					}
				}
				auto s = fn(event);
				event->dispatchTouch(m_target);
				if (s == ListenerResult::Stop || event->getTarget() == m_target) {
					event->swallow();
					if (click) {
						if (click->isDown()) {
							Mouse::capture(m_target);
						}
						else {
							Mouse::release(m_target);
						}
					}
				}
				return s;
			}
			else {
				if (m_hovered && !inside) {
					m_hovered = false;
					MouseAttributes::from(m_target)->m_hovered = false;
					MouseAttributes::from(m_target)->m_heldButtons.clear();
					MouseHoverEvent(
						m_target, false,
						event->getPosition(),
						event->getPrevPosition()
					).post();
				}
				return ListenerResult::Propagate;
			}
		}
		else {
			return ListenerResult::Propagate;
		}
	}
	else if (!event->getTarget()) {
		auto s = fn(event);
		if (s == ListenerResult::Stop) {
			event->swallow();
		}
		return s;
	}
	else {
		return ListenerResult::Propagate;
	}
}

CCNode* MouseEventFilter::getTarget() const {
	return m_target;
}

std::vector<int> MouseEventFilter::getTargetPriority() const {
	auto node = m_target;
	std::vector<int> tree {};
	while (auto parent = node->getParent()) {
		tree.insert(tree.begin(), parent->getChildren()->indexOfObject(node));
		node = parent;
	}
	return tree;
}

MouseEventFilter::MouseEventFilter(CCNode* target, bool ignorePosition)
  : m_target(target), m_ignorePosition(ignorePosition) {}

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

CCNode* Mouse::getCapturing() const {
	return m_swallowing;
}

void Mouse::capture(CCNode* target) {
	if (!Mouse::get()->m_swallowing) {
		for (auto& a : Event::listeners()) {
			if (auto af = typeinfo_cast<EventListener<MouseEventFilter>*>(a)) {
				auto t = af->getFilter().getTarget();
				if (t != target) {
					MouseAttributes::from(t)->m_heldButtons.clear();
				}
			}
		}
		Mouse::get()->m_swallowing = target;
	}
}

void Mouse::release(CCNode* target) {
	if (Mouse::get()->m_swallowing == target) {
		Mouse::get()->m_swallowing = nullptr;
	}
}

static CCPoint PREV_POS { 0, 0 };

#ifdef GEODE_IS_DESKTOP
#include <Geode/modify/CCMouseDispatcher.hpp>

struct $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		auto ev = MouseScrollEvent(
			Mouse::get()->getCapturing(), y, x,
			getMousePos(), PREV_POS
		);
		ev.post();
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
	MouseMoveEvent(Mouse::get()->getCapturing(), convertMouseCoords(x, y), PREV_POS).post();
	PREV_POS = convertMouseCoords(x, y);
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

struct $modify(CCEGLView) {
	void setupWindow(CCRect size) {
		CCEGLView::setupWindow(size);
		setCursorPosCallback(m_pMainWindow);
	}

	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
		// log::info("clicked {}", button);
		auto ev = MouseClickEvent(
			Mouse::get()->getCapturing(),
			static_cast<MouseButton>(button), action,
			getMousePos(), PREV_POS
		);
		ev.post();
		if (ev.isSwallowed()) {
			return;
		}
		CCEGLView::onGLFWMouseCallBack(window, button, action, mods);
	}
};

#else
#error "Not implemented on this platform"
#endif
