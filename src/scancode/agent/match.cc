#include "match.hpp"

Match::Match(string type, string matchName, unsigned startPosition,
             unsigned length)
    : type(type), matchName(matchName), startPosition(startPosition),
      length(length) {}

Match::~Match() {}

const string Match::getType() const { return type; }
const string Match::getMatchName() const { return matchName; }
unsigned Match::getStartPosition() const { return startPosition; }
unsigned Match::getLength() const { return length; }
