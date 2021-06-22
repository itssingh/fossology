#ifndef SCANCODE_AGENT_LICENSE_MATCH_HPP
#define SCANCODE_AGENT_LICENSE_MATCH_HPP

#include <string>

using namespace std;

class LicenseMatch
{
public:
  LicenseMatch(string licenseName, int percentage, unsigned startPosition, unsigned length );
  ~LicenseMatch();

  const string getLicenseName() const;
  int getPercentage() const;
  unsigned getStartPosition() const;
  unsigned getLength() const;

private:
  string licenseName;
  int percentage;
  unsigned startPosition;
  unsigned length;
};

#endif