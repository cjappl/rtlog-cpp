#include <doctest/doctest.h>
#include <rtlog/rtlog.h>

TEST_CASE("Dummy test")
{
    CHECK(true);
}

TEST_CASE("Test rtlog")
{
    rtlog::Logger logger;
    logger.Log("Hello, world!");
}
