#include "scancode_dbhandler.hpp"
#include "libfossUtils.hpp"

#include <iostream>

using namespace fo;
using namespace std;

/**
 * Default constructor for scancodeDatabaseHandler
 * @param dbManager DBManager to be used
 */
ScancodeDatabaseHandler::ScancodeDatabaseHandler(DbManager dbManager) :
  fo::AgentDatabaseHandler(dbManager)
{
}

/**
 * Spawn a new DbManager object.
 *
 * Used to create new objects for threads.
 * @return DbManager object for threads.
 */

ScancodeDatabaseHandler ScancodeDatabaseHandler::spawn() const
{
  DbManager spawnedDbMan(dbManager.spawn());
  return ScancodeDatabaseHandler(spawnedDbMan);
}

/**
 * Get a vector of all file id for a given upload id.
 * @param uploadId Upload ID to be queried
 * @return uploadId
 */
vector<unsigned long> ScancodeDatabaseHandler::queryFileIdsForUpload(int uploadId)
{
  return queryFileIdsVectorForUpload(uploadId,true);
}

bool ScancodeDatabaseHandler::saveLicenseMatch(
  int agentId, 
  long pFileId, 
  long licenseId, 
  int percentMatch)
{
  return dbManager.execPrepared(
    fo_dbManager_PrepareStamement(
      dbManager.getStruct_dbManager(),
      "saveLicenseMatch",
      "INSERT INTO license_file (agent_fk, pfile_fk, rf_fk, rf_match_pct) VALUES ($1, $2, $3, $4)",
      int, long, long, unsigned
    ),
    agentId,
    pFileId,
    licenseId,
    percentMatch
  );
}

void ScancodeDatabaseHandler::insertOrCacheLicenseIdForName(string const& rfShortName)
{
  if (getCachedLicenseIdForName(rfShortName)==0)
  {
    unsigned long licenseId = selectOrInsertLicenseIdForName(rfShortName);

    if (licenseId > 0)
    {
      licenseRefCache.insert(std::make_pair(rfShortName, licenseId));
    }
  }
}

unsigned long ScancodeDatabaseHandler::getCachedLicenseIdForName(string const& rfShortName) const
{
  std::unordered_map<string,long>::const_iterator findIterator = licenseRefCache.find(rfShortName);
  if (findIterator != licenseRefCache.end())
  {
    return findIterator->second;
  }
  else
  {
    return 0;
  }
}

/**
 * Helper function to check if a string ends with other string.
 * @param firstString The string to be checked
 * @param ending      The ending string
 * @return True if first string has the ending string at end, false otherwise.
 */
bool hasEnding(string const &firstString, string const &ending)
{
  if (firstString.length() >= ending.length())
  {
    return (0
      == firstString.compare(firstString.length() - ending.length(),
        ending.length(), ending));
  }
  else
  {
    return false;
  }
}

// insert license if not present in license_ref and return its primary key
unsigned long ScancodeDatabaseHandler::selectOrInsertLicenseIdForName(string rfShortName)
{
  bool success = false;
  unsigned long result = 0;


  icu::UnicodeString unicodeCleanShortname = fo::recodeToUnicode(rfShortName);

  // Clean shortname to get utf8 string
  rfShortName = "";
  unicodeCleanShortname.toUTF8String(rfShortName);

  fo_dbManager_PreparedStatement *searchWithOr = fo_dbManager_PrepareStamement(
      dbManager.getStruct_dbManager(),
      "selectLicenseIdWithOrScancode",
      " SELECT rf_pk FROM ONLY license_ref"
      " WHERE LOWER(rf_shortname) = LOWER($1)"
      " OR LOWER(rf_shortname) = LOWER($2);",
      char*, char*);

  /* First check similar matches */
  /* Check if the name ends with +, -or-later, -only */
  if (hasEnding(rfShortName, "+") || hasEnding(rfShortName, "-or-later"))
  {
    string tempShortName(rfShortName);
    /* Convert shortname to lower-case */
    std::transform(tempShortName.begin(), tempShortName.end(), tempShortName.begin(),
      ::tolower);
    string plus("+");
    string orLater("-or-later");

    unsigned long int plusLast = tempShortName.rfind(plus);
    unsigned long int orLaterLast = tempShortName.rfind(orLater);

    /* Remove last occurrence of + and -or-later (if found) */
    if (plusLast != string::npos)
    {
      tempShortName.erase(plusLast, string::npos);
    }
    if (orLaterLast != string::npos)
    {
      tempShortName.erase(orLaterLast, string::npos);
    }

    QueryResult queryResult = dbManager.execPrepared(searchWithOr,
        (tempShortName + plus).c_str(), (tempShortName + orLater).c_str());

    success = queryResult && queryResult.getRowCount() > 0;
    if (success)
    {
      result = queryResult.getSimpleResults<unsigned long>(0, fo::stringToUnsignedLong)[0];
    }
  }
  else
  {
    string tempShortName(rfShortName);
    /* Convert shortname to lower-case */
    std::transform(tempShortName.begin(), tempShortName.end(), tempShortName.begin(),
      ::tolower);
    string only("-only");

    unsigned long int onlyLast = tempShortName.rfind(only);

    /* Remove last occurrence of -only (if found) */
    if (onlyLast != string::npos)
    {
      tempShortName.erase(onlyLast, string::npos);
    }

    QueryResult queryResult = dbManager.execPrepared(searchWithOr,
        tempShortName.c_str(), (tempShortName + only).c_str());

    success = queryResult && queryResult.getRowCount() > 0;
    if (success)
    {
      result = queryResult.getSimpleResults<unsigned long>(0, fo::stringToUnsignedLong)[0];
    }
  }

  if (result > 0)
  {
    return result;
  }


  unsigned count = 0;
  while ((!success) && count++<3)
  {
    if (!dbManager.begin())
      continue;

    dbManager.queryPrintf("LOCK TABLE license_ref");

    QueryResult queryResult = dbManager.execPrepared(
      fo_dbManager_PrepareStamement(
        dbManager.getStruct_dbManager(),
        "selectOrInsertLicenseIdForName",
        "WITH "
          "selectExisting AS ("
            "SELECT rf_pk FROM ONLY license_ref"
            " WHERE rf_shortname = $1"
          "),"
          "insertNew AS ("
            "INSERT INTO license_ref(rf_shortname, rf_text, rf_detector_type)"
            " SELECT $1, $2, $3"
            " WHERE NOT EXISTS(SELECT * FROM selectExisting)"
            " RETURNING rf_pk"
          ") "

        "SELECT rf_pk FROM insertNew "
        "UNION "
        "SELECT rf_pk FROM selectExisting",
        char*, char*, int
      ),
      rfShortName.c_str(),
      "License by Scancode.",
      3 
      // ASK: why is 3 a detector type?
    );

    success = queryResult && queryResult.getRowCount() > 0;

    if (success) {
      success &= dbManager.commit();

      if (success) {
        result = queryResult.getSimpleResults(0, fo::stringToUnsignedLong)[0];
      }
    } else {
      dbManager.rollback();
    }
  }

  return result;
}
