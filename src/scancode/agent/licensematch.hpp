#ifndef SCANCODE_AGENT_LICENSE_MATCH_HPP
#define SCANCODE_AGENT_LICENSE_MATCH_HPP

#include <string>

using namespace std;

class LicenseMatch
{
public:
  LicenseMatch(string licenseName, int percentage, string licenseFullName, string textUrl, unsigned startPosition, unsigned length );
  ~LicenseMatch();

  const string getLicenseName() const;
  int getPercentage() const;
  const string getLicenseFullName() const;
  const string getTextUrl() const;
  unsigned getStartPosition() const;
  unsigned getLength() const;


private:
  string licenseName;
  int percentage;
  string licenseFullName; 
  string textUrl;
  unsigned startPosition;
  unsigned length;
  
};

#endif