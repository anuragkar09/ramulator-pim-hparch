/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2015 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 * This test tool is intended to expose memory leaks in Pin when the code cache is flushed.
 */

#include "pin.H"
#include <iostream>

int numFlushes = 0;
USIZE memUsageFirst = 0;
USIZE memUsageLast = 0;

static VOID OnDoFlush()
{
    CODECACHE_FlushCache();
}

static VOID OnCacheFlush()
{
    numFlushes++;
    memUsageLast = PIN_MemoryAllocatedForPin();

    if (numFlushes == 1)
    {
        memUsageFirst = memUsageLast;
        cout << "[flush_leaks] First flush. Memory = " << memUsageLast << " bytes" << endl;
    }
    else if (numFlushes % 10 == 0)
    {
        cout << "[flush_leaks] Flush #" << numFlushes << ". Memory = " << memUsageLast << " bytes" << endl;
    }
}


static VOID InstrumentRoutine(RTN rtn, VOID *)
{
    if (RTN_Name(rtn) == "DoFlush")
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(OnDoFlush), IARG_END);
        RTN_Close(rtn);
    }
}

VOID Fini(INT32 code, VOID *v)
{
    if (numFlushes == 0)
    {
        cout << "[flush_leaks] No flushes!" << endl;
    }
    else
    {
        cout << "[flush_leaks] Last flush. Memory = " << memUsageLast << " bytes" << endl;

        if (memUsageLast > memUsageFirst + 1024*1024)
        {
            cout << "[flush_leaks] Memory leak = " << memUsageLast - memUsageFirst << " bytes" << endl;
            PIN_ExitProcess(1);
        }
        else
        {
            cout << "[flush_leaks] No memory leaks!" << endl;
        }
    }
}

int main(INT32 argc, CHAR **argv)
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);

    RTN_AddInstrumentFunction(InstrumentRoutine, 0);
    CODECACHE_AddCacheFlushedFunction(OnCacheFlush, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
