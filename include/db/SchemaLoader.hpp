#pragma once

#include <string>

class Database;

namespace db
{

    class SchemaLoader
    {
    public:
        static void loadSchema(Database &db, const std::string &schemaFilePath);
    };

}
