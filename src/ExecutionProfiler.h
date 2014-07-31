/*
 * This file is a part of the Sharemind framework.
 * Copyright (C) Cybernetica AS
 *
 * All rights are reserved. Reproduction in whole or part is prohibited
 * without the written consent of the copyright owner. The usage of this
 * code is subject to the appropriate license agreement.
 */

#ifndef SHAREMINDCOMMON_EXECUTIONPROFILER_H
#define SHAREMINDCOMMON_EXECUTIONPROFILER_H

#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sharemind/common/Logger/Logger.h>
#include <stack>
#include "MicrosecondTimer.h"


namespace sharemind {

class ExecutionProfiler;

//#define PROFILE_MINER
//#define PROFILE_SECREC
//#define PROFILE_VM

/* common profiling defines */
#if defined(PROFILE_MINER) || defined(PROFILE_SECREC) || defined(PROFILE_VM)
    #define START_SECTION(profiler, sid, name, T, parameter)\
        uint32_t (sid) = (profiler).startSection<T> ((name), (parameter));
    #define END_SECTION(profiler, sid)\
        (profiler).endSection((sid));
    #define SCOPED_SECTION(profiler, sid, name, T, parameter)\
        ExecutionSectionScope<T> sectionScope_##sid((profiler), (name), (parameter), true);

    #define PUSH_PARENT_SECTION(profiler, sid)\
        (profiler).pushParentSection((sid));
    #define POP_PARENT_SECTION(profiler)\
        (profiler).popParentSection();
    #define PROCESS_SECTIONS(profiler, timeMs)\
        (profiler).processLog((timeMs));
#else
    #define START_SECTION(profiler, sid, type, parameter)
    #define END_SECTION(profiler, sid)
    #define SCOPED_SECTION(profiler, sid, type, parameter)
    #define PUSH_PARENT_SECTION(profiler, sid)
    #define POP_PARENT_SECTION(profiler)
    #define PROCESS_SECTIONS(profiler, timeMs)
#endif

/* Miner specific profiling defines */
#ifdef PROFILE_MINER
    #define START_SECTION_MINER(profiler, sid, name, parameter)\
        START_SECTION((profiler), (sid), (name), const char *, (parameter))
    #define END_SECTION_MINER(profiler, sid)\
        END_SECTION((profiler), (sid))
    #define SCOPED_SECTION_MINER(profiler, sid, name, parameter)\
        SCOPED_SECTION((profiler), sid, (name), const char *, (parameter))
#else
    #define START_SECTION_MINER(profiler, sid, type, parameter)
    #define END_SECTION_MINER(profiler, sid)
    #define SCOPED_SECTION_MINER(profiler, sid, name, parameter)
#endif

/* SecreC specific profiling defines */
#ifdef PROFILE_SECREC
    #define START_SECTION_SECREC(profiler, sid, type, parameter)\
        START_SECTION((profiler), (sid), (type), uint32_t, (parameter))
    #define END_SECTION_SECREC(profiler, sid)\
        END_SECTION((profiler), (sid))
    #define SCOPED_SECTION_SECREC(profiler, sid, name, parameter)\
        SCOPED_SECTION((profiler), sid, (name), uint32_t, (parameter))
#else
    #define START_SECTION_SECREC(profiler, sid, type, parameter)
    #define END_SECTION_SECREC(profiler, sid)
    #define SCOPED_SECTION_SECREC(profiler, sid, name, parameter)
#endif

/* VM specific profiling defines */
#ifdef PROFILE_VM
    #define START_SECTION_VM(profiler, sid, type, parameter)\
        START_SECTION((profiler), (sid), (type), const char *, (parameter))
    #define END_SECTION_VM(profiler, sid)\
        END_SECTION((profiler), (sid))
    #define SCOPED_SECTION_VM(profiler, sid, name, parameter)\
        SCOPED_SECTION((profiler), sid, (name), const char *, (parameter))
#else
    #define START_SECTION_VM(profiler, sid, type, parameter)
    #define END_SECTION_VM(profiler, sid)
    #define SCOPED_SECTION_VM(profiler, sid, name, parameter)
#endif

#ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
struct NetworkStats {
    uint64_t receivedBytes;
    uint64_t sentBytes;
};

typedef std::map<size_t, NetworkStats> MinerNetworkStatistics;
#endif

/**
 This is a data structure for storing executed sections for profiling purposes.

 This class is used internally by ExecutionProfiler.
*/
class ExecutionSection {

