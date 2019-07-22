#pragma once

#include "Container.h"

class Panel : public Container
{
public:
	Panel() : Container(true), use_custom_color(false) {}

	void Draw(ControlDrawData* cdd = nullptr) override;

	Color custom_color;
	bool use_custom_color;
};
