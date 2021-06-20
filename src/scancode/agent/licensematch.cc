#include "licensematch.hpp"

LicenseMatch::LicenseMatch(string licenseName, int percentage) :
  licenseName(licenseName),
  percentage(percentage)
{
}

LicenseMatch::~LicenseMatch()
{
}

const string LicenseMatch::getLicenseName() const
{
  return licenseName;
}

int LicenseMatch::getPercentage() const
{
  return percentage;
}