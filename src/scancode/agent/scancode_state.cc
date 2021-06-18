#include "scancode_state.hpp"

State::State(int agentId) :
  agentId(agentId)
{
}

int State::getAgentId() const
{
  return agentId;
};