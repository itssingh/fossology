#include "licensematch.hpp"

LicenseMatch::LicenseMatch(string licenseName, int percentage, string licenseFullName, string textUrl, unsigned startPosition, unsigned length) :
  licenseName(licenseName),
  percentage(percentage),
  licenseFullName(licenseFullName),
  textUrl(textUrl),
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

const string LicenseMatch::getLicenseFullName() const
{
  return licenseFullName;
}

const string LicenseMatch::getTextUrl() const
{
  return textUrl;
}

unsigned LicenseMatch::getStartPosition() const
{
  return startPosition;
}

unsigned LicenseMatch::getLength() const
{
  return length;
}
