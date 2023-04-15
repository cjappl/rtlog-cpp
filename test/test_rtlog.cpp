#include <doctest/doctest.h>
#include <rtlog/rtlog.h>

using namespace rtlog;

TEST_CASE("Dummy test")
{
    CHECK(true);
}

TEST_CASE("Test rtlog")
{
    rtlog::Logger logger;
    logger.Log(ExampleLogLevel::Debug, ExampleLogRegion::Engine, "Hello, world!");
    logger.Log(ExampleLogLevel::Info, ExampleLogRegion::Game, "Hello, world!");
    logger.Log(ExampleLogLevel::Warning, ExampleLogRegion::Network, "Hello, world!");
    logger.Log(ExampleLogLevel::Error, ExampleLogRegion::Audio, "Hello, world!");

    logger.ProcessLog();
    logger.ProcessLog();
    logger.ProcessLog();
    logger.ProcessLog();
}
