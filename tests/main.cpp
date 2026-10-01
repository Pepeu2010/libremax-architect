#if __has_include(<catch2/catch_session.hpp>)
#include <catch2/catch_session.hpp>
#else
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#endif
#include <QApplication>

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    return Catch::Session().run(argc, argv);
}
