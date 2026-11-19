// Definitions of the shared objects declared in shared.hpp.
#include "shared.hpp"

namespace leg {

// Diagram: "command queue — k_msgq, depth 8".
K_MSGQ_DEFINE(cmd_queue, sizeof(CmdMsg), 8, 4);

SharedState g;

// Gate: main() releases the worker threads once HAL + watchdog are up.
K_SEM_DEFINE(init_done, 0, 4);

}  // namespace leg
