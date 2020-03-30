#pragma once

//-----------------------------------------------------------------------------
class ShaderHandler
{
public:
	virtual ~ShaderHandler() {}
	virtual void OnInit() = 0;
	virtual void OnRelease() = 0;
	virtual cstring GetName() const = 0;
};
