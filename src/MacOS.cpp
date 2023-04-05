#include <Geode/DefaultInclude.hpp>

#ifdef GEODE_IS_MACOS
#error "Not implemented on this platform"

// all you gotta do is 
// 1. impl move event (just find whatever gets mouse move events on mac and 
// hook that then post MouseMoveEvent via postMouseEventThroughTouches)
// 2. impl click event (similar shit)
// see Windows.cpp for how it's on windows
#endif
