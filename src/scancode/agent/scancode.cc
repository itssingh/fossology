#include<iostream>
#include "scancode.hpp"
#include "scancode_wrapper.hpp"

using namespace fo;
using namespace std;

int main(int argc, char** argv)
{
  DbManager dbManager(&argc, argv);
  ScancodeDatabaseHandler databaseHandler(dbManager);
  
  State state = getState(dbManager);

  while (fo_scheduler_next() != NULL)
  {
    int uploadId = atoi(fo_scheduler_current());

    if (uploadId == 0) continue;

    int arsId = writeARS(state, 0, uploadId, 0, dbManager);

    if (arsId <= 0)
      bail(5);

    if (!processUploadId(state, uploadId, databaseHandler))
      bail(2);

    fo_scheduler_heart(0);
    writeARS(state, arsId, uploadId, 1, dbManager);
  }
  fo_scheduler_heart(0);

  /* do not use bail, as it would prevent the destructors from running */
  fo_scheduler_disconnect(0);
  return 0;
}