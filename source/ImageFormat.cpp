#include "Pch.h"
#include "ImageFormat.h"

#include <wincodec.h>

//=================================================================================================
ImageFormat ImageFormatMethods::FromString(const string& str)
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

//=================================================================================================
cstring ImageFormatMethods::GetExtension(ImageFormat format)
{
	switch(format)
	{
	default:
	case ImageFormat::Invalid:
		assert(0);
		return "";
	case ImageFormat::BMP:
		return "bmp";
	case ImageFormat::JPG:
		return "jpg";
	case ImageFormat::PNG:
		return "png";
	case ImageFormat::TIF:
		return "tif";
	case ImageFormat::GIF:
		return "gif";
	case ImageFormat::DDS:
		return "dds";
	}
}

//=================================================================================================
const GUID& ImageFormatMethods::ToGuid(ImageFormat format)
{
	switch(format)
	{
	case ImageFormat::BMP:
		return GUID_ContainerFormatBmp;
	case ImageFormat::JPG:
		return GUID_ContainerFormatJpeg;
	case ImageFormat::TIF:
		return GUID_ContainerFormatTiff;
	case ImageFormat::GIF:
		return GUID_ContainerFormatGif;
	case ImageFormat::PNG:
		return GUID_ContainerFormatPng;
	case ImageFormat::DDS:
		return GUID_ContainerFormatDds;
	default:
		assert(0);
		return GUID_ContainerFormatJpeg;
	}
}
