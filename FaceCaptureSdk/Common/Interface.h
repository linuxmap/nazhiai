
#ifndef _INTERFACE_SPECIFICATION_HEADER_H
#define _INTERFACE_SPECIFICATION_HEADER_H

#ifdef INTERFACE_EXPORTS
#define INTERFACE_C_API extern "C" __declspec(dllexport)
#define INTERFACE_API extern __declspec(dllexport)
#define INTERFACE_CLASS __declspec(dllexport)
#else
#define INTERFACE_C_API extern "C" __declspec(dllimport)
#define INTERFACE_API extern __declspec(dllimport)
#define INTERFACE_CLASS __declspec(dllimport)
#endif

#endif

