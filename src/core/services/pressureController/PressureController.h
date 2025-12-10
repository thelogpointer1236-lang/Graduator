#ifndef GRADUATOR_PRESSURECONTROLLERLUAAPI_H
#define GRADUATOR_PRESSURECONTROLLERLUAAPI_H

#include <sol/sol.hpp>

class PressureController {
public:
    PressureController() {
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::ffi);
    }
};

#endif //GRADUATOR_PRESSURECONTROLLERLUAAPI_H