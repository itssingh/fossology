#ifndef SCANCODE_AGENT_MATCH_HPP
#define SCANCODE_AGENT_MATCH_HPP

#include <string>

using namespace std;

class Match {
public:
  Match(string type, string matchName, unsigned startPosition, unsigned length);
  ~Match();
  const string getType() const;
  const string getMatchName() const;
  unsigned getStartPosition() const;
  unsigned getLength() const;

private:
  string type;
  string matchName;
  unsigned startPosition;
  unsigned length;
};

#endif