    friend class ExecutionProfiler;

public: /* Types: */

    union SectionName {
        const char * namePtr;
        uint32_t nameCacheId;

        SectionName(const char * name) : namePtr(name) {}
        SectionName(uint32_t cacheid) : nameCacheId(cacheid) {}
    };

public:

    /**
     Constructs an execution section based on the given parameters

     \param[in] sectionName a unique name describing what the section does
     \param[in] complexityParameter a number describing the O(n) complexity of the section
     \param[in] parentSectionId the identifier of a section which contains this one
    */
    ExecutionSection(const char * sectionName,
                     uint32_t sectionId,
                     uint32_t parentSectionId,
                     MicrosecondTimerTime startTime,
                     MicrosecondTimerTime endTime,
                     size_t complexityParameter
                     #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
                     , const MinerNetworkStatistics & netStats
                     #endif
                     );

    ExecutionSection(uint32_t sectionType,
                     uint32_t sectionId,
                     uint32_t parentSectionId,
                     MicrosecondTimerTime startTime,
                     MicrosecondTimerTime endTime,
                     size_t complexityParameter
                     #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
                     , const MinerNetworkStatistics & netStats
                     #endif
                     );

    /** The identifier of this section */
    uint32_t sectionId;

    /** The identifier of the parent section containing this one (zero, if none) */
    uint32_t parentSectionId;

    /** A timestamp for the moment the section started */
    MicrosecondTimerTime startTime;

    /** A timestamp for the moment the section was completed */
    MicrosecondTimerTime endTime;

    /** The O(n) complexity parameter for the section */
    size_t complexityParameter;

    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
    /**
     * Network statistics for the moment the section started.
     * Amounts of data (relevant to this section) transferred between local and remote miners.
     */
    MinerNetworkStatistics startNetworkStatistics;

    /**
     * Network statistics for the moment the section was completed.
     * Amounts of data (relevant to this section) transferred between local and remote miners.
     */
    MinerNetworkStatistics endNetworkStatistics;
    #endif

private:

    /** The name identifier of this section */
    const SectionName m_sectionName;
    const bool m_nameCached;
};


/**
 The ExecutionProfiler allows the programmer to perform pinpoint profiling by
 specifying sections of code with the Start/FinishSection methods.

 Each time the section is entered and exited the time spent is stored and logged. The
 resulting log file can be processed by various tools to create datasets and
 graphs of program execution.
*/
class ExecutionProfiler {

public: /* Methods: */

    ExecutionProfiler(const Logger & logger)
        : m_logger(logger, "[ExecutionProfiler]")
        , m_nextSectionTypeId(0)
        , m_nextSectionId(0)
        , m_profilingActive(false)
    {}

    inline ~ExecutionProfiler() noexcept { finishLog(); }

    /**
     Starts the profiler by specifying a log file to write sections into.

     The profiler will open a file with the given name and will log all sections to this file.

     \param[in] filename the name of the file to log the sections to

     \retval true if opening the file was successful
     \retval false if opening the file failed
    */
    bool startLog(const std::string &filename);

