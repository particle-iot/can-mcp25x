#include "mcp_25625.h"

MCP_CAN_25625::MCP_CAN_25625(pin_t cs, pin_t standby, SPIClass &spi, int speed)
	: MCP_CAN(cs, spi, speed),
	  standby(standby)
{
}

void MCP_CAN_25625::setStandby(bool value)
{
	if (value) {
		digitalWrite(standby, HIGH);
	} else {
		digitalWrite(standby, LOW);
	}
}

byte MCP_CAN_25625::sleep()
{
	setStandby(true);
	return MCP_CAN::sleep();
}

byte MCP_CAN_25625::wake()
{
	auto ret = MCP_CAN::wake();
	setStandby(false);
	return ret;
}
