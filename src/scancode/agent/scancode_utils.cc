#include "scancode_utils.hpp"
#include "scancode_wrapper.hpp"
#include <iostream>

using namespace fo;

/**
 * \brief Disconnect with scheduler returning an error code and exit
 * \param exitval Error code
 */
void bail(int exitval) {
  fo_scheduler_disconnect(exitval);
  exit(exitval);
}

State getState(DbManager &dbManager) {
  int agentId = queryAgentId(dbManager);
  return State(agentId);
}

/**
 * \brief Get agent id, exit if agent id is incorrect
 * \param[in]  dbConn Database connection object
 * \return ID of the agent
 */

int queryAgentId(DbManager &dbManager) {
  char *COMMIT_HASH = fo_sysconfig(AGENT_NAME, "COMMIT_HASH");
  char *VERSION = fo_sysconfig(AGENT_NAME, "VERSION");
  char *agentRevision;

  if (!asprintf(&agentRevision, "%s.%s", VERSION, COMMIT_HASH))
    bail(-1);

  int agentId = fo_GetAgentKey(dbManager.getConnection(), AGENT_NAME, 0,
                               agentRevision, AGENT_DESC);
  free(agentRevision);

  if (agentId <= 0)
    bail(1);

  return agentId;
}

int writeARS(const State &state, int arsId, int uploadId, int success,
             DbManager &dbManager) {
  PGconn *connection = dbManager.getConnection();
  int agentId = state.getAgentId();

  return fo_WriteARS(connection, arsId, uploadId, agentId, AGENT_ARS, NULL,
                     success);
}

bool processUploadId(const State &state, int uploadId,
                     ScancodeDatabaseHandler &databaseHandler) {
  vector<unsigned long> fileIds =
      databaseHandler.queryFileIdsForUpload(uploadId);

  bool errors = false;
#pragma omp parallel
  {
    ScancodeDatabaseHandler threadLocalDatabaseHandler(databaseHandler.spawn());

    size_t pFileCount = fileIds.size();
#pragma omp for
    for (size_t it = 0; it < pFileCount; ++it) {
      if (errors)
        continue;

      unsigned long pFileId = fileIds[it];

      if (pFileId == 0)
        continue;

      if (!matchPFileWithLicenses(state, pFileId, threadLocalDatabaseHandler)) {
        errors = true;
      }

      fo_scheduler_heart(1);
    }
  }

  return !errors;
}
/**
 * Description:
 * @param state what is state
 * @param pFileId what it it
 * @param databaseHandler what that
 *
 * @return some value
 */

bool matchPFileWithLicenses(const State &state, unsigned long pFileId,
                            ScancodeDatabaseHandler &databaseHandler) {
  char *pFile = databaseHandler.getPFileNameForFileId(pFileId);

  if (!pFile) {
    cout << "File not found " << pFileId << endl;
    bail(8);
  }

  char *fileName = NULL;
  {
#pragma omp critical(repo_mk_path)
    fileName = fo_RepMkPath("files", pFile);
  }
  if (fileName) {
    fo::File file(pFileId, fileName);

    if (!matchFileWithLicenses(state, file, databaseHandler))
      return false;

    free(fileName);
    free(pFile);
  } else {
    cout << "PFile not found in repo " << pFileId << endl;
    bail(7);
  }

  return true;
}

/**
 * First insert into license cache so that if the licencse if new it gets
 * inserted or if old then simply return primary_key from license_ref table
 *
 * second we query license cahce for the primary key to store the short_name
 * and percentage and pk all in license_file table
 */

bool matchFileWithLicenses(const State &state, const fo::File &file,
                           ScancodeDatabaseHandler &databaseHandler) {
  string scancodeResult = scanFileWithScancode(state, file);
  vector<LicenseMatch> scancodeLicenseNames =
      extractLicensesFromScancodeResult(scancodeResult, file.getFileName());
  vector<Match> scancodeOtherNames =
      extractOthersFromScancodeResult(scancodeResult, file.getFileName());

  return saveLicenseMatchesToDatabase(state, scancodeLicenseNames, file.getId(),
                                      databaseHandler) &&
         saveOtherMatchesToDatabase(state, scancodeOtherNames, file.getId(),
                                    databaseHandler);
}

bool saveLicenseMatchesToDatabase(const State &state,
                                  const vector<LicenseMatch> &matches,
                                  unsigned long pFileId,
                                  ScancodeDatabaseHandler &databaseHandler) {
  for (vector<LicenseMatch>::const_iterator it = matches.begin();
       it != matches.end(); ++it) {
    const LicenseMatch &match = *it;
    databaseHandler.insertOrCacheLicenseIdForName(
        match.getLicenseName(), match.getLicenseFullName(), match.getTextUrl());
  }

  if (!databaseHandler.begin())
    return false;
  if (matches.size() == 0) {
    cout << "No license found\n";
    if (!databaseHandler.insertNoResultInDatabase(state.getAgentId(), pFileId))
      return false;
  } 
  else {
    for (vector<LicenseMatch>::const_iterator it = matches.begin();
         it != matches.end(); ++it) {
      const LicenseMatch &match = *it;

      int agentId = state.getAgentId();
      string rfShortname = match.getLicenseName();
      int percent = match.getPercentage();
      unsigned start = match.getStartPosition();
      unsigned length = match.getLength();

      unsigned long licenseId =
          databaseHandler.getCachedLicenseIdForName(rfShortname);

      if (licenseId == 0) {
        databaseHandler.rollback();
        cout << "cannot get licenseId for shortname '" + rfShortname + "'"
             << endl;
        return false;
      }

      long licenseFileId = databaseHandler.saveLicenseMatch(agentId, pFileId,
                                                            licenseId, percent);

      if (licenseFileId > 0) {
        bool highlightRes =
            databaseHandler.saveHighlightInfo(licenseFileId, start, length);
        if (!highlightRes) {
          databaseHandler.rollback();
          cout << "failing save licensehighlight" << endl;
        }
      } else {
        databaseHandler.rollback();
        cout << "failing save licenseMatch" << endl;
        return false;
      }
    }
  }
  return databaseHandler.commit();
}

bool saveOtherMatchesToDatabase(const State &state,
                                const vector<Match> &matches,
                                unsigned long pFileId,
                                ScancodeDatabaseHandler &databaseHandler) {
  

  if (!databaseHandler.begin())
    return false;

  for (vector<Match>::const_iterator it = matches.begin();
       it != matches.end(); ++it) {
    const Match &match = *it;
    DatabaseEntry entry(match,state.getAgentId(),pFileId);

    if (!databaseHandler.insertInDatabase(entry))
    {
      databaseHandler.rollback();
      cout << "failing save otherMatches" << endl;
      return false;
    }
  }
  return databaseHandler.commit();
}