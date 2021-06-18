#ifndef SCANCODE_AGENT_UTILS_HPP
#define SCANCODE_AGENT_UTILS_HPP

#define AGENT_NAME "scancode"
#define AGENT_DESC "scancode agent"
#define AGENT_ARS  "scancode_ars"

#include <string>
#include <vector>
#include "files.hpp"
#include "licensematch.hpp"
#include "scancode_state.hpp"

extern "C" {
#include "libfossology.h"
}

using namespace std;

State getState(fo::DbManager& dbManager);
int queryAgentId(fo::DbManager& dbManager);
int writeARS(const State& state, int arsId, int uploadId, int success, fo::DbManager& dbManager);
void bail(int exitval);
bool processUploadId(const State& state, int uploadId, ScancodeDatabaseHandler& databaseHandler);
bool matchPFileWithLicenses(const State& state, unsigned long pFileId, ScancodeDatabaseHandler& databaseHandler);
bool matchFileWithLicenses(const State& state, const fo::File& file, ScancodeDatabaseHandler& databaseHandler);
bool saveLicenseMatchesToDatabase(const State& state, const vector<LicenseMatch>& matches, unsigned long pFileId, ScancodeDatabaseHandler& databaseHandler);

#endif