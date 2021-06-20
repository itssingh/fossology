#ifndef SCANCODE_AGENT_LICENSE_MATCH_HPP
#define SCANCODE_AGENT_LICENSE_MATCH_HPP

#include <string>

using namespace std;

class LicenseMatch
{
public:
  LicenseMatch(string licenseName, int percentage);
  ~LicenseMatch();

  const string getLicenseName() const;
  int getPercentage() const;

private:
  string licenseName;
  int percentage;
};

#endif