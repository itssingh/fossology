#include "licensematch.hpp"

LicenseMatch::LicenseMatch(string licenseName, float percentage) :
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

float LicenseMatch::getPercentage() const
{
  return percentage;
}