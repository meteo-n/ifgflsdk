﻿#include "stdafx.h"
#include "libgfl.h"
#include "susie.hpp"
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <limits>
#include "gfl.hpp"
//#include <shlwapi.h>
#define LWSTDAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
LWSTDAPI_(BOOL)     PathMatchSpecA(__in LPCSTR pszFile, __in LPCSTR pszSpec);
LWSTDAPI_(BOOL)     PathMatchSpecW(__in LPCWSTR pszFile, __in LPCWSTR pszSpec);

#pragma comment(lib,"shlwapi.lib")

constexpr inline SUSIE_RESULT GflErrorToSpiResult(GflError error) noexcept
{
	switch (error)
	{
	case GflError::NoError:
		return SUSIE_RESULT::ALL_RIGHT;
	case GflError::FileOpen: case GflError::FileRead:
		return SUSIE_RESULT::FILE_READ_ERROR;
	case GflError::FileCreate: case GflError::FileWrite:
		return SUSIE_RESULT::FILE_WRITE_ERROR;
	case GflError::NoMemory:
		return SUSIE_RESULT::MEMORY_ERROR;
	case GflError::UnknownFormat: case GflError::BadFormatIndex:
		return SUSIE_RESULT::NOT_SUPPORT;
	case GflError::BadBitmap:
		return SUSIE_RESULT::OUT_OF_ORDER;
	case GflError::BadParameters:
		return SUSIE_RESULT::OTHER_ERROR;
	case GflError::UnknownError: default:
		return SUSIE_RESULT::NO_FUNCTION;
	}
}

template <class F>
constexpr inline F bitwiseOr(F a, F b) noexcept
{
	return static_cast<F>(static_cast<typename std::underlying_type<F>::type>(a) | static_cast<typename std::underlying_type<F>::type>(b));
}

template <class F>
constexpr inline F bitwiseAnd(F a, F b) noexcept
{
	return static_cast<F>(static_cast<typename std::underlying_type<F>::type>(a) & static_cast<typename std::underlying_type<F>::type>(b));
}

int32_t WINAPI GetPluginInfo(int32_t infono, LPSTR  buf, int32_t buflen) noexcept
{
	try
	{
		GflLibraryInit init;

		switch (infono)
		{
		case 0:
			return (::strcpy_s(buf, buflen, "00IN")) == 0 ? static_cast<int>(std::strlen(buf)) : 0;
		case 1:
		{
			return (::strcpy_s(buf, buflen, ("ifgsldk: 0.1, GFL SDK: " + std::string(::gflGetVersion())).c_str())) == 0 ? static_cast<int>(std::strlen(buf)) : 0;
		}
		}

		auto index = infono / 2;
		auto isdescription = infono % 2;
		if (isdescription)
		{
			auto s = ::gflGetFormatDescriptionByIndex(index);
			if (s == nullptr) return 0;
			::strcpy_s(buf, buflen, s);
		}
		else
		{
			auto extName = ::gflGetDefaultFormatSuffixByIndex(index);
			if (extName == nullptr) return 0;
			::strcpy_s(buf, buflen, (std::string("*.") + extName).c_str());
		}
		return static_cast<int>(std::strlen(buf));
	}
	catch (const GflException & /* e */)
	{
		return 0;
	}
	catch (const std::exception &/* e */)
	{
		return 0;
	}
}
BOOL WINAPI IsSupported(LPCSTR filename, void* dw) noexcept
{
	try
	{
		GflLibraryInit init;
		{
			//BYTE buff[2048];
			if ((DWORD_PTR)dw & ~(DWORD_PTR)0xffff) { // 2K メモリイメージ
			}
			else { // ファイルハンドル
			}

			auto formats = GflFormats::getReadableFormats();
			for (auto& f : formats)
			{
				for (UINT i = 0; i < f.NumberOfExtension; i++)
				{
					if (::PathMatchSpecA(filename, (std::string("*.") + f.Extension[i]).c_str()))
						return TRUE;
				}
			}
			return FALSE;
		}
	}
	catch (const GflException &/*e*/)
	{
		return FALSE;
	}
	catch (const std::exception &/*e*/)
	{
		return FALSE;
	}
}
SUSIE_RESULT WINAPI GetPictureInfo(LPCSTR  buf, size_t len, SUSIE_FLAG flag, SUSIE_PICTUREINFO* lpInfo) noexcept
{
	try
	{
		GflLibraryInit init;
		{
			GflInfomation fileInfo;
			if (bitwiseAnd(flag, SUSIE_FLAG::SPI_INPUT_MASK) == SUSIE_FLAG::SPI_INPUT_MEMORY)
			{
				if (std::numeric_limits<decltype(len)>::max() > std::numeric_limits<GFL_UINT32>::max())
				{
					return SUSIE_RESULT::MEMORY_ERROR;
				}
				auto e = ::gflGetFileInformationFromMemory(reinterpret_cast<const BYTE*>(buf), static_cast<GFL_UINT32>(len), -1, &fileInfo);
				if (e != GFL_NO_ERROR)
					return GflErrorToSpiResult(static_cast<GflError>(e));
			}
			else if (bitwiseAnd(flag, SUSIE_FLAG::SPI_INPUT_MASK) == SUSIE_FLAG::SPI_INPUT_FILE)
			{
				auto e = ::gflGetFileInformation(buf, -1, &fileInfo);
				if (e != GFL_NO_ERROR)
					return GflErrorToSpiResult(static_cast<GflError>(e));
			}
			else
			{
				return SUSIE_RESULT::NO_FUNCTION;
			}
			*lpInfo = {};
			lpInfo->height = fileInfo.Height;
			lpInfo->width = fileInfo.Width;
			lpInfo->x_density = fileInfo.Xdpi;
			lpInfo->y_density = fileInfo.Ydpi;
			lpInfo->colorDepth = fileInfo.BitsPerComponent * fileInfo.ComponentsPerPixel;

			return SUSIE_RESULT::ALL_RIGHT;
		}
	}
	catch (const GflException &e)
	{
		return GflErrorToSpiResult(e.code());
	}
	catch (const std::exception &/*e*/)
	{
		return SUSIE_RESULT::NO_FUNCTION;
	}
}

