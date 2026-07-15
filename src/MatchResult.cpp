#include "MatchResult.h"

String encodeMatchResult(const MatchResult &result) {
  String out = "MF|";

  out += result.version;
  out += "|";

  out += result.localPilotId;
  out += "|";

  out += result.remotePilotId;
  out += "|";

  out += result.result;
  out += "|";

  out += result.rounds;
  out += "|";

  out += result.localChassisId;
  out += "|";

  out += result.remoteChassisId;

  return out;
}