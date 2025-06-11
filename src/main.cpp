#include <iostream>
#include <utils/Logger.hpp>
#include <config/ConfigLoader.hpp>
#include <db/Database.hpp>
#include <stdexcept>
#include <iomanip>



int main(int argc, char *argv[])
{
    std::cout << "Smart Greenhouse server: build successful!\n";

    INIT_LOGGER_DEFAULT();
    try
    {
        // 1. Загрузка конфигурации
        const std::string configDir = (argc > 1) ? argv[1] : "./config";
        Config config = ConfigLoader::load(configDir);

        LOG_INFO("Starting Greenhouse Control System...");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Fatal error: {}", e.what());
        return 1;
    }

    std::cout << "Starting database tests..." << std::endl;
    Database db("data/gh_test.db");
    if (db.test_database_operations(db)) {
        std::cout << "All tests passed successfully!" << std::endl;
        return 0;
    } else {
        std::cerr << "Some tests failed!" << std::endl;
        return 1;
    }

    return 0;
}
