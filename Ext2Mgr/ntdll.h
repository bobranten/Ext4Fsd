#include <windows.h>

namespace NT {
    extern "C" {

/* PAGE_SIZE for X86/X64 */
#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 12L

/* Definitions for NTDDK STATUS */
typedef LONG    NTSTATUS;
typedef ULONG   DEVICE_TYPE;

#define NT_SUCCESS(Status) (((NT::NTSTATUS)(Status)) >= 0)

/* Common status codes */

#define STATUS_SUCCESS                   ((NT::NTSTATUS)0x00000000L)
#define STATUS_BUFFER_OVERFLOW           ((NT::NTSTATUS)0x80000005L)
#define STATUS_UNSUCCESSFUL              ((NT::NTSTATUS)0xC0000001L)
#define STATUS_NO_MEDIA_IN_DEVICE        ((NT::NTSTATUS)0xC0000013L)
#define STATUS_ACCESS_DENIED             ((NT::NTSTATUS)0xC0000022L)
#define STATUS_BUFFER_TOO_SMALL          ((NT::NTSTATUS)0xC0000023L)
#define STATUS_INSUFFICIENT_RESOURCES    ((NT::NTSTATUS)0xC000009AL)


typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } DUMMYUNIONNAME;

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;


/* Definitios of kernel I/O IRPs */

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_PNP_POWER                IRP_MJ_PNP      // Obsolete....
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b


/*
 * strings
 */

typedef struct _STRING {
    __maybevalid USHORT Length;
    __maybevalid USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength), length_is(Length) ]
#endif // MIDL_PASS
    __field_bcount_part_opt(MaximumLength, Length) PCHAR Buffer;
} STRING;
typedef STRING *PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef CONST STRING* PCOEM_STRING;

typedef struct _CSTRING {
    USHORT Length;
    USHORT MaximumLength;
    CONST char *Buffer;
} CSTRING;
typedef CSTRING *PCSTRING;
#define ANSI_NULL ((CHAR)0)     // winnt

typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    __field_bcount_part(MaximumLength, Length) PWCH   Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsDriverPathInformation, // 9
    FileFsVolumeFlagsInformation,// 10
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;


typedef struct _FILE_FS_DEVICE_INFORMATION {
    DEVICE_TYPE DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,              // 0        Y        N
    SystemProcessorInformation,          // 1        Y        N
    SystemPerformanceInformation,        // 2        Y        N
    SystemTimeOfDayInformation,          // 3        Y        N
    SystemNotImplemented1,               // 4        Y        N
    SystemProcessesAndThreadsInformation, // 5       Y        N
    SystemCallCounts,                    // 6        Y        N
    SystemConfigurationInformation,      // 7        Y        N
    SystemProcessorTimes,                // 8        Y        N
    SystemGlobalFlag,                    // 9        Y        Y
    SystemNotImplemented2,               // 10       Y        N
    SystemModuleInformation,             // 11       Y        N
    SystemLockInformation,               // 12       Y        N
    SystemNotImplemented3,               // 13       Y        N
    SystemNotImplemented4,               // 14       Y        N
    SystemNotImplemented5,               // 15       Y        N
    SystemHandleInformation,             // 16       Y        N
    SystemObjectInformation,             // 17       Y        N
    SystemPagefileInformation,           // 18       Y        N
    SystemInstructionEmulationCounts,    // 19       Y        N
    SystemInvalidInfoClass1,             // 20
    SystemCacheInformation,              // 21       Y        Y
    SystemPoolTagInformation,            // 22       Y        N
    SystemProcessorStatistics,           // 23       Y        N
    SystemDpcInformation,                // 24       Y        Y
    SystemNotImplemented6,               // 25       Y        N
    SystemLoadImage,                     // 26       N        Y
    SystemUnloadImage,                   // 27       N        Y
    SystemTimeAdjustment,                // 28       Y        Y
    SystemNotImplemented7,               // 29       Y        N
    SystemNotImplemented8,               // 30       Y        N
    SystemNotImplemented9,               // 31       Y        N
    SystemCrashDumpInformation,          // 32       Y        N
    SystemExceptionInformation,          // 33       Y        N
    SystemCrashDumpStateInformation,     // 34       Y        Y/N
    SystemKernelDebuggerInformation,     // 35       Y        N
    SystemContextSwitchInformation,      // 36       Y        N
    SystemRegistryQuotaInformation,      // 37       Y        Y
    SystemLoadAndCallImage,              // 38       N        Y
    SystemPrioritySeparation,            // 39       N        Y
    SystemNotImplemented10,              // 40       Y        N
    SystemNotImplemented11,              // 41       Y        N
    SystemInvalidInfoClass2,             // 42
    SystemInvalidInfoClass3,             // 43
    SystemTimeZoneInformation,           // 44       Y        N
    SystemLookasideInformation,          // 45       Y        N
    SystemSetTimeSlipEvent,              // 46       N        Y
    SystemCreateSession,                 // 47       N        Y
    SystemDeleteSession,                 // 48       N        Y
    SystemInvalidInfoClass4,             // 49
    SystemRangeStartInformation,         // 50       Y        N
    SystemVerifierInformation,           // 51       Y        Y
    SystemAddVerifier,                   // 52       N        Y
    SystemSessionProcessesInformation    // 53       Y        N
} SYSTEM_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength
    );

