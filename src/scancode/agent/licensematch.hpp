#ifndef SCANCODE_AGENT_LICENSE_MATCH_HPP
#define SCANCODE_AGENT_LICENSE_MATCH_HPP

#include <string>

using namespace std;

class LicenseMatch
{
public:
  LicenseMatch(string licenseName, float percentage);
  ~LicenseMatch();

  const string getLicenseName() const;
  float getPercentage() const;

private:
  string licenseName;
  float percentage;
};

#endif