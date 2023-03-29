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
	if (m_target) {
		// Events will only be dispatched to nodes in the scene that are visible
		if (nodeIsVisible(m_target) && m_target->hasAncestor(nullptr)) {
			auto inside =
				m_ignorePosition ||
				m_target->boundingBox().containsPoint(event->getPosition());

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
				event->getTarget() == m_target ||
				// Is this node eating events?
				m_eaten.data() ||
				// Is this event inside the node and edible?
				(!event->getTarget() && inside)
			) {
				auto attrs = MouseAttributes::from(m_target);
				// Post hover event
				if (!attrs->m_hovered && inside) {
					attrs->m_hovered = true;
					MouseHoverEvent(
						m_target, true,
						event->getPosition()
					).post();
				}
				else if (attrs->m_hovered && !inside) {
					attrs->m_hovered = false;
					MouseHoverEvent(
						m_target, false,
						event->getPosition()
					).post();
				}
				// Add click to held list (may be something the callback needs 
				// to know, so needs to be set before it's called)
				auto click = typeinfo_cast<MouseClickEvent*>(event);
				if (click) {
					if (click->isDown()) {
						attrs->m_heldButtons.insert(click->getButton());
					}
					else {
						attrs->m_heldButtons.erase(click->getButton());
					}
				}
				auto s = fn(event);
				if (s == MouseResult::Leave) {
					// If the callback didn't want to capture the mouse, release 
					// the hold attribute immediately
					if (click) {
						attrs->m_heldButtons.erase(click->getButton());
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
					event->dispatchTouch(m_target, m_eaten);
				}
				// Release eaten only after dispatching the touch event so the 
				// touch event can still access the touch
				if (s != MouseResult::Leave && click && !click->isDown()) {
					m_eaten = nullptr;	
				}
				// If the callback wants to swallow, or has captured the mouse, 
				// capture the mouse and stop propagation here
				if (s == MouseResult::Swallow || event->getTarget() == m_target) {
					event->swallow();
					if (click) {
						if (click->isDown()) {
							Mouse::capture(m_target);
						}
						else {
							Mouse::release(m_target);
						}
					}
					return ListenerResult::Stop;
				}
				return ListenerResult::Propagate;
			}
			// If this target doesn't get the event, propagate onwards
			else {
				auto attrs = MouseAttributes::from(m_target);
				// Post hover leave event if necessary
				if (attrs->m_hovered && !inside) {
					attrs->m_hovered = false;
					attrs->m_heldButtons.clear();
					m_eaten = nullptr;
					MouseHoverEvent(
						m_target, false,
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
		log::info("sorting");
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
		for (auto& a : Event::listeners()) {
			if (auto af = typeinfo_cast<EventListener<MouseEventFilter>*>(a)) {
				log::info("{}: {}", af->getFilter().getTargetPriority(), af->getFilter().getTarget());
			}
		}
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
		Mouse::get()->m_swallowing = target;
	}
}

void Mouse::release(CCNode* target) {
	if (Mouse::get()->m_swallowing == target) {
		Mouse::get()->m_swallowing = nullptr;
	}
}

#ifdef GEODE_IS_DESKTOP
#include <Geode/modify/CCMouseDispatcher.hpp>

struct $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		auto ev = MouseScrollEvent(
			Mouse::get()->getCapturing(), y, x,
			getMousePos()
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
	MouseMoveEvent(Mouse::get()->getCapturing(), convertMouseCoords(x, y)).post();
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
			getMousePos()
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