typedef struct _SYSTEM_BASIC_INFORMATION { // Information Class 0
    ULONG Unknown;
    ULONG MaximumIncrement;
    ULONG PhysicalPageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPage;
    ULONG HighestPhysicalPage;
    ULONG AllocationGranularity;
    ULONG LowestUserAddress;
    ULONG HighestUserAddress;
    ULONG ActiveProcessors;
    UCHAR NumberProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_INFORMATION { // Information Class 1
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    USHORT Unknown;
    ULONG FeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION { // Information Class 2
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    ULONG ReadOperationCount;
    ULONG WriteOperationCount;
    ULONG OtherOperationCount;
    ULONG AvailablePages;
    ULONG TotalCommittedPages;
    ULONG TotalCommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaults;
    ULONG WriteCopyFaults;
    ULONG TransistionFaults;
    ULONG Reserved1;
    ULONG DemandZeroFaults;
    ULONG PagesRead;
    ULONG PageReadIos;
    ULONG Reserved2[2];
    ULONG PagefilePagesWritten;
    ULONG PagefilePageWriteIos;
    ULONG MappedFilePagesWritten;
    ULONG MappedFilePageWriteIos;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG TotalFreeSystemPtes;
    ULONG SystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG SmallNonPagedLookasideListAllocateHits;
    ULONG SmallPagedLookasideListAllocateHits;
    ULONG Reserved3;
    ULONG MmSystemCachePage;
    ULONG PagedPoolPage;
    ULONG SystemDriverPage;
    ULONG FastReadNoWait;
    ULONG FastReadWait;
    ULONG FastReadResourceMiss;
    ULONG FastReadNotPossible;
    ULONG FastMdlReadNoWait;
    ULONG FastMdlReadWait;
    ULONG FastMdlReadResourceMiss;
    ULONG FastMdlReadNotPossible;
    ULONG MapDataNoWait;
    ULONG MapDataWait;
    ULONG MapDataNoWaitMiss;
    ULONG MapDataWaitMiss;
    ULONG PinMappedDataCount;
    ULONG PinReadNoWait;
    ULONG PinReadWait;
    ULONG PinReadNoWaitMiss;
    ULONG PinReadWaitMiss;
    ULONG CopyReadNoWait;
    ULONG CopyReadWait;
    ULONG CopyReadNoWaitMiss;
    ULONG CopyReadWaitMiss;
    ULONG MdlReadNoWait;
    ULONG MdlReadWait;
    ULONG MdlReadNoWaitMiss;
    ULONG MdlReadWaitMiss;
    ULONG ReadAheadIos;
    ULONG LazyWriteIos;
    ULONG LazyWritePages;
    ULONG DataFlushes;
    ULONG DataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_TIME_OF_DAY_INFORMATION { // Information Class 3
    LARGE_INTEGER BootTime;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER TimeZoneBias;
    ULONG CurrentTimeZoneId;
} SYSTEM_TIME_OF_DAY_INFORMATION, *PSYSTEM_TIME_OF_DAY_INFORMATION;

typedef struct _IO_COUNTERSEX {
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
} IO_COUNTERSEX, *PIO_COUNTERSEX;

typedef enum {
    StateInitialized,
    StateReady,
    StateRunning,
    StateStandby,
    StateTerminated,
    StateWait,
    StateTransition,
    StateUnknown
} THREAD_STATE;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

/* Thread priority */
typedef LONG KPRIORITY;

typedef enum _KWAIT_REASON {
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    WrKeyedEvent,
    WrTerminated,
    WrProcessInSwap,
    WrCpuRateControl,
    WrCalloutStack,
    WrKernel,
    WrResource,
    WrPushLock,
    WrMutex,
    WrQuantumEnd,
    WrDispatchInt,
    WrPreempted,
    WrYieldExecution,
    WrFastMutex,
    WrGuardedMutex,
    WrRundown,
    MaximumWaitReason
} KWAIT_REASON;

typedef struct _SYSTEM_THREADS {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
    ULONG ContextSwitchCount;
    THREAD_STATE State;
    KWAIT_REASON WaitReason;
} SYSTEM_THREADS, *PSYSTEM_THREADS;

typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivateUsage;
} VM_COUNTERS_EX;

typedef VM_COUNTERS_EX *PVM_COUNTERS_EX;
#define MAX_HW_COUNTERS 16

typedef struct _SYSTEM_PROCESSES { // Information Class 5
    ULONG NextEntryDelta;
    ULONG ThreadCount;
    ULONG Reserved1[6];
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ProcessName;
    KPRIORITY BasePriority;
    ULONG ProcessId;
    ULONG InheritedFromProcessId;
    ULONG HandleCount;
    ULONG Reserved2[2];
    VM_COUNTERS VmCounters;
    IO_COUNTERS IoCounters;
    SYSTEM_THREADS Threads[1];
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;

typedef struct _SYSTEM_CALLS_INFORMATION { // Information Class 6
    ULONG Size;
    ULONG NumberOfDescriptorTables;
    ULONG NumberOfRoutinesInTable[1];
    // ULONG CallCounts[];
} SYSTEM_CALLS_INFORMATION, *PSYSTEM_CALLS_INFORMATION;

typedef struct _SYSTEM_CONFIGURATION_INFORMATION { // Information Class 7
    ULONG DiskCount;
    ULONG FloppyCount;
    ULONG CdRomCount;
    ULONG TapeCount;
    ULONG SerialCount;
    ULONG ParallelCount;
} SYSTEM_CONFIGURATION_INFORMATION, *PSYSTEM_CONFIGURATION_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_TIMES { // Information Class 8
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_TIMES, *PSYSTEM_PROCESSOR_TIMES;

typedef struct _SYSTEM_GLOBAL_FLAG { // Information Class 9
    ULONG GlobalFlag;
} SYSTEM_GLOBAL_FLAG, *PSYSTEM_GLOBAL_FLAG;

typedef struct _SYSTEM_MODULE_INFORMATION { // Information Class 11
    ULONG Reserved[2];
    PVOID Base;
    ULONG Size;
    ULONG Flags;
    USHORT Index;
    USHORT Unknown;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    CHAR ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct _SYSTEM_LOCK_INFORMATION { // Information Class 12
    PVOID Address;
    USHORT Type;
    USHORT Reserved1;
    ULONG ExclusiveOwnerThreadId;
    ULONG ActiveCount;
    ULONG ContentionCount;
    ULONG Reserved2[2];
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
} SYSTEM_LOCK_INFORMATION, *PSYSTEM_LOCK_INFORMATION;

typedef struct _SYSTEM_HANDLE_INFORMATION { // Information Class 16
    ULONG ProcessId;
    UCHAR ObjectTypeNumber;
    UCHAR Flags;  // 0x01 = PROTECT_FROM_CLOSE, 0x02 = INHERIT
    USHORT Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

//
// Pool Allocation routines (in pool.c)
//

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType,

    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,
} POOL_TYPE;

typedef struct _SYSTEM_OBJECT_TYPE_INFORMATION { // Information Class 17
    ULONG NextEntryOffset;
    ULONG ObjectCount;
    ULONG HandleCount;
    ULONG TypeNumber;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ACCESS_MASK ValidAccessMask;
    POOL_TYPE PoolType;
    UCHAR Unknown;
    UNICODE_STRING Name;
} SYSTEM_OBJECT_TYPE_INFORMATION, *PSYSTEM_OBJECT_TYPE_INFORMATION;

typedef struct _SYSTEM_OBJECT_INFORMATION {
    ULONG NextEntryOffset;
    PVOID Object;
    ULONG CreatorProcessId;
    USHORT Unknown;
    USHORT Flags;
    ULONG PointerCount;
    ULONG HandleCount;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG ExclusiveProcessId;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    UNICODE_STRING Name;
} SYSTEM_OBJECT_INFORMATION, *PSYSTEM_OBJECT_INFORMATION;

typedef struct _SYSTEM_PAGEFILE_INFORMATION { // Information Class 18
    ULONG NextEntryOffset;
    ULONG CurrentSize;
    ULONG TotalUsed;
    ULONG PeakUsed;
    UNICODE_STRING FileName;
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

typedef struct _SYSTEM_INSTRUCTION_EMULATION_INFORMATION { // Info Class 19
    ULONG GenericInvalidOpcode;
    ULONG TwoByteOpcode;
    ULONG ESprefix;
    ULONG CSprefix;
    ULONG SSprefix;
    ULONG DSprefix;
    ULONG FSPrefix;
    ULONG GSprefix;
    ULONG OPER32prefix;
    ULONG ADDR32prefix;
    ULONG INSB;
    ULONG INSW;
    ULONG OUTSB;
    ULONG OUTSW;
    ULONG PUSHFD;
    ULONG POPFD;
    ULONG INTnn;
    ULONG INTO;
    ULONG IRETD;
    ULONG FloatingPointOpcode;
    ULONG INBimm;
    ULONG INWimm;
    ULONG OUTBimm;
    ULONG OUTWimm;
    ULONG INB;
    ULONG INW;
    ULONG OUTB;
    ULONG OUTW;
    ULONG LOCKprefix;
    ULONG REPNEprefix;
    ULONG REPprefix;
    ULONG CLI;
    ULONG STI;
    ULONG HLT;
} SYSTEM_INSTRUCTION_EMULATION_INFORMATION, *PSYSTEM_INSTRUCTION_EMULATION_INFORMATION;

typedef struct _SYSTEM_CACHE_INFORMATION { // Information Class 21
    ULONG SystemCacheWsSize;
    ULONG SystemCacheWsPeakSize;
    ULONG SystemCacheWsFaults;
    ULONG SystemCacheWsMinimum;
    ULONG SystemCacheWsMaximum;
    ULONG TransitionSharedPages;
    ULONG TransitionSharedPagesPeak;
    ULONG Reserved[2];
} SYSTEM_CACHE_INFORMATION, *PSYSTEM_CACHE_INFORMATION;

typedef struct _SYSTEM_POOL_TAG_INFORMATION { // Information Class 22
    CHAR Tag[4];
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG NonPagedPoolUsage;
} SYSTEM_POOL_TAG_INFORMATION, *PSYSTEM_POOL_TAG_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_STATISTICS { // Information Class 23
    ULONG ContextSwitches;
    ULONG DpcCount;
    ULONG DpcRequestRate;
    ULONG TimeIncrement;
    ULONG DpcBypassCount;
    ULONG ApcBypassCount;
} SYSTEM_PROCESSOR_STATISTICS, *PSYSTEM_PROCESSOR_STATISTICS;

typedef struct _SYSTEM_DPC_INFORMATION { // Information Class 24
    ULONG Reserved;
    ULONG MaximumDpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG AdjustDpcThreshold;
    ULONG IdealDpcRate;
} SYSTEM_DPC_INFORMATION, *PSYSTEM_DPC_INFORMATION;

typedef struct _SYSTEM_LOAD_IMAGE { // Information Class 26
    UNICODE_STRING ModuleName;
    PVOID ModuleBase;
    PVOID Unknown;
    PVOID EntryPoint;
    PVOID ExportDirectory;
} SYSTEM_LOAD_IMAGE, *PSYSTEM_LOAD_IMAGE;

typedef struct _SYSTEM_UNLOAD_IMAGE { // Information Class 27
    PVOID ModuleBase;
} SYSTEM_UNLOAD_IMAGE, *PSYSTEM_UNLOAD_IMAGE;

typedef struct _SYSTEM_QUERY_TIME_ADJUSTMENT { // Information Class 28
    ULONG TimeAdjustment;
    ULONG MaximumIncrement;
    BOOL TimeSynchronization;
} SYSTEM_QUERY_TIME_ADJUSTMENT, *PSYSTEM_QUERY_TIME_ADJUSTMENT;

typedef struct _SYSTEM_SET_TIME_ADJUSTMENT { // Information Class 28
    ULONG TimeAdjustment;
    BOOL TimeSynchronization;
} SYSTEM_SET_TIME_ADJUSTMENT, *PSYSTEM_SET_TIME_ADJUSTMENT;

typedef struct _SYSTEM_CRASH_DUMP_INFORMATION { // Information Class 32
    HANDLE CrashDumpSectionHandle;
    HANDLE Unknown;  // Windows 2000 only
} SYSTEM_CRASH_DUMP_INFORMATION, *PSYSTEM_CRASH_DUMP_INFORMATION;

typedef struct _SYSTEM_EXCEPTION_INFORMATION { // Information Class 33
    ULONG AlignmentFixupCount;
    ULONG ExceptionDispatchCount;
    ULONG FloatingEmulationCount;
    ULONG Reserved;
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_CRASH_STATE_INFORMATION { // Information Class 34
    ULONG ValidCrashDump;
    ULONG Unknown;  // Windows 2000 only
} SYSTEM_CRASH_STATE_INFORMATION, *PSYSTEM_CRASH_STATE_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION { // Information Class 35
    BOOL DebuggerEnabled;
    BOOL DebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef struct _SYSTEM_CONTEXT_SWITCH_INFORMATION { // Information Class 36
    ULONG ContextSwitches;
    ULONG ContextSwitchCounters[11];
} SYSTEM_CONTEXT_SWITCH_INFORMATION, *PSYSTEM_CONTEXT_SWITCH_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION { // Information Class 37
    ULONG RegistryQuota;
    ULONG RegistryQuotaInUse;
    ULONG PagedPoolSize;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE { // Information Class 38
    UNICODE_STRING ModuleName;
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;

typedef struct _SYSTEM_PRIORITY_SEPARATION { // Information Class 39
    ULONG PrioritySeparation;
} SYSTEM_PRIORITY_SEPARATION, *PSYSTEM_PRIORITY_SEPARATION;

typedef struct _SYSTEM_TIME_ZONE_INFORMATION { // Information Class 44
    LONG Bias;
    WCHAR StandardName[32];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} SYSTEM_TIME_ZONE_INFORMATION, *PSYSTEM_TIME_ZONE_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION { // Information Class 45
    USHORT Depth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    ULONG AllocateMisses;
    ULONG TotalFrees;
    ULONG FreeMisses;
    POOL_TYPE Type;
    ULONG Tag;
    ULONG Size;
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_SET_TIME_SLIP_EVENT { // Information Class 46
    HANDLE TimeSlipEvent;
} SYSTEM_SET_TIME_SLIP_EVENT, *PSYSTEM_SET_TIME_SLIP_EVENT;

typedef struct _SYSTEM_CREATE_SESSION { // Information Class 47
    ULONG Session;
} SYSTEM_CREATE_SESSION, *PSYSTEM_CREATE_SESSION;

typedef struct _SYSTEM_DELETE_SESSION { // Information Class 48
    ULONG Session;
} SYSTEM_DELETE_SESSION, *PSYSTEM_DELETE_SESSION;

typedef struct _SYSTEM_RANGE_START_INFORMATION { // Information Class 50
    PVOID SystemRangeStart;
} SYSTEM_RANGE_START_INFORMATION, *PSYSTEM_RANGE_START_INFORMATION;

typedef struct _SYSTEM_POOL_BLOCK {
    BOOL Allocated;
    USHORT Unknown;
    ULONG Size;
    CHAR Tag[4];
} SYSTEM_POOL_BLOCK, *PSYSTEM_POOL_BLOCK;

typedef struct _SYSTEM_POOL_BLOCKS_INFORMATION { // Info Classes 14 and 15
    ULONG PoolSize;
    PVOID PoolBase;
    USHORT Unknown;
    ULONG NumberOfBlocks;
    SYSTEM_POOL_BLOCK PoolBlocks[1];
} SYSTEM_POOL_BLOCKS_INFORMATION, *PSYSTEM_POOL_BLOCKS_INFORMATION;

typedef struct _SYSTEM_MEMORY_USAGE {
    PVOID Name;
    USHORT Valid;
    USHORT Standby;
    USHORT Modified;
    USHORT PageTables;
} SYSTEM_MEMORY_USAGE, *PSYSTEM_MEMORY_USAGE;

typedef struct _SYSTEM_MEMORY_USAGE_INFORMATION { // Info Classes 25 and 29
    ULONG Reserved;
    PVOID EndOfData;
    SYSTEM_MEMORY_USAGE MemoryUsage[1];
} SYSTEM_MEMORY_USAGE_INFORMATION, *PSYSTEM_MEMORY_USAGE_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PVOID Value,
    IN ULONG ValueLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    IN PUNICODE_STRING Value
    );

typedef enum _SHUTDOWN_ACTION {
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION;

NTSYSAPI
NTSTATUS
NTAPI
ZwShutdownSystem(
    IN SHUTDOWN_ACTION Action
    );

typedef enum _DEBUG_CONTROL_CODE {
    DebugGetTraceInformation = 1,
    DebugSetInternalBreakpoint,
    DebugSetSpecialCall,
    DebugClearSpecialCalls,
    DebugQuerySpecialCalls,
    DebugDbgBreakPoint
} DEBUG_CONTROL_CODE;

NTSYSAPI
NTSTATUS
NTAPI
ZwSystemDebugControl(
    IN DEBUG_CONTROL_CODE ControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,             // 0    Y       N
    ObjectNameInformation,              // 1    Y       N
    ObjectTypeInformation,              // 2    Y       N
    ObjectAllTypesInformation,          // 3    Y       N
    ObjectHandleInformation             // 4    Y       Y
} OBJECT_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG ObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG ObjectInformationLength
    );

typedef struct _OBJECT_BASIC_INFORMATION { // Information Class 0
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG Reserved[3];
    ULONG NameInformationLength;
    ULONG TypeInformationLength;
    ULONG SecurityDescriptorLength;
    LARGE_INTEGER CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_TYPE_INFORMATION { // Information Class 2
    UNICODE_STRING Name;
    ULONG ObjectCount;
    ULONG HandleCount;
    ULONG Reserved1[4];
    ULONG PeakObjectCount;
    ULONG PeakHandleCount;
    ULONG Reserved2[4];
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccess;
    UCHAR Unknown;
    BOOL MaintainHandleDatabase;
    UCHAR Reserved3[2];
    POOL_TYPE PoolType;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_ALL_TYPES_INFORMATION { // Information Class 3
    ULONG NumberOfTypes;
    OBJECT_TYPE_INFORMATION TypeInformation;
} OBJECT_ALL_TYPES_INFORMATION, *POBJECT_ALL_TYPES_INFORMATION;

typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION { // Information Class 4
    BOOL Inherit;
    BOOL ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

typedef struct _OBJECT_ATTRIBUTES64 {
    ULONG Length;
    ULONG64 RootDirectory;
    ULONG64 ObjectName;
    ULONG Attributes;
    ULONG64 SecurityDescriptor;
    ULONG64 SecurityQualityOfService;
} OBJECT_ATTRIBUTES64;
typedef OBJECT_ATTRIBUTES64 *POBJECT_ATTRIBUTES64;
typedef CONST OBJECT_ATTRIBUTES64 *PCOBJECT_ATTRIBUTES64;

typedef struct _OBJECT_ATTRIBUTES32 {
    ULONG Length;
    ULONG RootDirectory;
    ULONG ObjectName;
    ULONG Attributes;
    ULONG SecurityDescriptor;
    ULONG SecurityQualityOfService;
} OBJECT_ATTRIBUTES32;
typedef OBJECT_ATTRIBUTES32 *POBJECT_ATTRIBUTES32;
typedef CONST OBJECT_ATTRIBUTES32 *PCOBJECT_ATTRIBUTES32;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;


typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    ProcessIoPriority,
    ProcessExecuteFlags,
    ProcessTlsInformation,
    ProcessCookie,
    ProcessImageInformation,
    ProcessCycleTime,
    ProcessPagePriority,
    ProcessInstrumentationCallback,
    ProcessThreadStackAllocation,
    ProcessWorkingSetWatchEx,
    ProcessImageFileNameWin32,
    ProcessImageFileMapping,
    ProcessAffinityUpdateMode,
    ProcessMemoryAllocationMode,
    ProcessGroupInformation,
    ProcessTokenVirtualizationEnabled,
    ProcessConsoleHostProcess,
    ProcessWindowInformation,
    MaxProcessInfoClass             // MaxProcessInfoClass should always be the last enum
} PROCESSINFOCLASS;

//
// Thread Information Classes
//

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,   // Obsolete
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,          // Obsolete
    ThreadCSwitchPmu,
    ThreadWow64Context,
    ThreadGroupInformation,
    ThreadUmsInformation,      // UMS
    ThreadCounterProfiling,
    ThreadIdealProcessorEx,
    MaxThreadInfoClass
} THREADINFOCLASS;

#define THREAD_CSWITCH_PMU_DISABLE  FALSE
#define THREAD_CSWITCH_PMU_ENABLE   TRUE

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    OUT PHANDLE TargetHandle OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Attributes,
    IN ULONG Options
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
    IN HANDLE Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
    IN HANDLE Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    OUT PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOL ReturnSingleEntry,
    IN BOOL RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef struct _DIRECTORY_BASIC_INFORMATION {
    UNICODE_STRING ObjectName;
    UNICODE_STRING ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION, *PDIRECTORY_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING TargetName
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
    IN HANDLE SymbolicLinkHandle,
    IN OUT PUNICODE_STRING TargetName,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN OUT PULONG AllocationSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PULONG FreeSize,
    IN ULONG FreeType
    );

typedef enum _MEMORY_INFORMATION_CLASS {
    MemoryBasicInformation,
    MemoryWorkingSetList,
    MemorySectionName,
    MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
    OUT PVOID MemoryInformation,
    IN ULONG MemoryInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef struct _MEMORY_BASIC_INFORMATION { // Information Class 0
    PVOID BaseAddress;
    PVOID AllocationBase;
    ULONG AllocationProtect;
    ULONG RegionSize;
    ULONG State;
    ULONG Protect;
    ULONG Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

typedef struct _MEMORY_WORKING_SET_LIST { // Information Class 1
    ULONG NumberOfPages;
    ULONG WorkingSetList[1];
} MEMORY_WORKING_SET_LIST, *PMEMORY_WORKING_SET_LIST;

typedef struct _MEMORY_SECTION_NAME { // Information Class 2
    UNICODE_STRING SectionFileName;
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

NTSYSAPI
NTSTATUS
NTAPI
ZwLockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PULONG LockSize,
    IN ULONG LockType
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PULONG LockSize,
    IN ULONG LockType
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PULONG ProtectSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PULONG FlushSize,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN PULONG NumberOfPages,
    OUT PULONG PageFrameNumbers
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFreeUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN OUT PULONG NumberOfPages,
    IN PULONG PageFrameNumbers
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwMapUserPhysicalPages(
    IN PVOID BaseAddress,
    IN PULONG NumberOfPages,
    IN PULONG PageFrameNumbers
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwMapUserPhysicalPagesScatter(
    IN PVOID *BaseAddresses,
    IN PULONG NumberOfPages,
    IN PULONG PageFrameNumbers
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwGetWriteWatch(
    IN HANDLE ProcessHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG RegionSize,
    OUT PULONG Buffer,
    IN OUT PULONG BufferEntries,
    OUT PULONG Granularity
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwResetWriteWatch(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG RegionSize
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PLARGE_INTEGER SectionSize OPTIONAL,
    IN ULONG Protect,
    IN ULONG Attributes,
    IN HANDLE FileHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

typedef enum _SECTION_INFORMATION_CLASS {
    SectionBasicInformation,
    SectionImageInformation
} SECTION_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN ULONG SectionInformationLength,
    OUT PULONG ResultLength OPTIONAL
    );

typedef struct _SECTION_BASIC_INFORMATION { // Information Class 0
    PVOID BaseAddress;
    ULONG Attributes;
    LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

typedef struct _SECTION_IMAGE_INFORMATION { // Information Class 1
    PVOID EntryPoint;
    ULONG Unknown1;
    ULONG StackReserve;
    ULONG StackCommit;
    ULONG Subsystem;
    USHORT MinorSubsystemVersion;
    USHORT MajorSubsystemVersion;
    ULONG Unknown2;
    ULONG Characteristics;
    USHORT ImageNumber;
    BOOL Executable;
    UCHAR Unknown3;
    ULONG Unknown4[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwExtendSection(
    IN HANDLE SectionHandle,
    IN PLARGE_INTEGER SectionSize
    );
#if 0
NTSYSAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PULONG ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAreMappedFilesTheSame(
    IN PVOID Address1,
    IN PVOID Address2
    );

typedef struct _USER_STACK {
    PVOID FixedStackBase;
    PVOID FixedStackLimit;
    PVOID ExpandableStackBase;
    PVOID ExpandableStackLimit;
    PVOID ExpandableStackBottom;
} USER_STACK, *PUSER_STACK;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PUSER_STACK UserStack,
    IN BOOL CreateSuspended
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateThread(
    IN HANDLE ThreadHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
    );

typedef struct _THREAD_BASIC_INFORMATION { // Information Class 0
    NTSTATUS ExitStatus;
    PNT_TIB TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwSuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
    );

typedef
VOID
(KNORMAL_ROUTINE) (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );
typedef KNORMAL_ROUTINE *PKNORMAL_ROUTINE;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueueApcThread(
    IN HANDLE ThreadHandle,
    IN PKNORMAL_ROUTINE ApcRoutine,
    IN PVOID ApcContext OPTIONAL,
    IN PVOID Argument1 OPTIONAL,
    IN PVOID Argument2 OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwTestAlert(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread(
    IN HANDLE ThreadHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
    IN HANDLE PortHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE TargetThreadHandle,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateAnonymousToken(
    IN HANDLE ThreadHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE InheritFromProcessHandle,
    IN BOOL InheritHandles,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess(
    IN HANDLE ProcessHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
    );

typedef struct _PROCESS_PRIORITY_CLASS { // Information Class 18
    BOOL Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

typedef struct _PROCESS_PARAMETERS {
    ULONG AllocationSize;
    ULONG Size;
    ULONG Flags;
    ULONG Zero;
    LONG Console;
    ULONG ProcessGroup;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
    UNICODE_STRING CurrentDirectoryName;
    HANDLE CurrentDirectoryHandle;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImageFile;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG dwX;
    ULONG dwY;
    ULONG dwXSize;
    ULONG dwYSize;
    ULONG dwXCountChars;
    ULONG dwYCountChars;
    ULONG dwFillAttribute;
    ULONG dwFlags;
    ULONG wShowWindow;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING Desktop;
    UNICODE_STRING Reserved;
    UNICODE_STRING Reserved2;
} PROCESS_PARAMETERS, *PPROCESS_PARAMETERS;

NTSTATUS
NTAPI
RtlCreateProcessParameters(
    OUT PPROCESS_PARAMETERS *ProcessParameters,
    IN PUNICODE_STRING ImageFile,
    IN PUNICODE_STRING DllPath OPTIONAL,
    IN PUNICODE_STRING CurrentDirectory OPTIONAL,
    IN PUNICODE_STRING CommandLine OPTIONAL,
    IN ULONG CreationFlags,
    IN PUNICODE_STRING WindowTitle OPTIONAL,
    IN PUNICODE_STRING Desktop OPTIONAL,
    IN PUNICODE_STRING Reserved OPTIONAL,
    IN PUNICODE_STRING Reserved2 OPTIONAL
    );

NTSTATUS
NTAPI
RtlDestroyProcessParameters(
    IN PPROCESS_PARAMETERS ProcessParameters
    );

typedef struct _DEBUG_BUFFER {
    HANDLE SectionHandle;
    PVOID SectionBase;
    PVOID RemoteSectionBase;
    ULONG SectionBaseDelta;
    HANDLE EventPairHandle;
    ULONG Unknown[2];
    HANDLE RemoteThreadHandle;
    ULONG InfoClassMask;
    ULONG SizeOfInfo;
    ULONG AllocatedSize;
    ULONG SectionSize;
    PVOID ModuleInformation;
    PVOID BackTraceInformation;
    PVOID HeapInformation;
    PVOID LockInformation;
    PVOID Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;

#define PDI_MODULES      0x01
#define PDI_BACKTRACE	 0x02
#define PDI_HEAPS	 0x04
#define PDI_HEAP_TAGS	 0x08
#define PDI_HEAP_BLOCKS	 0x10
#define PDI_LOCKS	 0x20

typedef struct _DEBUG_MODULE_INFORMATION { // c.f. SYSTEM_MODULE_INFORMATION
    ULONG Reserved[2];
    ULONG Base;
    ULONG Size;
    ULONG Flags;
    USHORT Index;
    USHORT Unknown;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    CHAR ImageName[256];
} DEBUG_MODULE_INFORMATION, *PDEBUG_MODULE_INFORMATION;

typedef struct _DEBUG_HEAP_INFORMATION {
    ULONG Base;
    ULONG Flags;
    USHORT Granularity;
    USHORT Unknown;
    ULONG Allocated;
    ULONG Committed;
    ULONG TagCount;
    ULONG BlockCount;
    ULONG Reserved[7];
    PVOID Tags;
    PVOID Blocks;
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

typedef struct _DEBUG_LOCK_INFORMATION { // c.f. SYSTEM_LOCK_INFORMATION
    PVOID Address;
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    ULONG OwnerThreadId;
    ULONG ActiveCount;
    ULONG ContentionCount;
    ULONG EntryCount;
    ULONG RecursionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
} DEBUG_LOCK_INFORMATION, *PDEBUG_LOCK_INFORMATION;

PDEBUG_BUFFER
NTAPI
RtlCreateQueryDebugBuffer(
    IN ULONG Size,
    IN BOOL EventPair
    );

NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    IN ULONG ProcessId,
    IN ULONG DebugInfoClassMask,
    IN OUT PDEBUG_BUFFER DebugBuffer
    );

NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
    IN PDEBUG_BUFFER DebugBuffer
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateJobObject(
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenJobObject(
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateJobObject(
    IN HANDLE JobHandle,
    IN NTSTATUS ExitStatus
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
    IN HANDLE JobHandle,
    IN HANDLE ProcessHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobInformationClass,
    OUT PVOID JobInformation,
    IN ULONG JobInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobInformationClass,
    IN PVOID JobInformation,
    IN ULONG JobInformationLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateToken(
    OUT PHANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TOKEN_TYPE Type,
    IN PLUID AuthenticationId,
    IN PLARGE_INTEGER ExpirationTime,
    IN PTOKEN_USER User,
    IN PTOKEN_GROUPS Groups,
    IN PTOKEN_PRIVILEGES Privileges,
    IN PTOKEN_OWNER Owner,
    IN PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN PTOKEN_DEFAULT_DACL DefaultDacl,
    IN PTOKEN_SOURCE Source
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOL OpenAsSelf,
    OUT PHANDLE TokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOL EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFilterToken(
    IN HANDLE ExistingTokenHandle,
    IN ULONG Flags,
    IN PTOKEN_GROUPS SidsToDisable,
    IN PTOKEN_PRIVILEGES PrivilegesToDelete,
    IN PTOKEN_GROUPS SidsToRestricted,
    OUT PHANDLE NewTokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken(
    IN HANDLE TokenHandle,
    IN BOOL DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState,
    IN ULONG BufferLength,
    OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
    OUT PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustGroupsToken(
    IN HANDLE TokenHandle,
    IN BOOL ResetToDefault,
    IN PTOKEN_GROUPS NewState,
    IN ULONG BufferLength,
    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
    OUT PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    IN PVOID TokenInformation,
    IN ULONG TokenInformationLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOL Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSignalAndWaitForSingleObject(
    IN HANDLE HandleToSignal,
    IN HANDLE HandleToWait,
    IN BOOL Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

typedef enum _WAIT_TYPE {
    WaitAll,
    WaitAny
} WAIT_TYPE;

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects(
    IN ULONG HandleCount,
    IN PHANDLE Handles,
    IN WAIT_TYPE WaitType,
    IN BOOL Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

/* Win2000 DDK includes these declarations but without explicit calling convention specification

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TIMER_TYPE TimerType
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOL PreviousState OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimer(
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
    IN PVOID TimerContext,
    IN BOOL Resume,
    IN LONG Period,
    OUT PBOOL PreviousState OPTIONAL
    );

*/

typedef enum _TIMER_INFORMATION_CLASS {
    TimerBasicInformation
} TIMER_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG TimerInformationLength,
    OUT PULONG ResultLength OPTIONAL
    );

typedef struct _TIMER_BASIC_INFORMATION {
    LARGE_INTEGER TimeRemaining;
    BOOL SignalState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOL InitialState
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
    IN HANDLE EventHandle,
    OUT PULONG PreviousState OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent(
    IN HANDLE EventHandle,
    OUT PULONG PreviousState OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent(
    IN HANDLE EventHandle,
    OUT PULONG PreviousState OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent(
    IN HANDLE EventHandle
    );

typedef enum _EVENT_INFORMATION_CLASS {
    EventBasicInformation
} EVENT_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEvent(
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ResultLength OPTIONAL
    );

typedef struct _EVENT_BASIC_INFORMATION {
    EVENT_TYPE EventType;
    LONG SignalState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN LONG InitialCount,
    IN LONG MaximumCount
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseSemaphore(
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount OPTIONAL
    );

typedef enum _SEMAPHORE_INFORMATION_CLASS {
    SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySemaphore(
    IN HANDLE SemaphoreHandle,
    IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN ULONG SemaphoreInformationLength,
    OUT PULONG ResultLength OPTIONAL
    );

typedef struct _SEMAPHORE_BASIC_INFORMATION {
    LONG CurrentCount;
    LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOL InitialOwner
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseMutant(
    IN HANDLE MutantHandle,
    OUT PULONG PreviousState
    );

typedef enum _MUTANT_INFORMATION_CLASS {
    MutantBasicInformation
} MUTANT_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMutant(
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG MutantInformationLength,
    OUT PULONG ResultLength OPTIONAL
    );

typedef struct _MUTANT_BASIC_INFORMATION {
    LONG SignalState;
    BOOL Owned;
    BOOL Abandoned;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateIoCompletion(
    OUT PHANDLE IoCompletionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG NumberOfConcurrentThreads
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenIoCompletion(
    OUT PHANDLE IoCompletionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIoCompletion(
    IN HANDLE IoCompletionHandle,
    IN ULONG CompletionKey,
    IN ULONG CompletionValue,
    IN NTSTATUS Status,
    IN ULONG Information
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRemoveIoCompletion(
    IN HANDLE IoCompletionHandle,
    OUT PULONG CompletionKey,
    OUT PULONG CompletionValue,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

typedef enum _IO_COMPLETION_INFORMATION_CLASS {
    IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryIoCompletion(
    IN HANDLE IoCompletionHandle,
    IN IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
    OUT PVOID IoCompletionInformation,
    IN ULONG IoCompletionInformationLength,
    OUT PULONG ResultLength OPTIONAL
    );

typedef struct _IO_COMPLETION_BASIC_INFORMATION {
    LONG SignalState;
} IO_COMPLETION_BASIC_INFORMATION, *PIO_COMPLETION_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitLowEventPair(
    IN HANDLE EventPairHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitHighEventPair(
    IN HANDLE EventPairHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighEventPair(
    IN HANDLE EventPairHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowEventPair(
    IN HANDLE EventPairHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighEventPair(
    IN HANDLE EventPairHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime(
    IN PLARGE_INTEGER NewTime,
    OUT PLARGE_INTEGER OldTime OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
    OUT PLARGE_INTEGER PerformanceCount,
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOL Set,
    OUT PULONG ActualResolution
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimerResolution(
    OUT PULONG CoarsestResolution,
    OUT PULONG FinestResolution,
    OUT PULONG ActualResolution
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDelayExecution(
    IN BOOL Alertable,
    IN PLARGE_INTEGER Interval
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution(
    VOID
    );

NTSYSAPI
ULONG
NTAPI
ZwGetTickCount(
    VOID
    );

typedef enum _KPROFILE_SOURCE {
    ProfileTime,
    ProfileAlignmentFixup,
    ProfileTotalIssues,
    ProfilePipelineDry,
    ProfileLoadInstructions,
    ProfilePipelineFrozen,
    ProfileBranchInstructions,
    ProfileTotalNonissues,
    ProfileDcacheMisses,
    ProfileIcacheMisses,
    ProfileCacheMisses,
    ProfileBranchMispredictions,
    ProfileStoreInstructions,
    ProfileFpInstructions,
    ProfileIntegerInstructions,
    Profile2Issue,
    Profile3Issue,
    Profile4Issue,
    ProfileSpecialInstructions,
    ProfileTotalCycles,
    ProfileIcacheIssues,
    ProfileDcacheAccesses,
    ProfileMemoryBarrierCycles,
    ProfileLoadLinkedIssues,
    ProfileMaximum
} KPROFILE_SOURCE;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProfile(
    OUT PHANDLE ProfileHandle,
    IN HANDLE ProcessHandle,
    IN PVOID Base,
    IN ULONG Size,
    IN ULONG BucketShift,
    IN PULONG Buffer,
    IN ULONG BufferLength,
    IN KPROFILE_SOURCE Source,
    IN ULONG ProcessorMask
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIntervalProfile(
    IN ULONG Interval,
    IN KPROFILE_SOURCE Source
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryIntervalProfile(
    IN KPROFILE_SOURCE Source,
    OUT PULONG Interval
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwStartProfile(
    IN HANDLE ProfileHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwStopProfile(
    IN HANDLE ProfileHandle
    );

typedef struct _PORT_MESSAGE {
    USHORT DataSize;
    USHORT MessageSize;
    USHORT MessageType;
    USHORT VirtualRangesOffset;
    CLIENT_ID ClientId;
    ULONG MessageId;
    ULONG SectionSize;
    // UCHAR Data[];
} PORT_MESSAGE, *PPORT_MESSAGE;

typedef enum _LPC_TYPE {
    LPC_NEW_MESSAGE,           // A new message
    LPC_REQUEST,               // A request message
    LPC_REPLY,                 // A reply to a request message
    LPC_DATAGRAM,              //
    LPC_LOST_REPLY,            //
    LPC_PORT_CLOSED,           // Sent when port is deleted
    LPC_CLIENT_DIED,           // Messages to thread termination ports
    LPC_EXCEPTION,             // Messages to thread exception port
    LPC_DEBUG_EVENT,           // Messages to thread debug port
    LPC_ERROR_EVENT,           // Used by ZwRaiseHardError
    LPC_CONNECTION_REQUEST     // Used by ZwConnectPort
} LPC_TYPE;

typedef struct _PORT_SECTION_WRITE {
    ULONG Length;
    HANDLE SectionHandle;
    ULONG SectionOffset;
    ULONG ViewSize;
    PVOID ViewBase;
    PVOID TargetViewBase;
} PORT_SECTION_WRITE, *PPORT_SECTION_WRITE;

typedef struct _PORT_SECTION_READ {
    ULONG Length;
    ULONG ViewSize;
    ULONG ViewBase;
} PORT_SECTION_READ, *PPORT_SECTION_READ;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePort(
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxDataSize,
    IN ULONG MaxMessageSize,
    IN ULONG Reserved
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateWaitablePort(
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxDataSize,
    IN ULONG MaxMessageSize,
    IN ULONG Reserved
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwConnectPort(
    OUT PHANDLE PortHandle,
    IN PUNICODE_STRING PortName,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
    IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
    OUT PULONG MaxMessageSize OPTIONAL,
    IN OUT PVOID ConnectData OPTIONAL,
    IN OUT PULONG ConnectDataLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSecureConnectPort(
    OUT PHANDLE PortHandle,
    IN PUNICODE_STRING PortName,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
    IN PSID ServerSid OPTIONAL,
    IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
    OUT PULONG MaxMessageSize OPTIONAL,
    IN OUT PVOID ConnectData OPTIONAL,
    IN OUT PULONG ConnectDataLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwListenPort(
    IN HANDLE PortHandle,
    OUT PPORT_MESSAGE Message
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAcceptConnectPort(
    OUT PHANDLE PortHandle,
    IN ULONG PortIdentifier,
    IN PPORT_MESSAGE Message,
    IN BOOL Accept,
    IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
    IN OUT PPORT_SECTION_READ ReadSection OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCompleteConnectPort(
    IN HANDLE PortHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE RequestMessage
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE RequestMessage,
    OUT PPORT_MESSAGE ReplyMessage
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE ReplyMessage
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
    IN HANDLE PortHandle,
    IN OUT PPORT_MESSAGE ReplyMessage
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
    IN HANDLE PortHandle,
    OUT PULONG PortIdentifier OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE Message
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePortEx(
    IN HANDLE PortHandle,
    OUT PULONG PortIdentifier OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE Message,
    IN PLARGE_INTEGER Timeout
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReadRequestData(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message,
    IN ULONG Index,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteRequestData(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message,
    IN ULONG Index,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef enum _PORT_INFORMATION_CLASS {
    PortBasicInformation
} PORT_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationPort(
    IN HANDLE PortHandle,
    IN PORT_INFORMATION_CLASS PortInformationClass,
    OUT PVOID PortInformation,
    IN ULONG PortInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef struct _PORT_BASIC_INFORMATION {
} PORT_BASIC_INFORMATION, *PPORT_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message
    );

#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe



//
// Define access rights to files and directories
//

//
// The FILE_READ_DATA and FILE_WRITE_DATA constants are also defined in
// devioctl.h as FILE_READ_ACCESS and FILE_WRITE_ACCESS. The values for these
// constants *MUST* always be in sync.
// The values are redefined in devioctl.h because they must be available to
// both DOS and NT.
//

#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory

#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define FILE_ADD_FILE             ( 0x0002 )    // directory

#define FILE_APPEND_DATA          ( 0x0004 )    // file
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe


#define FILE_READ_EA              ( 0x0008 )    // file & directory

#define FILE_WRITE_EA             ( 0x0010 )    // file & directory

#define FILE_EXECUTE              ( 0x0020 )    // file
#define FILE_TRAVERSE             ( 0x0020 )    // directory

#define FILE_DELETE_CHILD         ( 0x0040 )    // directory

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all

#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   FILE_READ_DATA           |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_READ_EA             |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)




//
// Define share access rights to files and directories
//

#define FILE_SHARE_READ                 0x00000001  
#define FILE_SHARE_WRITE                0x00000002  
#define FILE_SHARE_DELETE               0x00000004  
#define FILE_SHARE_VALID_FLAGS          0x00000007

//
// Define the file attributes values
//
// Note:  0x00000008 is reserved for use for the old DOS VOLID (volume ID)
//        and is therefore not considered valid in NT.
//
// Note:  Note also that the order of these flags is set to allow both the
//        FAT and the Pinball File Systems to directly set the attributes
//        flags in attributes words without having to pick each flag out
//        individually.  The order of these flags should not be changed!
//

#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
//OLD DOS VOLID                             0x00000008

#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_DEVICE               0x00000040  
#define FILE_ATTRIBUTE_NORMAL               0x00000080  

#define FILE_ATTRIBUTE_TEMPORARY            0x00000100  
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800  

#define FILE_ATTRIBUTE_OFFLINE              0x00001000  
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000  

#define FILE_ATTRIBUTE_VIRTUAL              0x00010000  

#define FILE_ATTRIBUTE_VALID_FLAGS          0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS      0x000031a7

//
// Define the create disposition values
//

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

//
// Define the create/open option flags
//

#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_REMOTE_INSTANCE               0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#if (NTDDI_VERSION >= NTDDI_WIN7)
#define FILE_OPEN_REQUIRING_OPLOCK              0x00010000
#define FILE_DISALLOW_EXCLUSIVE                 0x00020000
#endif /* NTDDI_VERSION >= NTDDI_WIN7 */

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000


#define FILE_VALID_OPTION_FLAGS                 0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036

//
// Define the I/O status information return values for NtCreateFile/NtOpenFile
//

#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

//
// Define special ByteOffset parameters for read and write operations
//

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff
#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe

//
// Define alignment requirement values
//

#define FILE_BYTE_ALIGNMENT             0x00000000
#define FILE_WORD_ALIGNMENT             0x00000001
#define FILE_LONG_ALIGNMENT             0x00000003
#define FILE_QUAD_ALIGNMENT             0x00000007
#define FILE_OCTA_ALIGNMENT             0x0000000f
#define FILE_32_BYTE_ALIGNMENT          0x0000001f
#define FILE_64_BYTE_ALIGNMENT          0x0000003f
#define FILE_128_BYTE_ALIGNMENT         0x0000007f
#define FILE_256_BYTE_ALIGNMENT         0x000000ff
#define FILE_512_BYTE_ALIGNMENT         0x000001ff

//
// Define the maximum length of a filename string
//

#define MAXIMUM_FILENAME_LENGTH         256

//
// Define the various device characteristics flags
//

#define FILE_REMOVABLE_MEDIA                    0x00000001
#define FILE_READ_ONLY_DEVICE                   0x00000002
#define FILE_FLOPPY_DISKETTE                    0x00000004
#define FILE_WRITE_ONCE_MEDIA                   0x00000008
#define FILE_REMOTE_DEVICE                      0x00000010
#define FILE_DEVICE_IS_MOUNTED                  0x00000020
#define FILE_VIRTUAL_VOLUME                     0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME          0x00000080
#define FILE_DEVICE_SECURE_OPEN                 0x00000100
#define FILE_CHARACTERISTIC_PNP_DEVICE          0x00000800
#define FILE_CHARACTERISTIC_TS_DEVICE           0x00001000
#define FILE_CHARACTERISTIC_WEBDAV_DEVICE       0x00002000


NTSYSAPI
NTSTATUS
NTAPI
ZwCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelIoFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    __in PVOID ApcContext,
    __in PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG Reserved
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFileScatter(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFileGather(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwLockFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PULARGE_INTEGER LockOffset,
    IN PULARGE_INTEGER LockLength,
    IN ULONG Key,
    IN BOOL FailImmediately,
    IN BOOL ExclusiveLock
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PULARGE_INTEGER LockOffset,
    IN PULARGE_INTEGER LockLength,
    IN ULONG Key
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PFILE_NOTIFY_INFORMATION Buffer,
    IN ULONG BufferLength,
    IN ULONG NotifyFilter,
    IN BOOL WatchSubtree
    );

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOL DeletePending;
    BOOL Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;


typedef struct _FILE_POSITION_INFORMATION {
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;


typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_GET_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR EaNameLength;
    CHAR EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PFILE_FULL_EA_INFORMATION Buffer,
    IN ULONG BufferLength,
    IN BOOL ReturnSingleEntry,
    IN PFILE_GET_EA_INFORMATION EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PULONG EaIndex OPTIONAL,
    IN BOOL RestartScan
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_FULL_EA_INFORMATION Buffer,
    IN ULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateNamedPipeFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN BOOL TypeMessage,
    IN BOOL ReadmodeMessage,
    IN BOOL Nonblocking,
    IN ULONG MaxInstances,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PLARGE_INTEGER DefaultTimeout OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateMailslotFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CreateOptions,
    IN ULONG Unknown,
    IN ULONG MaxMessageSize,
    IN PLARGE_INTEGER ReadTimeout OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID VolumeInformation,
    IN ULONG VolumeInformationLength,
    IN FS_INFORMATION_CLASS VolumeInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN FS_INFORMATION_CLASS VolumeInformationClass
    );

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    UCHAR Unknown;
    WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_LABEL_INFORMATION {
    ULONG VolumeLabelLength;
    WCHAR VolumeLabel;
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG FileSystemFlags;
    ULONG MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

typedef struct _FILE_FS_CONTROL_INFORMATION {
    LARGE_INTEGER Reserved[3];
    LARGE_INTEGER DefaultQuotaThreshold;
    LARGE_INTEGER DefaultQuotaLimit;
    ULONG QuotaFlags;
} FILE_FS_CONTROL_INFORMATION, *PFILE_FS_CONTROL_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
    LARGE_INTEGER TotalQuotaAllocationUnits;
    LARGE_INTEGER AvailableQuotaAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

typedef struct _FILE_FS_OBJECT_ID_INFORMATION {
    UUID VolumeObjectId;
    ULONG VolumeObjectIdExtendedInfo[12];
} FILE_FS_OBJECT_ID_INFORMATION, *PFILE_FS_OBJECT_ID_INFORMATION;

typedef struct _FILE_USER_QUOTA_INFORMATION {
    ULONG NextEntryOffset;
    ULONG SidLength;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER QuotaUsed;
    LARGE_INTEGER QuotaThreshold;
    LARGE_INTEGER QuotaLimit;
    SID Sid[1];
} FILE_USER_QUOTA_INFORMATION, *PFILE_USER_QUOTA_INFORMATION;

typedef struct _FILE_QUOTA_LIST_INFORMATION {
    ULONG NextEntryOffset;
    ULONG SidLength;
    SID Sid[1];
} FILE_QUOTA_LIST_INFORMATION, *PFILE_QUOTA_LIST_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PFILE_USER_QUOTA_INFORMATION Buffer,
    IN ULONG BufferLength,
    IN BOOL ReturnSingleEntry,
    IN PFILE_QUOTA_LIST_INFORMATION QuotaList OPTIONAL,
    IN ULONG QuotaListLength,
    IN PSID ResumeSid OPTIONAL,
    IN BOOL RestartScan
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_USER_QUOTA_INFORMATION Buffer,
    IN ULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation         = 1,
    FileFullDirectoryInformation,   // 2
    FileBothDirectoryInformation,   // 3
    FileBasicInformation,           // 4
    FileStandardInformation,        // 5
    FileInternalInformation,        // 6
    FileEaInformation,              // 7
    FileAccessInformation,          // 8
    FileNameInformation,            // 9
    FileRenameInformation,          // 10
    FileLinkInformation,            // 11
    FileNamesInformation,           // 12
    FileDispositionInformation,     // 13
    FilePositionInformation,        // 14
    FileFullEaInformation,          // 15
    FileModeInformation,            // 16
    FileAlignmentInformation,       // 17
    FileAllInformation,             // 18
    FileAllocationInformation,      // 19
    FileEndOfFileInformation,       // 20
    FileAlternateNameInformation,   // 21
    FileStreamInformation,          // 22
    FilePipeInformation,            // 23
    FilePipeLocalInformation,       // 24
    FilePipeRemoteInformation,      // 25
    FileMailslotQueryInformation,   // 26
    FileMailslotSetInformation,     // 27
    FileCompressionInformation,     // 28
    FileObjectIdInformation,        // 29
    FileCompletionInformation,      // 30
    FileMoveClusterInformation,     // 31
    FileQuotaInformation,           // 32
    FileReparsePointInformation,    // 33
    FileNetworkOpenInformation,     // 34
    FileAttributeTagInformation,    // 35
    FileTrackingInformation,        // 36
    FileIdBothDirectoryInformation, // 37
    FileIdFullDirectoryInformation, // 38
    FileValidDataLengthInformation, // 39
    FileShortNameInformation,       // 40
    FileIoCompletionNotificationInformation, // 41
    FileIoStatusBlockRangeInformation,       // 42
    FileIoPriorityHintInformation,           // 43
    FileSfioReserveInformation,              // 44
    FileSfioVolumeInformation,               // 45
    FileHardLinkInformation,                 // 46
    FileProcessIdsUsingFileInformation,      // 47
    FileNormalizedNameInformation,           // 48
    FileNetworkPhysicalNameInformation,      // 49
    FileIdGlobalTxDirectoryInformation,      // 50
    FileIsRemoteDeviceInformation,           // 51
    FileAttributeCacheInformation,           // 52
    FileNumaNodeInformation,                 // 53
    FileStandardLinkInformation,             // 54
    FileRemoteProtocolInformation,           // 55
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG FileInformationLength,
    IN FILE_INFORMATION_CLASS FileInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG FileInformationLength,
    IN FILE_INFORMATION_CLASS FileInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG FileInformationLength,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOL ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOL RestartScan
    );

typedef struct _FILE_DIRECTORY_INFORMATION { // Information Class 1
    ULONG NextEntryOffset;
    ULONG Unknown;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION { // Information Class 2
    ULONG NextEntryOffset;
    ULONG Unknown;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaInformationLength;
    WCHAR FileName[1];
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION;

typedef struct _FILE_BOTH_DIRECTORY_INFORMATION { // Information Class 3
    ULONG NextEntryOffset;
    ULONG Unknown;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaInformationLength;
    UCHAR AlternateNameLength;
    WCHAR AlternateName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION;

typedef struct _FILE_INTERNAL_INFORMATION { // Information Class 6
    LARGE_INTEGER FileId;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION { // Information Class 7
    ULONG EaInformationLength;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION { // Information Class 8
    ACCESS_MASK GrantedAccess;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_NAME_INFORMATION { // Information Classes 9 and 21
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION,
  FILE_ALTERNATE_NAME_INFORMATION, *PFILE_ALTERNATE_NAME_INFORMATION;

typedef struct _FILE_LINK_RENAME_INFORMATION { // Info Classes 10 and 11
    BOOL ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION,
  FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION { // Information Class 12
    ULONG NextEntryOffset;
    ULONG Unknown;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_MODE_INFORMATION { // Information Class 16
    ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_ALL_INFORMATION { // Information Class 18
    FILE_BASIC_INFORMATION BasicInformation;
    FILE_STANDARD_INFORMATION StandardInformation;
    FILE_INTERNAL_INFORMATION InternalInformation;
    FILE_EA_INFORMATION EaInformation;
    FILE_ACCESS_INFORMATION AccessInformation;
    FILE_POSITION_INFORMATION PositionInformation;
    FILE_MODE_INFORMATION ModeInformation;
    FILE_ALIGNMENT_INFORMATION AlignmentInformation;
    FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION { // Information Class 19
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION { // Information Class 22
    ULONG NextEntryOffset;
    ULONG StreamNameLength;
    LARGE_INTEGER EndOfStream;
    LARGE_INTEGER AllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_PIPE_INFORMATION { // Information Class 23
    ULONG ReadModeMessage;
    ULONG WaitModeBlocking;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION { // Information Class 24
    ULONG MessageType;
    ULONG Unknown1;
    ULONG MaxInstances;
    ULONG CurInstances;
    ULONG InBufferSize;
    ULONG Unknown2;
    ULONG OutBufferSize;
    ULONG Unknown3[2];
    ULONG ServerEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_PIPE_REMOTE_INFORMATION { // Information Class 25
    LARGE_INTEGER CollectDataTimeout;
    ULONG MaxCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION { // Information Class 26
    ULONG MaxMessageSize;
    ULONG Unknown;
    ULONG NextSize;
    ULONG MessageCount;
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION { // Information Class 27
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION { // Information Class 28
    LARGE_INTEGER CompressedSize;
    USHORT CompressionFormat;
    UCHAR CompressionUnitShift;
    UCHAR Unknown;
    UCHAR ClusterSizeShift;
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_COMPLETION_INFORMATION { // Information Class 30
    HANDLE IoCompletionHandle;
    ULONG CompletionKey;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
    IN HANDLE KeyHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
    IN HANDLE KeyHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveMergedKeys(
    IN HANDLE KeyHandle1,
    IN HANDLE KeyHandle2,
    IN HANDLE FileHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey(
    IN POBJECT_ATTRIBUTES NewFileObjectAttributes,
    IN HANDLE KeyHandle,
    IN POBJECT_ATTRIBUTES OldFileObjectAttributes
    );

typedef struct _KEY_SET_VIRTUALIZATION_INFORMATION {
    ULONG   VirtualTarget           : 1; // Tells if the key is a virtual target key. 
    ULONG   VirtualStore	        : 1; // Tells if the key is a virtual store key.
    ULONG   VirtualSource           : 1; // Tells if the key has been virtualized at least one (virtual hint)
    ULONG   Reserved                : 29;   
} KEY_SET_VIRTUALIZATION_INFORMATION, *PKEY_SET_VIRTUALIZATION_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS {
    KeyWriteTimeInformation,
    KeyWow64FlagsInformation,
    KeyControlFlagsInformation,
    KeySetVirtualizationInformation,
    KeySetDebugInformation,
    KeySetHandleTagsInformation,
    MaxKeySetInfoClass  // MaxKeySetInfoClass should always be the last enum
} KEY_SET_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
    );

typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    MaxKeyInfoClass  // MaxKeyInfoClass should always be the last enum
} KEY_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG KeyInformationLength,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG KeyInformationLength,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE EventHandle OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG NotifyFilter,
    IN BOOL WatchSubtree,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOL Asynchronous
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeMultipleKeys (
    IN HANDLE KeyHandle,
    IN ULONG Flags,
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN HANDLE EventHandle OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG NotifyFilter,
    IN BOOL WatchSubtree,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOL Asynchronous
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    );

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    MaxKeyValueInfoClass  // MaxKeyValueInfoClass should always be the last enum
} KEY_VALUE_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG KeyValueInformationLength,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG KeyValueInformationLength,
    OUT PULONG ResultLength
    );

typedef struct _KEY_VALUE_ENTRY {
    PUNICODE_STRING ValueName;
    ULONG           DataLength;
    ULONG           DataOffset;
    ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN OUT PKEY_VALUE_ENTRY ValueList,
    IN ULONG NumberOfValues,
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    OUT PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeCheck(
    IN HANDLE TokenHandle,
    IN PPRIVILEGE_SET RequiredPrivileges,
    OUT PBOOL Result
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOL AccessGranted
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegedServiceAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE TokenHandle,
    IN PPRIVILEGE_SET Privileges,
    IN BOOL AccessGranted
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheck(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    IN PPRIVILEGE_SET PrivilegeSet,
    IN PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PBOOL AccessStatus
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation,
    OUT PACCESS_MASK GrantedAccess,
    OUT PBOOL AccessStatus,
    OUT PBOOL GenerateOnClose
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByType(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE TokenHandle,
    IN ULONG DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN PPRIVILEGE_SET PrivilegeSet,
    IN PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PULONG AccessStatus
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeAndAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN ACCESS_MASK DesiredAccess,
    IN AUDIT_EVENT_TYPE AuditType,
    IN ULONG Flags,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation,
    OUT PACCESS_MASK GrantedAccess,
    OUT PULONG AccessStatus,
    OUT PBOOL GenerateOnClose
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultList(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN PPRIVILEGE_SET PrivilegeSet,
    IN PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccessList,
    OUT PULONG AccessStatusList
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultListAndAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN ACCESS_MASK DesiredAccess,
    IN AUDIT_EVENT_TYPE AuditType,
    IN ULONG Flags,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation,
    OUT PACCESS_MASK GrantedAccessList,
    OUT PULONG AccessStatusList,
    OUT PULONG GenerateOnClose
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE TokenHandle,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN ACCESS_MASK DesiredAccess,
    IN AUDIT_EVENT_TYPE AuditType,
    IN ULONG Flags,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation, 
    OUT PACCESS_MASK GrantedAccessList,
    OUT PULONG AccessStatusList,
    OUT PULONG GenerateOnClose
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID *HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOL ObjectCreation,
    IN BOOL AccessGranted,
    OUT PBOOL GenerateOnClose
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOL GenerateOnClose
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOL GenerateOnClose
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestWakeupLatency(
    IN LATENCY_TIME Latency
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestDeviceWakeup(
    IN HANDLE DeviceHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelDeviceWakeupRequest(
    IN HANDLE DeviceHandle
    );

NTSYSAPI
BOOL
NTAPI
ZwIsSystemResumeAutomatic(
    VOID
    );

typedef EXECUTION_STATE *PEXECUTION_STATE;

NTSYSAPI
NTSTATUS
NTAPI
ZwSetThreadExecutionState(
    IN EXECUTION_STATE ExecutionState,
    OUT PEXECUTION_STATE PreviousExecutionState
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwGetDevicePowerState(
    IN HANDLE DeviceHandle,
    OUT PDEVICE_POWER_STATE DevicePowerState
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemPowerState(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,
    IN BOOL Asynchronous
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    IN POWER_INFORMATION_LEVEL PowerInformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwGetPlugPlayEvent(
    IN ULONG Reserved1,
    IN ULONG Reserved2,
    OUT PVOID Buffer,
    IN ULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOL SearchFrames
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwContinue(
    IN PCONTEXT Context,
    IN BOOL TestAlert
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwW32Call(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID *Result OPTIONAL,
    OUT PULONG ResultLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCallbackReturn(
    IN PVOID Result OPTIONAL,
    IN ULONG ResultLength,
    IN NTSTATUS Status
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighThread(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowThread(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress OPTIONAL,
    IN ULONG FlushSize
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushWriteBuffer(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
    IN BOOL ThreadOrSystem,
    OUT PLCID Locale
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale(
    IN BOOL ThreadOrSystem,
    IN LCID Locale
    );

typedef LANGID *PLANGID;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultUILanguage(
    OUT PLANGID LanguageId
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage(
    IN LANGID LanguageId
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInstallUILanguage(
    OUT PLANGID LanguageId
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
    OUT PLUID Luid
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateUuids(
    OUT PLARGE_INTEGER UuidLastTimeAllocated,
    OUT PULONG UuidDeltaTime,
    OUT PULONG UuidSequenceNumber,
    OUT PUCHAR UuidSeed
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetUuidSeed(
    IN PUCHAR UuidSeed
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseHardError(
    IN NTSTATUS Status,
    IN ULONG NumberOfArguments,
    IN ULONG StringArgumentsMask,
    IN PULONG Arguments,
    IN ULONG MessageBoxType,
    OUT PULONG MessageBoxResult
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultHardErrorPort(
    IN HANDLE PortHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
    IN PUNICODE_STRING String
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePagingFile(
    IN PUNICODE_STRING FileName,
    IN PULARGE_INTEGER InitialSize,
    IN PULARGE_INTEGER MaximumSize,
    IN ULONG Reserved
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAddAtom(
    IN PWSTR String,
    IN ULONG StringLength,
    OUT PUSHORT Atom
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFindAtom(
    IN PWSTR String,
    IN ULONG StringLength,
    OUT PUSHORT Atom
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteAtom(
    IN USHORT Atom
    );

typedef enum _ATOM_INFORMATION_CLASS {
    AtomBasicInformation,
    AtomListInformation
} ATOM_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationAtom(
    IN USHORT Atom,
    IN ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef struct _ATOM_BASIC_INFORMATION {
    USHORT ReferenceCount;
    USHORT Pinned;
    USHORT NameLength;
    WCHAR Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

typedef struct _ATOM_LIST_INFORMATION {
    ULONG NumberOfAtoms;
    ATOM Atoms[1];
} ATOM_LIST_INFORMATION, *PATOM_LIST_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLdtEntries(
    IN ULONG Selector1,
    IN LDT_ENTRY LdtEntry1,
    IN ULONG Selector2,
    IN LDT_ENTRY LdtEntry2
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwVdmControl(
    IN ULONG ControlCode,
    IN PVOID ControlData
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCANSI_STRING SourceString,
    BOOL AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOL AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOL AllocateDestinationString
    );

    }
}
