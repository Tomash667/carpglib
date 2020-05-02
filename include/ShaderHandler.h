#pragma once

//-----------------------------------------------------------------------------
class ShaderHandler
{
public:
	virtual ~ShaderHandler() {}
	virtual cstring GetName() const = 0;
	virtual void OnInit() = 0;
	virtual void OnRelease() = 0;
};
