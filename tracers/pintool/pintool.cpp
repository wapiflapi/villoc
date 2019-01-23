#include "pin.H"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#if defined(TARGET_MAC)
#define CALLOC "_calloc"
#define MALLOC "_malloc"
#define FREE "_free"
#define REALLOC "_realloc"
#else
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

ofstream TraceFile;

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
    args->size = size;
}

VOID AfterMalloc(ADDRINT ret)
{
    TraceFile << "malloc(" << args->size << ") = " << ADDRINTToHexString(ret) << endl;
}

VOID Free(ADDRINT addr)
{
    string formatted_addr = "";
    if(addr == 0){
        formatted_addr = "0";
    } else {
        formatted_addr = ADDRINTToHexString(addr);
    }
    TraceFile << "free(" + formatted_addr +") = <void>" << endl;
}

VOID BeforeCalloc(ADDRINT num, ADDRINT size)
{
    args->num = num;
    args->size = size;
}

VOID AfterCalloc(ADDRINT ret)
{
    TraceFile << "calloc(" << args->num << "," << ADDRINTToHexString(args->size) +") = " + ADDRINTToHexString(ret) << endl;
}

VOID BeforeRealloc(ADDRINT addr, ADDRINT size)
{
    args->addr = addr;
    args->size = size;
}

VOID AfterRealloc(ADDRINT ret)
{
    TraceFile << "realloc(" << ADDRINTToHexString(args->addr) << "," << args->size << ") = " << ADDRINTToHexString(ret) << endl;
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
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)BeforeMalloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)AfterMalloc,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(mallocRtn);
    }

    // Find the free() function.
    RTN freeRtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(freeRtn))
    {
        RTN_Open(freeRtn);
        // Instrument free() to print the input argument value.
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)Free,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);

        RTN_Close(freeRtn);
    }

    //Find the calloc() function
    RTN callocRtn = RTN_FindByName(img, CALLOC);
    if (RTN_Valid(callocRtn))
    {
        RTN_Open(callocRtn);

        // Instrument callocRtn to print the input argument value and the return value.
        RTN_InsertCall(callocRtn, IPOINT_BEFORE, (AFUNPTR)BeforeCalloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(callocRtn, IPOINT_AFTER, (AFUNPTR)AfterCalloc,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(callocRtn);
    }
    //Find the realloc() function
    RTN reallocRtn = RTN_FindByName(img, REALLOC);
    if (RTN_Valid(reallocRtn))
    {
        RTN_Open(reallocRtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(reallocRtn, IPOINT_BEFORE, (AFUNPTR)BeforeRealloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(reallocRtn, IPOINT_AFTER, (AFUNPTR)AfterRealloc,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(reallocRtn);
    }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
    TraceFile.close();
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
    TraceFile.open(KnobOutputFile.Value().c_str());
    // Write to a file since TraceFile and cerr maybe closed by the application
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
