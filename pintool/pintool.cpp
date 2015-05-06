#include "pin.H"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#if defined(TARGET_MAC)
#define MAIN "_main"
#define CALLOC "_calloc"
#define MALLOC "_malloc"
#define FREE "_free"
#define REALLOC "_realloc"
#else
#define MAIN "main"
#define CALLOC "calloc"
#define MALLOC "malloc"
#define FREE "free"
#define REALLOC "realloc"
#endif

using namespace std;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

class Args;

bool record = false;
ofstream trace_file;

Args* args = NULL;

string ADDRINTToHexString(ADDRINT a)
{
    ostringstream temp;
    temp << "0x" << hex <<a;
    return temp.str();
}

class Args
{
public:
    Args();
    ~Args();
    ADDRINT addr;
    ADDRINT num;
    ADDRINT size;
};

Args::Args()
{

}

Args::~Args()
{

}

/* ===================================================================== */
/* Analysis routines                                                     */
/* ===================================================================== */

VOID BeforeMalloc(ADDRINT size)
{
    if(!record) return;
    args->size = size;
}

VOID AfterMalloc(ADDRINT ret)
{
    if(!record) return;
    trace_file << "malloc(" << args->size << ") = " << ADDRINTToHexString(ret) << endl;
}

VOID Free(ADDRINT addr)
{
    if(!record) return;
    string formatted_addr = "";
    if(addr == 0){
        formatted_addr = "0";
    } else {
        formatted_addr = ADDRINTToHexString(addr);
    }
    trace_file << "free(" + formatted_addr +") = <void>" << endl;
}

VOID BeforeCalloc(ADDRINT num, ADDRINT size)
{
    if(!record) return;
    args->num = num;
    args->size = size;
}

VOID AfterCalloc(ADDRINT ret)
{
    if(!record) return;
    trace_file << "calloc(" << args->num << ", " << ADDRINTToHexString(args->size) +") = " + ADDRINTToHexString(ret) << endl;
}

VOID BeforeRealloc(ADDRINT addr, ADDRINT size)
{
    if(!record) return;
    args->addr = addr;
    args->size = size;
}

VOID AfterRealloc(ADDRINT ret)
{
    if(!record) return;
    trace_file << "realloc(" << ADDRINTToHexString(args->addr) << ", " << args->size << ") = " << ADDRINTToHexString(ret) << endl;
}

VOID RecordMainBegin() {
  record = true;
}
VOID RecordMainEnd() {
  record = false;
}

/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */
   
VOID Image(IMG img, VOID *v)
{
    // Instrument the malloc() and free() functions.  Print the input argument
    // of each malloc() or free(), and the return value of malloc().
    //
    //  Find the malloc() function.
    RTN malloc_rtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(malloc_rtn))
    {
        RTN_Open(malloc_rtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(malloc_rtn, IPOINT_BEFORE, (AFUNPTR)BeforeMalloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_InsertCall(malloc_rtn, IPOINT_AFTER, (AFUNPTR)AfterMalloc,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(malloc_rtn);
    }

    // Find the free() function.
    RTN free_rtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(free_rtn))
    {
        RTN_Open(free_rtn);
        // Instrument free() to print the input argument value.
        RTN_InsertCall(free_rtn, IPOINT_BEFORE, (AFUNPTR)Free,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);

        RTN_Close(free_rtn);
    }

    //Find the calloc() function
    RTN calloc_rtn = RTN_FindByName(img, CALLOC);
    if (RTN_Valid(calloc_rtn))
    {
        RTN_Open(calloc_rtn);

        // Instrument calloc_rtn to print the input argument value and the return value.
        RTN_InsertCall(calloc_rtn, IPOINT_BEFORE, (AFUNPTR)BeforeCalloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(calloc_rtn, IPOINT_AFTER, (AFUNPTR)AfterCalloc,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(calloc_rtn);
    }
    //Find the realloc() function
    RTN realloc_rtn = RTN_FindByName(img, REALLOC);
    if (RTN_Valid(realloc_rtn))
    {
        RTN_Open(realloc_rtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(realloc_rtn, IPOINT_BEFORE, (AFUNPTR)BeforeRealloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(realloc_rtn, IPOINT_AFTER, (AFUNPTR)AfterRealloc,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(realloc_rtn);
    }

    RTN main_rtn = RTN_FindByName(img, MAIN);
    if (main_rtn.is_valid()) {
        RTN_Open(main_rtn);
        RTN_InsertCall(main_rtn, IPOINT_BEFORE, (AFUNPTR)RecordMainBegin,
                       IARG_END);
        RTN_InsertCall(main_rtn, IPOINT_AFTER, (AFUNPTR)RecordMainEnd,
                       IARG_END);
        RTN_Close(main_rtn);
    } else {
        //if the binary stripped then record everything
        record = true;
    }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
    trace_file.close();
}

/* ===================================================================== */

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "trace", "specify trace file name");
/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    cerr << "This tool produces a visualisation is memory allocator activity." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    // Initialize pin & symbol manager
    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    trace_file.open(KnobOutputFile.Value().c_str());
    // Write to a file since trace_file and cerr maybe closed by the application
    Args* initial = new Args();
    args = initial;
    // Register Image to be called to instrument functions.

    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
