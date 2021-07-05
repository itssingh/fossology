#include "scancode_dbhandler.hpp"
#include "libfossUtils.hpp"

#include <iostream>

using namespace fo;
using namespace std;

// HACK: Make an object for license match fields then pass them to functions.

/**
 * \brief Default constructor for DatabaseEntry
 */
DatabaseEntry::DatabaseEntry() :
        agent_fk(0),
        pfile_fk(0),
        content(""),
        hash(""),
        type(""),
        copy_startbyte(0),
        copy_endbyte(0)
{
};

DatabaseEntry::DatabaseEntry(Match match,unsigned long agentId, unsigned long pfileId) :
        agent_fk(agentId),
        pfile_fk(pfileId),
        hash("")  
{
  content = match.getMatchName();
  type = match.getType();
  copy_startbyte = match.getStartPosition();
  copy_endbyte = match.getStartPosition() + match.getLength();
};


std::string ScancodeDatabaseHandler::getColumnCreationString(const ScancodeDatabaseHandler::ColumnDef in[], size_t size) const
{
  std::string result;
  for (size_t i = 0; i < size; ++i)
  {
    if (i != 0)
      result += ", ";
    result += in[i].name;
    result += " ";
    result += in[i].type;
    result += " ";
    result += in[i].creationFlags;
  }
  return result;
}


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

// DONE insertNoResultInDatabase

// ASK: scancode should display no license or just insert null?

/**
 * @brief Save no result to the database
 * @param entry Entry containing the agent id and file id
 * @return True of successful insertion, false otherwise
 */
bool ScancodeDatabaseHandler::insertNoResultInDatabase(int agentId, long pFileId )
{
  return saveLicenseMatch(agentId, pFileId, 320, NULL); //320 is not constant
}

long ScancodeDatabaseHandler::saveLicenseMatch(
  int agentId, 
  long pFileId, 
  long licenseId, 
  int percentMatch)
{
  QueryResult result = dbManager.execPrepared(
    fo_dbManager_PrepareStamement(
      dbManager.getStruct_dbManager(),
      "saveLicenseMatch",
      "WITH "
          "selectExisting AS ("
          "SELECT fl_pk FROM ONLY license_file"
          " WHERE (agent_fk = $1 AND pfile_fk = $2 AND rf_fk = $3)"
          "),"
          "insertNew AS ("
          "INSERT INTO license_file"
          "(agent_fk, pfile_fk, rf_fk, rf_match_pct)"
          " SELECT $1, $2, $3, $4"
          " WHERE NOT EXISTS(SELECT * FROM license_file WHERE (agent_fk = $1 AND pfile_fk = $2 AND rf_fk = $3))"
          " RETURNING fl_pk"
          ") "
          "SELECT fl_pk FROM insertNew "
          "UNION "
          "SELECT fl_pk FROM selectExisting",
      int, long, long, unsigned
    ),
    agentId,
    pFileId,
    licenseId,
    percentMatch
  );

  long licenseFilePK= -1;
  if(!result.isFailed()){
    
    vector<unsigned long> res = result.getSimpleResults<unsigned long>(0,
    fo::stringToUnsignedLong);

    licenseFilePK = res.at(0);
  }
  return licenseFilePK;  
}

// DONE: remove glitch in UI for license highlight 
// BUG: more than one href link to highlight
// HACK: inserted only unique entries in license_file and highlight table.

// TODO use 'S' instead of 'L' for scancode highlight in highlight.type
// HACK: add to match highlight instead of relevent text, because scancode matches text
// ASK which is better?

bool ScancodeDatabaseHandler::saveHighlightInfo(
  long licenseFileId,
  unsigned start,
  unsigned length)
{
  cout<<"saving highlight info"<<endl;
  return dbManager.execPrepared(
    fo_dbManager_PrepareStamement(
      dbManager.getStruct_dbManager(),
      "saveHighlightInfo",
          "INSERT INTO highlight"
          "(fl_fk, type, start, len)"
          " SELECT $1, 'S', $2, $3 "
          " WHERE NOT EXISTS(SELECT * FROM highlight WHERE (fl_fk = $1 AND start = $2 AND len = $3))",
      long, unsigned, unsigned
    ),
    licenseFileId,
    start,
    length
  );}


