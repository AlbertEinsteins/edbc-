#include "import.hpp"
#include "common.hpp"
#include "pmdOptions.hpp"
#include "pd.hpp"
#include <argparse/argparse.hpp>

namespace edb {


pmdOptions::pmdOptions()
{
    maxPool_ = DEFAULT_POOLSIZE;
}

pmdOptions::~pmdOptions()
{

}

int pmdOptions::readFromCmd(int argc, char *argv[])
{
    int rc = EDB_OK;
    argparse::ArgumentParser parser("The DB Param Needed!");

    setUp(parser);
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        ::exit(-1);
    }

    if (parser.get<bool>("help")) {
        std::cout << parser.help().str();
        return rc;
    }

    logPath_ = parser.get<std::string>("logpath");
    dbPath_ = parser.get<std::string>("dbpath");
    confPath_ = parser.get<std::string>("confpath");
    svcName_ = parser.get<std::string>("svcname");
    maxPool_ = parser.get<int>("poolsize");
    return rc;
}


int pmdOptions::readConfigFromFile()
{
    return EDB_OK;
}


void pmdOptions::setUp(argparse::ArgumentParser& parser)
{
    parser.add_argument("--dbpath")
        .help( "The db data file, where the data saved")
        .default_value(DEFAULT_DB_FILENAME);
    parser.add_argument("--logpath")
        .help("The log path, where the log saved")
        .default_value(DEFAULT_LOG_FILENAME);
    parser.add_argument("--confpath")
        .help("The conf path, where the config saved")
        .default_value(DEFAULT_CONFIG_FILENAME);
    parser.add_argument("--svcname")
        .help("The service name")
        .default_value(DEFAULT_SVCNAME);
    parser.add_argument("--poolsize")
        .help("The thread pool size, default set to " + std::to_string(DEFAULT_POOLSIZE))
        .default_value(DEFAULT_POOLSIZE);
    
}


}