    /**
     Defines a new section type.

     This method creates a new section type and maps an integer identifier to its name. The identifier can then be passed to startSection to identify the code section being profiled.
     This allows to avoid performance penalty for having to copy the section name every time a section is started, as this process can happen very fast and often.

     \param[in] name the name of section type, that will be used in the logging output to identify the type of section being logged.
     \returns a unique identifier for the new section type.
    */
    uint32_t newSectionType(const char *name);

    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
    /**
     Specifies the starting point of a code section for profiling.

     This method is called before the profiled piece of code. The EndSection method is called after.
     The profiler will store timestamps of both events and compute durations during ProcessLog invocations.

     \param[in] sectionTypeName a value that specifies the type name of a section describing what is being done in the section
     \param[in] complexityParameter indicates the complexity parameter for the section (eg number of values in the processed vector)
     \param[in] startNetStats the network statistics measured in the beginning of the section.
     \param[in] parentSectionId the identifier of a section which contains this new section (see also: PushParentSection)

     \returns an unique identifier for the profiled code section which should be passed to EndSection later on
    */
    template<class T>
    uint32_t startSection(T sectionTypeName,
                          size_t complexityParameter,
                          const MinerNetworkStatistics & startNetStats,
                          uint32_t parentSectionId = 0)
    {
        return startSection__<T>(std::move(sectionTypeName),
                                 complexityParameter,
                                 startNetStats,
                                 parentSectionId);
    }
    #endif

    template<class T>
    uint32_t startSection(T sectionTypeName,
                          size_t complexityParameter,
                          uint32_t parentSectionId = 0)
    {
        return startSection__<T>(
                    sectionTypeName,
                    complexityParameter,
                    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
                    MinerNetworkStatistics(),
                    #endif
                    parentSectionId);
    }

    /**
     Completes the specified section.

     This method finds the section specified by sectionId and completes it.
     The current time is stored as the end timestamp for this section.

     \param[in] sectionId the id returned by StartSection. If no such section has been started, the method does nothing.
    */
    void endSection(uint32_t sectionId);

    /**
     Completes the specified section.

     This method finds the section specified by sectionId and completes it.
     The given end time is stored as the end timestamp for this section.

     \param[in] sectionId the id returned by StartSection. If no such section has been started, the method does nothing.
     \param[in] endTime the end time to be stored in the section specified by sectionId.
     \param[in] endNetStats the network statistics measured in the end of the section.
    */
    void endSection(uint32_t sectionId,
                    const MicrosecondTimerTime endTime
                    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
                    , const MinerNetworkStatistics & endNetStats
                    #endif
                    );

    /**
     Finishes profiling and writes cached section to the log file.
    */
    void finishLog();

    /**
     Processes and logs sections cached in memory.

     If more than a thousand sections are cached, work for the allowed time of milliseconds
     and write as many as possible to the disk. For each section the duration is also computed.

     \param[in] timeLimitMs the number of milliseconds allowed for flushing sections to the file.
     If this is zero, no sections are flushed. The default value for this parameter is 10.
     \param[in] flush if true, flushes the sections, false by default
    */
    void processLog(uint32_t timeLimitMs = 10, bool flush = false);

    /**
     Specifies a default parent section for subsequent sections.

     Sometimes it is not comfortable to implicitly specify a parent section identifer when starting a section.
     For example, the identifier might be specified elsewhere in the code and it is not feasible to transfer it.
     This method allows the programmer to specify a section which will be used as a parent section for all
     sections which are started later, but have no parent specified.

     The identifier is pushed on a stack of identifiers which allows the programmer to nest sections.
     The PopParentSection method is used to pop the top identifier from this stack.

     \param[in] sectionId the id of the section to be used as a parent for subsequent sections
    */
    void pushParentSection(uint32_t sectionId);

    /**
     Pops a parent section identifier from the stack.

     If the stack is empty, nothing is done.
    */
    void popParentSection();


private: /* Methods: */

