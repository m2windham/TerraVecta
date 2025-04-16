#include "catch.hpp"
#include "../Application.h"

TEST_CASE("Application initializes correctly", "[Application]") {
    Application app(800, 600, "Test Window");

    REQUIRE_NOTHROW(app.run());
}