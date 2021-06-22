#ifndef SCANCODE_AGENT_SCANCODE_WRAPPER_HPP
#define SCANCODE_AGENT_SCANCODE_WRAPPER_HPP

#define AGENT_NAME "scancode"
#define AGENT_DESC "scancode agent"
#define AGENT_ARS  "scancode_ars"

#include <string>
#include <vector>
#include<unordered_map>
#include "files.hpp"
#include "licensematch.hpp" 
#include "scancode_state.hpp"

#include <jsoncpp/json/json.h>

using namespace std;

string scanFileWithScancode(const State& state, const fo::File& file);
string scanFileWithScancode(string filename);
vector<LicenseMatch> extractLicensesFromScancodeResult( const string& scancodeResult, const string& filename);

#endif 
