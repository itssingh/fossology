#ifndef SCANCODE_AGENT_STATE_HPP
#define SCANCODE_AGENT_STATE_HPP

#include "scancode_dbhandler.hpp"
#include "libfossdbmanagerclass.hpp"

using namespace std;

class State
{
public:
  State(int agentId);

  int getAgentId() const;

private:
  int agentId;
};

#endif