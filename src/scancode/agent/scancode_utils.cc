/*****************************************************************************
 * SPDX-License-Identifier: GPL-2.0
 * SPDX-FileCopyrightText: 2021 Sarita Singh <saritasingh.0425@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ****************************************************************************/

#include "scancode_utils.hpp"
#include "scancode_wrapper.hpp"
#include <iostream>

using namespace fo;

/**
 * @brief Disconnect with scheduler returning an error code and exit
 * @param exitval Error code
 */
void bail(int exitval) {
  fo_scheduler_disconnect(exitval);
  exit(exitval);
}

/**
 * @brief Get state of the agent
 * @param dbManager DBManager to be used
 * @return  state of the agent
 */
State getState(DbManager &dbManager) {
  int agentId = queryAgentId(dbManager);
  return State(agentId);
}

/**
 * @brief Get agent id, exit if agent id is incorrect
 * @param[in]  dbConn Database connection object
 * @return ID of the agent
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

/**
 * @brief Write ARS to the agent's ars table
 * @param state State of the agent
 * @param arsId ARS id (0 for new entry)
 * @param uploadId  Upload ID
 * @param success Success status
 * @param dbManager DbManager to use
 * @return ARS ID.
 */
int writeARS(const State &state, int arsId, int uploadId, int success,
             DbManager &dbManager) {
  PGconn *connection = dbManager.getConnection();
  int agentId = state.getAgentId();

  return fo_WriteARS(connection, arsId, uploadId, agentId, AGENT_ARS, NULL,
                     success);
}

/**
 * @brief Process a given upload id, scan from statements and add to database
 * @param state State of the agent
 * @param uploadId  Upload id to be processed
 * @param databaseHandler Database handler object
 * @return True when upload is successful
 */
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
 * @brief 
 * @param state State of the agent
 * @param pFileId pfile Id of upload
 * @param databaseHandler Database handler object
 * @return true 
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
 * @brief match file with license
 * 
 * scan file with scancode and save result in the database
 *  
 * @param state state of the agent
 * @param file  code/binary file sent by scheduler 
 * @param databaseHandler databaseHandler Database handler object
 * @return true on saving scan result successfully, false otherwise
 */
bool matchFileWithLicenses(const State &state, const fo::File &file,
                           ScancodeDatabaseHandler &databaseHandler) {
  string scancodeResult = scanFileWithScancode(state, file);
map<string, vector<Match>> scancodeData =
      extractDataFromScancodeResult(scancodeResult, file.getFileName());

return saveLicenseMatchesToDatabase(
           state, scancodeData["scancode_license"], file.getId(),
           databaseHandler) &&
       saveOtherMatchesToDatabase(
           state, scancodeData["scancode_statement"], file.getId(),
           databaseHandler) &&
       saveOtherMatchesToDatabase(
           state, scancodeData["scancode_author"], file.getId(),
           databaseHandler);
}

/**
 * @brief save license matches to database
 * 
 * insert or catch license in license_ref table
 * save license in license_file table
 * save license highlight inforamtion in highlight table
 * 
 * @param state state of the agent
 * @param matches vector of objects of Match class
 * @param pFileId pfile Id
 * @param databaseHandler databaseHandler Database handler object
 * @return true on success, false otherwise
 */
bool saveLicenseMatchesToDatabase(const State &state,
                                  const vector<Match> &matches,
                                  unsigned long pFileId,
                                  ScancodeDatabaseHandler &databaseHandler) {
  for (vector<Match>::const_iterator it = matches.begin();
       it != matches.end(); ++it) {
    const Match &match = *it;
    databaseHandler.insertOrCacheLicenseIdForName(
        match.getMatchName(), match.getLicenseFullName(), match.getTextUrl());
  }

  if (!databaseHandler.begin())
    return false;
  if (matches.size() == 0) {
    cout << "No license found\n";
    if (!databaseHandler.insertNoResultInDatabase(state.getAgentId(), pFileId))
      return false;
  } 
  else {
    for (vector<Match>::const_iterator it = matches.begin();
         it != matches.end(); ++it) {
      const Match &match = *it;

      int agentId = state.getAgentId();
      string rfShortname = match.getMatchName();
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

/**
 * @brief save copyright/copyright holder in the database
 * @param state state of the agent
 * @param matches vector of objects of Match class
 * @param pFileId pfile Id
 * @param databaseHandler databaseHandler Database handler object
 * @return  true on success, false otherwise
 */
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