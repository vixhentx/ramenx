#include "logic.hpp"
#include "zephyr/logging/log.h"
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(app);

int main()
{
	LOG_INF("app started.");
	// async::demo(K_MSEC(200));
	logic::demo();
	LOG_INF("bye bye");
}
