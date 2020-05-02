#pragma once

//-----------------------------------------------------------------------------
enum class ImageFormat
{
	Invalid,
	BMP,
	JPG,
	PNG,
	TIF,
	GIF,
	DDS
};

//-----------------------------------------------------------------------------
namespace ImageFormatMethods
{
	ImageFormat FromString(const string& str);
	cstring GetExtension(ImageFormat format);
	const GUID& ToGuid(ImageFormat format);
}
