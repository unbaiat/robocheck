
#ifndef RBC_PENALTY_H_
#define RBC_PENALTY_H_

#ifdef _WIN32
	#ifdef DLL_EXPORTS
		#define DLL_DECLSPEC __declspec(dllexport)
	#endif
	#ifndef DLL_EXPORTS
		#define DLL_DECLSPEC __declspec(dllimport)
	#endif
#else
	#define DLL_DECLSPEC
#endif

#include "rbc_api.h"

DLL_DECLSPEC int init_penalties(rbc_xml_doc );

DLL_DECLSPEC void free_penalties();

DLL_DECLSPEC struct rbc_out_info *
apply_penalty(enum EN_err_type , int );

#endif
