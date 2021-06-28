#ifndef SCANCODE_AGENT_MATCH_HPP
#define SCANCODE_AGENT_MATCH_HPP

#include <string>

using namespace std;

class Match {
public:
  Match(char type, string matchName, unsigned startPosition, unsigned length);
  ~Match();
  const char getType() const;
  const string getMatchName() const;
  unsigned getStartPosition() const;
  unsigned getLength() const;

private:
  char type;
  string matchName;
  unsigned startPosition;
  unsigned length;
};

#endif