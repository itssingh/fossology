#ifndef SCANCODE_AGENT_DATABASE_HANDLER_HPP
#define SCANCODE_AGENT_DATABASE_HANDLER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "match.hpp"

#include "libfossAgentDatabaseHandler.hpp"
#include "libfossdbmanagerclass.hpp"


extern "C" {
#include "libfossology.h"
}

#define MAX_TABLE_CREATION_RETRIES 5

/**
 * \class DatabaseEntry
 * \brief Maps agent data to database schema
 */
class DatabaseEntry
{
public:
  DatabaseEntry();
  DatabaseEntry(Match match,unsigned long agentId, unsigned long pfileId);
  unsigned long agent_fk;                    /**< Id of agent performed the scan */
  unsigned long pfile_fk;                    /**< Id of pfile on which the scan was performed */
  std::string content;              /**< Statement found during the scan */
  std::string hash;                 /**< MD5 hash of the statement */
  /**
   * \brief Type of statement found.
   *
   * Can be
   *   - statement for Copyright
   *   - author for Author
   *   - url for URL
   *   - email for email
   *   - ecc for ECC
   */
  std::string type;
  unsigned copy_startbyte;               /**< Statement start offset from start of pfile content */
  unsigned copy_endbyte;                 /**< Statement end offset from start of pfile content */
};

class ScancodeDatabaseHandler : public fo::AgentDatabaseHandler
{
public:
  ScancodeDatabaseHandler(fo::DbManager dbManager);
  ScancodeDatabaseHandler(ScancodeDatabaseHandler&& other) : fo::AgentDatabaseHandler(std::move(other)) {};
  ScancodeDatabaseHandler spawn() const;
  long saveLicenseMatch(int agentId, long pFileId, long licenseId, int percentMatch);
  bool insertNoResultInDatabase(int agentId, long pFileId);

  bool saveHighlightInfo(long licenseFileId, unsigned start, unsigned length);
  void insertOrCacheLicenseIdForName(std::string const& rfShortName, std::string const& rfFullname, std::string const& rfTexturl);
  unsigned long getCachedLicenseIdForName(std::string const& rfShortName) const;
  bool insertInDatabase(DatabaseEntry& entry) const;
  std::vector<unsigned long> queryFileIdsForUpload(int uploadId);
private:
  unsigned long selectOrInsertLicenseIdForName(std::string rfShortname, std::string rfFullname, std::string rfTexturl);
  std::unordered_map<std::string,long> licenseRefCache;
  /**
   * \struct ColumnDef
   * \brief Holds the column related data for table creation
   * \see CopyrightDatabaseHandler::columns
   */
  typedef struct
  {
    const char* name;               /**< Name of the table column */
    const char* type;               /**< Data type of the table column */
    const char* creationFlags;      /**< Special flags of the table column */
  } ColumnDef;

  static const ColumnDef columns[];
  static const ColumnDef columnsDecision[];

  bool createTableAgentFindings() const;
  std::string getColumnListString(const ColumnDef in[], size_t size) const;
  std::string getColumnCreationString(const ColumnDef in[], size_t size) const;
};

#endif