void ScancodeDatabaseHandler::insertOrCacheLicenseIdForName(string const& rfShortName, string const& rfFullName, string const& rfTextUrl)
{
  if (getCachedLicenseIdForName(rfShortName)==0)
  {
    unsigned long licenseId = selectOrInsertLicenseIdForName(rfShortName, rfFullName, rfTextUrl);

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

//  TODO insert license text to database
// either create a scancode plugin or use local data 

// insert license if not present in license_ref and return its primary key
unsigned long ScancodeDatabaseHandler::selectOrInsertLicenseIdForName(string rfShortName, string rfFullname, string rfTexturl)
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
            "INSERT INTO license_ref(rf_shortname, rf_text, rf_detector_type, rf_fullname, rf_url)"
            " SELECT $1, $2, $3, $4, $5"
            " WHERE NOT EXISTS(SELECT * FROM selectExisting)"
            " RETURNING rf_pk"
          ") "

        "SELECT rf_pk FROM insertNew "
        "UNION "
        "SELECT rf_pk FROM selectExisting",
        char*, char*, int, char* , char* 
      ),
      rfShortName.c_str(),
      "License by Scancode.",
      4,
      rfFullname.c_str(),
      rfTexturl.c_str()
      
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


bool ScancodeDatabaseHandler::insertInDatabase(DatabaseEntry& entry) const
{
  std::string tableName = "scancode_author";

  if("copyright" == entry.type ){
    tableName = "scancode_copyright";
  }

  return dbManager.execPrepared(
    fo_dbManager_PrepareStamement(
      dbManager.getStruct_dbManager(),
      ("insertInDatabaseFor " + tableName).c_str(),
      ("INSERT INTO "+ tableName +
      "(agent_fk, pfile_fk, content, hash, type, copy_startbyte, copy_endbyte)" +
        " SELECT $1, $2, $3, md5($3), $4, $5, $6 "
        " WHERE NOT EXISTS(SELECT * FROM " + tableName +
        " WHERE (hash = md5($3)))").c_str(),
        long, long, char*, char*, int, int
    ),
    entry.agent_fk, entry.pfile_fk,
    entry.content.c_str(),
    entry.type.c_str(),
    entry.copy_startbyte, entry.copy_endbyte
  );
}
// WIP Add new copyright and author tables for scancode

/**
 * \brief Create tables required by agent
 *
 * Calls createTableAgentFindings() and createTableClearing()
 * to create the tables required by the agent to work.
 *
 * The function tries to create table in maximum of MAX_TABLE_CREATION_RETRIES
 * attempts.
 * \return True if success, false otherwise
 */
bool ScancodeDatabaseHandler::createTables() const
{
  int failedCounter = 0;
  bool tablesChecked = false;

  dbManager.ignoreWarnings(true);
  while (!tablesChecked && failedCounter < MAX_TABLE_CREATION_RETRIES)
  {
    dbManager.begin();
    tablesChecked = createTableAgentFindings("scancode_copyright") && createTableAgentFindings("scancode_author");


    if (tablesChecked)
      dbManager.commit();
    else
    {
      dbManager.rollback();
      ++failedCounter;
      if (failedCounter < MAX_TABLE_CREATION_RETRIES)
        std::cout << "WARNING: table creation failed: trying again"
          " (" << failedCounter << "/" << MAX_TABLE_CREATION_RETRIES << ")"
          << std::endl;
    }
  }
  if (tablesChecked && (failedCounter > 0))
    std::cout << "NOTICE: table creation succeded on try "
      << failedCounter << "/" << MAX_TABLE_CREATION_RETRIES
      << std::endl;

  dbManager.ignoreWarnings(false);
  return tablesChecked;
}

/**
 * \brief Columns required by agent in database
 * \todo Removed constrain: "CHECK (type in ('statement', 'email', 'url'))"}
 */
const ScancodeDatabaseHandler::ColumnDef
    ScancodeDatabaseHandler::columns_copyright[] = {
#define CSEQUENCE_NAME "scancode_copyright_pk_seq"
#define CCOLUMN_NAME_PK "scancode_copyright_pk"
        {CCOLUMN_NAME_PK, "bigint",
         "PRIMARY KEY DEFAULT nextval('" CSEQUENCE_NAME "'::regclass)"},
        {"agent_fk", "bigint", "NOT NULL"},
        {"pfile_fk", "bigint", "NOT NULL"},
        {"content", "text", ""},
        {"hash", "text", ""},
        {"type", "text", ""},
        {"copy_startbyte", "integer", ""},
        {"copy_endbyte", "integer", ""},
        {"is_enabled", "boolean", "NOT NULL DEFAULT TRUE"},
};

const ScancodeDatabaseHandler::ColumnDef
    ScancodeDatabaseHandler::columns_author[] = {
#define ASEQUENCE_NAME "scancode_author_pk_seq"
#define ACOLUMN_NAME_PK "scancode_author_pk"
        {ACOLUMN_NAME_PK, "bigint",
         "PRIMARY KEY DEFAULT nextval('" ASEQUENCE_NAME "'::regclass)"},
        {"agent_fk", "bigint", "NOT NULL"},
        {"pfile_fk", "bigint", "NOT NULL"},
        {"content", "text", ""},
        {"hash", "text", ""},
        {"type", "text", ""},
        {"copy_startbyte", "integer", ""},
        {"copy_endbyte", "integer", ""},
        {"is_enabled", "boolean", "NOT NULL DEFAULT TRUE"},
};

/**
 * \brief Create table to store agent find data
 * \return True on success, false otherwise
 * \see CopyrightDatabaseHandler::columns
 */
bool ScancodeDatabaseHandler::createTableAgentFindings( string tableName) const
{
  const char *tablename;
  const char *sequencename;
  if (tableName == "scancode_copyright") {
    tablename = "scancode_copyright";
    sequencename = "scancode_copyright_pk_seq";
  } else if (tableName == "scancode_author") {
    tablename = "scancode_author";
    sequencename = "scancode_author_pk_seq";
  }
  cout << "CHECK here " << tablename << " " << sequencename<<"\n";
  if (!dbManager.sequenceExists(sequencename)) {
    RETURN_IF_FALSE(dbManager.queryPrintf("CREATE SEQUENCE %s"
      " START WITH 1"
        " INCREMENT BY 1"
        " NO MAXVALUE"
        " NO MINVALUE"
        " CACHE 1",sequencename));
  }

  if (!dbManager.tableExists(tablename))
  {
    if (tableName == "scancode_copyright") {
    size_t ncolumns = (sizeof(ScancodeDatabaseHandler::columns_copyright) / sizeof(ScancodeDatabaseHandler::ColumnDef));
    RETURN_IF_FALSE(dbManager.queryPrintf("CREATE table %s(%s)", tablename,
      getColumnCreationString(ScancodeDatabaseHandler::columns_copyright, ncolumns).c_str()
    )
    );
  } else if (tableName == "scancode_author") {
    size_t ncolumns = (sizeof(ScancodeDatabaseHandler::columns_author) / sizeof(ScancodeDatabaseHandler::ColumnDef));
    RETURN_IF_FALSE(dbManager.queryPrintf("CREATE table %s(%s)", tablename,
      getColumnCreationString(ScancodeDatabaseHandler::columns_author, ncolumns).c_str()
    )
    );
  }
    
    RETURN_IF_FALSE(dbManager.queryPrintf(
      "CREATE INDEX %s_agent_fk_index"
        " ON %s"
        " USING BTREE (agent_fk)",
      tablename, tablename
    ));

    RETURN_IF_FALSE(dbManager.queryPrintf(
      "CREATE INDEX %s_hash_index"
        " ON %s"
        " USING BTREE (hash)",
      tablename, tablename
    ));

    RETURN_IF_FALSE(dbManager.queryPrintf(
      "CREATE INDEX %s_pfile_fk_index"
        " ON %s"
        " USING BTREE (pfile_fk)",
      tablename, tablename
    ));

    RETURN_IF_FALSE(dbManager.queryPrintf(
      "ALTER TABLE ONLY %s"
        " ADD CONSTRAINT agent_fk"
        " FOREIGN KEY (agent_fk)"
        " REFERENCES agent(agent_pk) ON DELETE CASCADE",
      tablename
    ));

    RETURN_IF_FALSE(dbManager.queryPrintf(
      "ALTER TABLE ONLY %s"
        " ADD CONSTRAINT pfile_fk"
        " FOREIGN KEY (pfile_fk)"
        " REFERENCES pfile(pfile_pk) ON DELETE CASCADE",
      tablename
    ));
  }
  return true;
}

// TODO insertNoResult function for copyright and author


