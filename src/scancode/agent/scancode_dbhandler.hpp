#ifndef SCANCODE_AGENT_DATABASE_HANDLER_HPP
#define SCANCODE_AGENT_DATABASE_HANDLER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "libfossAgentDatabaseHandler.hpp"
#include "libfossdbmanagerclass.hpp"


extern "C" {
#include "libfossology.h"
}

class ScancodeDatabaseHandler : public fo::AgentDatabaseHandler
{
public:
  ScancodeDatabaseHandler(fo::DbManager dbManager);
  ScancodeDatabaseHandler(ScancodeDatabaseHandler&& other) : fo::AgentDatabaseHandler(std::move(other)) {};
  ScancodeDatabaseHandler spawn() const;
  std::vector<unsigned long> queryFileIdsForUpload(int uploadId);
  long saveLicenseMatch(int agentId, long pFileId, long licenseId, int percentMatch);
  bool saveHighlightInfo(long licenseFileId, unsigned start, unsigned length);
  void insertOrCacheLicenseIdForName(std::string const& rfShortName, std::string const& rfFullname, std::string const& rfTexturl);
  unsigned long getCachedLicenseIdForName(std::string const& rfShortName) const;

private:
  unsigned long selectOrInsertLicenseIdForName(std::string rfShortname, std::string rfFullname, std::string rfTexturl);
  std::unordered_map<std::string,long> licenseRefCache;
};

#endif