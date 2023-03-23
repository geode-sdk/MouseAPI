#include "../include/API.hpp"
#include <Geode/utils/cocos.hpp>
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

MouseEvent::MouseEvent(CCPoint const& position)
  : m_position(position) {}

void MouseEvent::swallow() {
	m_swallow = true;
}

bool MouseEvent::isSwallowed() const {
	return m_swallow;
}

cocos2d::CCPoint MouseEvent::getPosition() const {
	return m_position;
}

MouseClickEvent::MouseClickEvent(MouseButton button, bool down, CCPoint const& position)
  : MouseEvent(position), m_button(button), m_down(down) {}

MouseButton MouseClickEvent::getButton() const {
	return m_button;
}

bool MouseClickEvent::isDown() const {
	return m_down;
}

MouseMoveEvent::MouseMoveEvent(CCPoint const& position)
  : MouseEvent(position) {}

MouseHoverEvent::MouseHoverEvent(bool enter, CCPoint const& position)
  : MouseEvent(position), m_enter(enter) {}

bool MouseHoverEvent::isEnter() const {
	return m_enter;
}

bool MouseHoverEvent::isLeave() const {
	return !m_enter;
}

MouseScrollEvent::MouseScrollEvent(float deltaY, float deltaX, CCPoint const& position)
  : MouseEvent(position), m_deltaY(deltaY), m_deltaX(deltaX) {}

float MouseScrollEvent::getDeltaY() const {
	return m_deltaY;
}

float MouseScrollEvent::getDeltaX() const {
	return m_deltaX;
}

ListenerResult MouseEventFilter::handle(geode::utils::MiniFunction<Callback> fn, MouseEvent* event) {
	if (Mouse::get()->getCapturing() && m_target != Mouse::get()->getCapturing()) {
		return ListenerResult::Propagate;
	}
	if (m_target) {
		if (m_target->boundingBox().containsPoint(event->getPosition())) {
			if (!m_hovered) {
				MouseHoverEvent(true, event->getPosition()).post();
				m_hovered = true;
			}
			auto s = fn(event);
			if (s == ListenerResult::Stop) {
				event->swallow();
			}
			return s;
		}
		else {
			if (m_hovered) {
				MouseHoverEvent(false, event->getPosition()).post();
				m_hovered = false;
			}
			return ListenerResult::Propagate;
		}
	}
	else {
		return fn(event);
	}
}

MouseEventFilter::MouseEventFilter(CCNode* target)
  : m_target(target) {}

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
		auto ev = MouseScrollEvent(y, x, getMousePos());
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

void __cdecl glfwPosCallback(GLFWwindow* window, double x, double y) {
	MouseMoveEvent(convertMouseCoords(x, y)).post();
}

void setWindowPosCallback(GLFWwindow* window) {
	reinterpret_cast<_GLFWwindow*>(window)->callbacks.cursorPos
	  = reinterpret_cast<GLFWcursorposfun>(&glfwPosCallback);
}

$on_mod(Loaded) {
	if (CCEGLView::get()->getWindow()) {
		setWindowPosCallback(CCEGLView::get()->getWindow());
	}
};

struct $modify(CCEGLView) {
	void setupWindow(CCRect size) {
		CCEGLView::setupWindow(size);
		setWindowPosCallback(m_pMainWindow);
	}

	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
		auto ev = MouseClickEvent(static_cast<MouseButton>(button), action, getMousePos());
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