SUSIE_RESULT WINAPI GetPicture(LPCSTR  buf, size_t len, SUSIE_FLAG flag, HLOCAL* pHBInfo, HLOCAL* pHBm, SUSIE_PROGRESS /* progressCallback */, intptr_t /* lData */) noexcept
{
	try
	{
		GflLibraryInit init;
		{
			GflLoadParam load_params;
			load_params.Origin = GFL_BOTTOM_LEFT;
			load_params.ColorModel = GFL_BGRA;

			GFL_BITMAP *ptr = nullptr;
			GflInfomation fileInfo;
			if (bitwiseAnd(flag, SUSIE_FLAG::SPI_INPUT_MASK) == SUSIE_FLAG::SPI_INPUT_MEMORY)
			{
				if (std::numeric_limits<decltype(len)>::max() > std::numeric_limits<GFL_UINT32>::max())
				{
					return SUSIE_RESULT::MEMORY_ERROR;
				}
				auto e = ::gflLoadBitmapFromMemory(reinterpret_cast<const BYTE*>(buf), static_cast<GFL_UINT32>(len), &ptr, &load_params, &fileInfo);
				if (e != GFL_NO_ERROR)
					return GflErrorToSpiResult(static_cast<GflError>(e));
			}
			else if (bitwiseAnd(flag, SUSIE_FLAG::SPI_INPUT_MASK) == SUSIE_FLAG::SPI_INPUT_FILE)
			{
				if (len != 0)
					return SUSIE_RESULT::NO_FUNCTION;

				auto e = ::gflLoadBitmap(buf, &ptr, &load_params, &fileInfo);
				if (e != GFL_NO_ERROR)
					return GflErrorToSpiResult(static_cast<GflError>(e));
			}
			else
			{
				return SUSIE_RESULT::NO_FUNCTION;
			}

			const GflBitmapPtr pBitmap(ptr);
			std::unique_ptr<BITMAPINFOHEADER, decltype(&::LocalFree)> pBInfo(
				static_cast<BITMAPINFOHEADER*>(::LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * pBitmap->ColorUsed)), &::LocalFree);

			if (!pBInfo)
				return SUSIE_RESULT::SPI_NO_MEMORY;

			switch (pBitmap->Type)
			{
			case GFL_GREY:
			case GFL_COLORS:
			case GFL_BGRA:
			case GFL_BGR:
				break;
			default:
				assert(false);
				return SUSIE_RESULT::NOT_SUPPORT;
			}

			*pBInfo = {
				sizeof(BITMAPINFOHEADER),
				fileInfo.Width,
				(pBitmap->Origin & GFL_BOTTOM ? 1 : -1) * fileInfo.Height,
				1,
				static_cast<WORD>(pBitmap->BitsPerComponent * pBitmap->ComponentsPerPixel),
				BI_RGB,
				pBitmap->BytesPerLine * fileInfo.Height,
				1,
				1,
				static_cast<DWORD>(pBitmap->ColorUsed),
			};

			RGBQUAD *colorTable = reinterpret_cast<RGBQUAD*>(pBInfo.get() + 1);

			if (pBitmap->ColorMap)
			{
				for (INT i = 0; i < pBitmap->ColorUsed; i++)
				{
					colorTable[i].rgbBlue = pBitmap->ColorMap->Blue[i];
					colorTable[i].rgbRed = pBitmap->ColorMap->Red[i];
					colorTable[i].rgbGreen = pBitmap->ColorMap->Green[i];
				}
			}
			else if (pBitmap->Type == GFL_GREY)
			{
				for (INT i = 0; i < pBitmap->ColorUsed; i++)
				{
					colorTable[i].rgbBlue = colorTable[i].rgbGreen = colorTable[i].rgbRed = static_cast<BYTE>(i);
				}
			}

			std::unique_ptr<void, decltype(&::LocalFree)> pBm(::LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, pBInfo->biSizeImage), &::LocalFree);
			if (!pBm)
				return SUSIE_RESULT::SPI_NO_MEMORY;

			std::memcpy(pBm.get(), pBitmap->Data, pBInfo->biSizeImage);

			*pHBInfo = pBInfo.release();
			*pHBm = pBm.release();

			return SUSIE_RESULT::ALL_RIGHT;
		}
	}
	catch (const GflException &e)
	{
		return GflErrorToSpiResult(e.code());
	}
	catch (const std::exception & /*e*/)
	{
		return SUSIE_RESULT::NO_FUNCTION;
	}
}

SUSIE_RESULT WINAPI GetPreview(LPCSTR  buf, size_t len, SUSIE_FLAG flag, HLOCAL* pHBInfo, HLOCAL* pHBm, SUSIE_PROGRESS progressCallback, intptr_t lData)
{
	UNREFERENCED_PARAMETER(buf);
	UNREFERENCED_PARAMETER(len);
	UNREFERENCED_PARAMETER(flag);
	UNREFERENCED_PARAMETER(pHBInfo);
	UNREFERENCED_PARAMETER(pHBm);
	UNREFERENCED_PARAMETER(progressCallback);
	UNREFERENCED_PARAMETER(lData);
	return SUSIE_RESULT::NO_FUNCTION;
}
