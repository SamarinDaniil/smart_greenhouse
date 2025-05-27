#include "db/SchemaLoader.hpp"
#include "db/Database.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace db
{

    void SchemaLoader::loadSchema(Database &db, const std::string &schemaFilePath)
    {

        std::ifstream file(schemaFilePath);
        if (!file.is_open())
        {
            throw std::runtime_error("SchemaLoader: cannot open file " + schemaFilePath);
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        if (file.fail() && !file.eof())
        {
            throw std::runtime_error("SchemaLoader: error reading file " + schemaFilePath);
        }
        std::string sql = ss.str();
        file.close();

        db.executeScript(sql);
    }

}