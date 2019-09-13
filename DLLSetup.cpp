// H264EncoderFilter.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"

#pragma warning(push)     // disable for this header only
#pragma warning(disable:4312) 
// DirectShow
#include <streams.h>
#pragma warning(pop)      // restore original warning level

#include "X264EncoderFilter.h"
#include "X264EncoderProperties.h"

//////////////////////////////////////////////////////////////////////////
//###############################  Standard Filter DLL Code ###############################
static const WCHAR g_wszName[] = L"CSIR X264 Encoder";   /// A name for the filter 

DEFINE_GUID(MEDIASUBTYPE_I420, 0x30323449, 0x0000, 0x0010, 0x80, 0x00,
  0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

const AMOVIESETUP_MEDIATYPE sudMediaTypes[] = 
{
  { 
		&MEDIATYPE_Video, &MEDIASUBTYPE_I420
	},
	{ 
		&MEDIATYPE_Video, &MEDIASUBTYPE_VPP_H264
	},
#if 0
  {
    &MEDIATYPE_Video, &MEDIASUBTYPE_H264
  },
  {
    &MEDIATYPE_Video, &MEDIASUBTYPE_AVC1
  },
#endif
};

const AMOVIESETUP_PIN sudPin[] = 
{
	//RG 01/04/trying different input type again
	//{
	//	L"",            // Obsolete, not used.
	//		FALSE,          // Is this pin rendered?
	//		FALSE,           // Is it an output pin?
	//		FALSE,          // Can the filter create zero instances?
	//		FALSE,          // Does the filter create multiple instances?
	//		&GUID_NULL,     // Obsolete.
	//		NULL,           // Obsolete.
	//		1,              // Number of media types.
	//		&sudMediaTypes[1]   // Pointer to media types.
	//},
	// RG Before testing with media type stream
	{
		L"",                // Obsolete, not used.
			FALSE,            // Is this pin rendered?
			FALSE,            // Is it an output pin?
			FALSE,            // Can the filter create zero instances?
			FALSE,            // Does the filter create multiple instances?
			&GUID_NULL,       // Obsolete.
			NULL,             // Obsolete.
			1,                // Number of media types.
			&sudMediaTypes[0] // Pointer to media types.
	},
	{
		L"",                // Obsolete, not used.
			FALSE,            // Is this pin rendered?
			TRUE,             // Is it an output pin?
			FALSE,            // Can the filter create zero instances?
			FALSE,            // Does the filter create multiple instances?
			&GUID_NULL,       // Obsolete.
			NULL,             // Obsolete.
			1,                // Number of media types.
			&sudMediaTypes[1] // Pointer to media types.
	}
};

// The next bunch of structures define information for the class factory.
AMOVIESETUP_FILTER FilterInfo =
{
	&CLSID_RTVC_X264Encoder, 	// CLSID
	g_wszName,					      // Name
	MERIT_DO_NOT_USE,			    // Merit
	2,							          // Number of AMOVIESETUP_PIN structs
  sudPin	  			          // Pin registration information.
};


CFactoryTemplate g_Templates[] = 
{
	{
		g_wszName,                          // Name
		&CLSID_RTVC_X264Encoder,            // CLSID
		X264EncoderFilter::CreateInstance,  // Method to create an instance of MyComponent
		NULL,                               // Initialization function
		&FilterInfo                         // Set-up information (for filters)
	},
  // This entry is for the property page.
  {
    L"X264 Properties",
    &CLSID_X264Properties,
    X264EncoderProperties::CreateInstance, 
    NULL, NULL
  }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);   

//////////////////////////////////////////////////////////////////////////
// Functions needed by the DLL, for registration.

STDAPI DllRegisterServer(void)
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

//DLL Entry point
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
