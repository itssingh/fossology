#include "licensematch.hpp"

LicenseMatch::LicenseMatch(string licenseName, int percentage, unsigned startPosition, unsigned length) :
  licenseName(licenseName),
  percentage(percentage),
  startPosition(startPosition),
  length(length)
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

unsigned LicenseMatch::getStartPosition() const
{
  return startPosition;
}

unsigned LicenseMatch::getLength() const
{
  return length;
}