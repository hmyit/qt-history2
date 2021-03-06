/*
 *  CWPluginErrors.h - CW_Result constants for plugin errors
 *
 *  Copyright (c) 1995-1997 Metrowerks, Inc.  All rights reserved.
 *
 */

#ifndef __CWPluginErrors_H__
#define __CWPluginErrors_H__

#ifdef __MWERKS__
#	pragma once
#endif

enum
{
	// common errors for all plugins
	
		cwNoErr,				/* successful return							*/
		cwErrUserCanceled,		/* operation canceled by user					*/
		cwErrRequestFailed,		/* generic failure when plugin fails			*/
		cwErrInvalidParameter,	/* one or more callback parameters invalid		*/
		cwErrInvalidCallback,	/* invalid given current request and plugin type*/
		cwErrInvalidMPCallback,	/* this request is not support from MP threads	*/
		cwErrOSError,			/* OS-specific, call CWGetCallbackOSError()		*/
		cwErrOutOfMemory,		/* not enough memory							*/
		cwErrFileNotFound,		/* file not found on disk						*/
		cwErrUnknownFile,		/* bad file number,  doesn't exist				*/
		cwErrSilent,	 		/* request failed but plugin didn't report any	*/
								/* errors and doesn't want IDE to report that	*/
								/* an unknown error occurred					*/
		cwErrCantSetAttribute,  /* plugin requested inapplicable file flags in 	*/
								/* CWAddProjectEntry							*/
		cwErrStringBufferOverflow,	/* an output string buffer was too small	*/
		cwErrDirectoryNotFound,	/* unable to find a directory being sought		*/
		cwErrLastCommonError = 512,	
		
	// compiler/linker errors
	
		cwErrUnknownSegment,	/* bad segment number, doesn't exist			*/
		cwErrSBMNotFound,		/* 												*/
		cwErrObjectFileNotStored,	/* No external object file has been stored	*/
		cwErrLicenseCheckFailed,/* license check failed, error reported by IDE  */
		cwErrFileSpecNotSpecified,	/* a file spec was unspecified				*/
		cwErrFileSpecInvalid,		/* a file spec was invalid					*/
		cwErrLastCompilerLinkerError = 1024

};

#endif	// __CWPluginErrors_H__
