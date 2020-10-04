/**********************************
 * FILE NAME: stdincludes.h
 *
 * DESCRIPTION: standard header file
 **********************************/

#ifndef _STDINCLUDES_H_
#define _STDINCLUDES_H_

/*
 * Macros
 */
#define RING_SIZE 512
#define FAILURE -1
#define SUCCESS 0

/*
 * Standard Header files
 */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <execinfo.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <queue>
#include <fstream>

using namespace std;

#define STDCLLBKARGS (void *env, char *data, int size)
#define STDCLLBKRET	void
#define DEBUGLOG 1

//#define MsgDbgLog 1
//#define MsgCompressDbgLog 1
//#define MsgDecompressDbgLog 1
//#define MemberListDbgLog 1
//#define MemberAddDbgLog 1
//#define NodeAddLog 1

//#define MsgREQDbgLog 1
//#define MsgCompressREQDbgLog 1
//#define MsgDecompressREQDbgLog 1
//#define MemberListREQDbgLog 1
//#define NodeAddREQLog 1
//#define MemberAddREQDbgLog 1

//#define MsgREPDbgLog 1
//#define MsgCompressREPDbgLog 1
//#define MsgDecompressREPDbgLog 1
//#define MemberListREPDbgLog 1
//#define NodeAddREPLog 1
//#define MemberAddREPDbgLog 1

//#define MsgHBDbgLog 1
//#define MsgCompressHBDbgLog 1
//#define MsgDecompressHBDbgLog 1
//#define MemberListHBDbgLog 1
//#define NodeAddHBLog 1
//#define MemberAddHBDbgLog 1

#endif	/* _STDINCLUDES_H_ */