    /**
     Specifies the starting point of a code section for profiling.

     This method is called before the profiled piece of code. The EndSection method is called after.
     The profiler will store timestamps of both events and compute durations during ProcessLog invocations.

     \param[in] sectionTypeName a value that specifies the type name of a section describing what is being done in the section
     \param[in] complexityParameter indicates the complexity parameter for the section (eg number of values in the processed vector)
     \param[in] startNetStats the network statistics measured in the beginning of the section.
     \param[in] parentSectionId the identifier of a section which contains this new section (see also: PushParentSection)

     \returns an unique identifier for the profiled code section which should be passed to EndSection later on
    */
    template<class T>
    uint32_t startSection__(
            T sectionTypeName,
            size_t complexityParameter,
            #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
            const MinerNetworkStatistics & startNetStats,
            #endif
            uint32_t parentSectionId = 0)
    {
        if (!m_profilingActive)
            return 0;

        // Lock the list
        std::lock_guard<std::mutex> lock(m_profileLogMutex);

        // Automatically set parent
        const uint32_t usedParentSectionId = parentSectionId == 0
                                             && !m_parentSectionStack.empty()
                                             ? m_parentSectionStack.top()
                                             : parentSectionId;

        // Create the entry and store it
        ExecutionSection * const s = new ExecutionSection(
                    sectionTypeName,
                    m_nextSectionId++,
                    usedParentSectionId,
                    0,
                    0,
                    complexityParameter
                    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
                    , startNetStats
                    #endif
                    );

        m_sectionMap.insert(std::make_pair(s->sectionId, s));
        s->startTime = MicrosecondTimer_get_global_time();
        //WRITE_LOG_FULLDEBUG (m_logger, "[ExecutionProfiler] Started section " << s.sectionId << ".");
        return s->sectionId;
    }

    void __processLog(uint32_t timeLimitMs, bool flush);

    inline const char * getSectionName(ExecutionSection * s) const {
        if (s->m_nameCached) {
            std::map<uint32_t, char *>::const_iterator it = m_sectionTypes.find(s->m_sectionName.nameCacheId);
            return (it == m_sectionTypes.end() ? "undefined_section" : it->second);
        } else {
            return s->m_sectionName.namePtr;
        }
    }

private: /* Fields: */

    const Logger m_logger;

    /** The name of the logfile to use */
    std::string m_filename;

    /** Handle of the file we write the profiling log to */
    std::ofstream m_logfile;

    /** The map of section types */
    std::map<uint32_t, char *> m_sectionTypes;

    /** The next available section type identifier */
    uint32_t m_nextSectionTypeId;

    /**
     The stack of parent section identifiers.

     \see PushParentSection
    */
    std::stack<uint32_t> m_parentSectionStack;

    /**
     The map of execution sections

     \see PushParentSection
     */
    std::map<uint32_t, ExecutionSection*> m_sectionMap;

    /** The cache of sections waiting for flushing to the disk */
    std::deque<ExecutionSection*> m_sections;

    /** The next available section identifier */
    uint32_t m_nextSectionId;

    /** The lock for the profiling log */
    std::mutex m_profileLogMutex;

    /** True, if profiling is active */
    bool m_profilingActive;

};

/**
 This class is used to automatically end ExecutionProfile sections and pop
 parent sections if an instance of this class goes out of scope.
*/
template <class T>
class ExecutionSectionScope {

public:
    /**
     Constructs the scope instance for the given section.

     \param[in] profiler the profiler instance to create this section on
     \param[in] sectionTypeName the section type name identifier
     \param[in] complexityParameter the O(n) parameter describing the complexity of this section
     \param[in] pushParent whether or not to push a parent section and pop it when finished
    */
    ExecutionSectionScope(ExecutionProfiler& profiler, T sectionTypeName, size_t complexityParameter, bool pushParent = true)
        : m_profiler (profiler)
        , m_sectionId (profiler.startSection<T>(sectionTypeName, complexityParameter))
        , m_isParent (pushParent)
    {
        if (m_isParent)
            m_profiler.pushParentSection(m_sectionId);
    }

    /**
     Pops the parent section and ends the section
    */
    ~ExecutionSectionScope() {
        if (m_isParent)
            m_profiler.popParentSection();

        m_profiler.endSection(m_sectionId);
    }

private:
    /** The identifier of the section to end. */
    uint32_t m_sectionId;

    /** Indicates whether the section is parent section and should be popped. */
    bool m_isParent;

    /** Holds the reference to the ExecutionProfiler instance. */
    ExecutionProfiler& m_profiler;
};

} /* namespace sharemind { */

#endif /* SHAREMINDCOMMON_EXECUTIONPROFILER_H */
