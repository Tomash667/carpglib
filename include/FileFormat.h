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

inline ImageFormat ToImageFormat(const string& str)
{
	if(str == "bmp")
		return ImageFormat::BMP;
	else if(str == "jpg")
		return ImageFormat::JPG;
	else if(str == "png")
		return ImageFormat::PNG;
	else if(str == "tif")
		return ImageFormat::TIF;
	else if(str == "gif")
		return ImageFormat::GIF;
	else if(str == "dds")
		return ImageFormat::DDS;
	else
		return ImageFormat::Invalid;
}
