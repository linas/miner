/*
 * opencog/server/CogServerMain.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by Singularity Institute for Artificial Intelligence
 * All Rights Reserved
 *
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <boost/filesystem/operations.hpp>

#include <opencog/server/CogServer.h>
#include <opencog/server/load-file.h>
#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/misc.h>

using namespace opencog;

static const char* DEFAULT_CONFIG_FILENAME = "opencog.conf";
static const char* DEFAULT_CONFIG_PATHS[] = 
{
    CONFDIR,
#ifndef WIN32
    "/etc",
#endif // !WIN32
    NULL
};

static const char* DEFAULT_MODULE_PATHS[] = 
{
    "opencog",
    "../opencog",
#ifndef WIN32
    "/usr/share/opencog",
    "/usr/local/share/opencog",
#endif // !WIN32
    NULL
};

static void usage(char* progname)
{
    std::cerr << "Usage: " << progname << " [-c <config-file>]\n\n";
}

int main(int argc, char *argv[])
{
    // check command line
    if ((argc == 1) || ((argc == 3) && (strcmp(argv[1], "-c") == 0))) {
        // load the configuration from file if a filename was supplied on the command line
        if (argc == 3) {
            try {
                config().load(argv[2]);
            } catch (RuntimeException &e) {
                std::cerr << e.getMessage() << std::endl;
                return 1;
            }
        } else {
            // search for configuration file on default locations
            for (int i = 0; DEFAULT_CONFIG_PATHS[i] != NULL; ++i) {
                boost::filesystem::path configPath(DEFAULT_CONFIG_PATHS[i]);
                configPath /= DEFAULT_CONFIG_FILENAME;
                if (boost::filesystem::exists(configPath)) {
                    try {
                        config().load(configPath.string().c_str());
                        fprintf(stderr, "loaded configuration from file \"%s\"\n", configPath.string().c_str());
                        break;
                    } catch (RuntimeException &e) {
                        std::cerr << e.getMessage() << std::endl;
                        return 1;
                    }
                }
            }
        }

        // setup global logger
        logger().setFilename(config()["LOG_FILE"]);
        logger().setLevel(Logger::getLevelFromString(config()["LOG_LEVEL"]));
        logger().setBackTraceLevel(Logger::getLevelFromString(config()["BACK_TRACE_LOG_LEVEL"]));
        logger().setPrintToStdoutFlag(config().get_bool("LOG_TO_STDOUT"));
        //logger().setLevel(Logger::DEBUG);

        CogServer& cogserver = static_cast<CogServer&>(server());

        // Load modules specified in the config file
        std::vector<std::string> modules;
        tokenize(config()["MODULES"], std::back_inserter(modules), ", ");
        for (std::vector<std::string>::const_iterator it = modules.begin();
             it != modules.end(); ++it) {
            cogserver.loadModule(*it);
        }

        // Load scheme modules specified in the config file
        std::vector<std::string> scm_modules;
        tokenize(config()["SCM_PRELOAD"], std::back_inserter(scm_modules), ", ");
#ifdef HAVE_GUILE
        for (std::vector<std::string>::const_iterator it = scm_modules.begin();
             it != scm_modules.end(); ++it) {

            int rc = 2;
            const char * mod = "";
            for (int i = 0; DEFAULT_MODULE_PATHS[i] != NULL; ++i) {
                boost::filesystem::path modulePath(DEFAULT_MODULE_PATHS[i]);
                modulePath /= *it;
                if (boost::filesystem::exists(modulePath)) {
                    mod = modulePath.string().c_str();
                    rc = load_scm_file(mod);
                    if (0 == rc) break;
                }
            }
            if (rc)
            {
               logger().log(opencog::Logger::ERROR, "%d %s: %s", 
                     rc, strerror(rc), mod);
            }
            else
            {
                logger().log(opencog::Logger::INFO, "Loaded %s", mod);
            }
        }
#else /* HAVE_GUILE */
        logger().log(opencog::Logger::WARN,
            "Server compiled without SCM support");
#endif /* HAVE_GUILE */

        // enable the network server and run the server's main loop
        cogserver.enableNetworkServer();
        cogserver.serverLoop();
    } else {
        usage(argv[0]);
        return 1;
    }
}
