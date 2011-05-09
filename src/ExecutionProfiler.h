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

#include <stack>
#include <deque>
#include <map>
#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <fstream>

#include "common/SharemindTypes.h"
#include "common/ProfilerActionCode.h"

class Logger;

typedef boost::unordered_map<std::string, int> timingmap;

#define PROFILE_APP
#define PROFILE_VM

// common profiling defines
#if defined(PROFILE_APP) || defined(PROFILE_VM)
    #define START_SECTION(profiler, sid, type, parameter)\
        uint32 sid = 0; if (profiler) sid = (profiler)->startSection (type, parameter);
    #define END_SECTION(profiler, sid)\
        if (profiler) (profiler)->endSection(sid);

    #define PUSH_PARENT_SECTION(profiler, sid)\
        if (profiler) (profiler)->pushParentSection(sid);
	#define POP_PARENT_SECTION(profiler)\
		if (profiler) (profiler)->popParentSection();
	#define PROCESS_SECTIONS(profiler, time)\
		if (profiler) (profiler)->processLog(time);

    #define SCOPED_SECTION(profiler, sid, type, parameter)\
        START_SECTION(profiler, sid, type, parameter)\
        PUSH_PARENT_SECTION(profiler, sid)\
        ExecutionSectionScope sectionScope_##sid(profiler, sid, true);
#else
    #define START_SECTION(profiler, sid, type, parameter)
    #define END_SECTION(profiler, sid)
    #define PUSH_PARENT_SECTION(profiler, sid)
	#define POP_PARENT_SECTION(profiler)
	#define PROCESS_SECTIONS(profiler, time)
    #define SCOPED_SECTION(profiler, sid, type, parameter)
#endif

// vm specific profiling defines
#ifdef PROFILE_VM
    #define START_SECTION_VM(profiler, sid, type, parameter)\
        START_SECTION(profiler, sid, type, parameter)
    #define END_SECTION_VM(profiler, sid)\
        END_SECTION(profiler, sid)
#else
    #define START_SECTION_VM(profiler, sid, type, parameter)
    #define END_SECTION_VM(profiler, sid)
#endif

// application specific profiling defines
#ifdef PROFILE_APP
    #define START_SECTION_APP(profiler, sid, type, parameter)\
        START_SECTION(profiler, sid, type, parameter)
    #define END_SECTION_APP(profiler, sid)\
        END_SECTION(profiler, sid)

    #define START_INSTRUCTION_TIMER(profiler, x, instruction)\
		uint32 x = 0; if (profiler) x = RakNet::GetTime ();
	#define END_INSTRUCTION_TIMER(profiler, x)\
		if (profiler) (profiler)->logInstructionTime (instructionToExecute->OpName(), RakNet::GetTime() - x);
	#define DUMP_INSTRUCTION_TIMINGS(profiler, filename)\
		if (profiler) (profiler)->dumpInstructionTimings (filename);
#else
    #define START_SECTION_APP(profiler, sid, type, parameter)
    #define END_SECTION_APP(profiler, sid)

	#define START_INSTRUCTION_TIMER(profiler, x, instruction)
	#define END_INSTRUCTION_TIMER(profiler, x)
	#define DUMP_INSTRUCTION_TIMINGS(profiler, filename)
#endif


/**
 This is a data structure for storing executed sections for profiling purposes.

 This class is used internally by ExecutionProfiler.
*/
struct ExecutionSection {

public:

	/**
	 Constructs an execution section based on the given parameters

     \param[in] actionCode a code describing what the section does (\see the ProfilerActionCode enum)
	 \param[in] complexityParameter a number describing the O(n) complexity of the section
	 \param[in] parentSectionId the identifier of a section which contains this one
	*/
    ExecutionSection(ProfilerActionCode actionCode, uint32 complexityParameter, uint32 parentSectionId);

	/**
	 Stores the action code of the section
	*/
    ProfilerActionCode actionCode;

	/**
	 The identifier of this section
	*/
	uint32 sectionId;

	/**
	 The identifier of the parent section containing this one (zero, if none)
	*/
	uint32 parentSectionId;

	/**
	 A timestamp for the moment the section started
	*/
	uint32 startTime;

    /**
     A timestamp for the moment the section was completed
    */
    uint32 endTime;

    /**
     The O(n) complexity parameter for the section
    */
    uint32 complexityParameter;

};


/**
 The ExecutionProfiler allows the programmer to perform pinpoint profiling by
 specifying sections of code with the Start/FinishSection methods.

 Each time the section is entered and exited the time spent is stored and logged. The
 resulting log file can be processed by various tools to create datasets and
 graphs of program execution.
*/
class ExecutionProfiler {

public:

	ExecutionProfiler (Logger* logger);

	~ExecutionProfiler();

	/**
	 Starts the profiler by specifying a log file to write sections into.

	 The profiler will open a file with the given name and will log all sections to this file.

	 \param[in] filename the name of the file to log the sections to

	 \retval true if opening the file was successful
	 \retval false if opening the file failed
	*/
    bool startLog(const std::string &filename);

	/**
	 Specifies the starting point of a code section for profiling.

	 This method is called before the profiled piece of code. The EndSection method is called after.
	 The profiler will store timestamps of both events and compute durations during ProcessLog invocations.

	 \param[in] actionCode a value in the ActionCodes enum which specifies what is being done in the section
	 \param[in] complexityParameter indicates the complexity parameter for the section (eg number of values in the processed vector)
	 \param[in] parentSectionId the identifier of a section which contains this new section (see also: PushParentSection)

	 \returns an unique identifier for the profiled code section which should be passed to EndSection later on
	*/
    uint32 startSection(ProfilerActionCode actionCode, uint32 complexityParameter, uint32 parentSectionId = 0);

	/**
	 Completes the specified section.

	 This method finds the section specified by sectionId and completes it.
	 The current time is stored as the end timestamp for this section.

	 \param[in] sectionId the id returned by StartSection. If no such section has been started, the method does nothing.
	*/
	void endSection(uint32 sectionId);

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
	void processLog(uint32 timeLimitMs = 10, bool flush = false);

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
	void pushParentSection(uint32 sectionId);


	/**
	 Pops a parent section identifier from the stack.

	 If the stack is empty, nothing is done.
	*/
	void popParentSection();

    void logInstructionTime (const std::string& name, uint32 time);

    void dumpInstructionTimings (const std::string& filename);

private:

	Logger* m_logger;

	timingmap m_instructionTimings;


    /**
     The name of the logfile to use
    */
    std::string m_filename;

	/**
	 Handle of the file we write the profiling log to
	*/
    std::ofstream m_logfile;

	/**
	 The stack of parent section identifiers.

     \see PushParentSection
	*/
    std::stack<uint32> m_sectionStack;

	/**
	 The map of execution sections

	 \see PushParentSection
	 */
    std::map<uint32, ExecutionSection> m_sectionMap;

	/**
	 The cache of sections waiting for flushing to the disk
	*/
    std::deque<ExecutionSection> m_sections;

	/**
	 The next available section identifier
	*/
	uint32 m_nextSectionId;

	/**
	 The lock for the profiling log
	*/
    boost::mutex m_profileLogMutex;

	/**
	 True, if we have profiling enabled
	 */
	bool m_enableProfiling;

};

/**
 This class is used to automatically end ExecutionProfile sections and pop
 parent sections if an instance of this class goes out of scope.
*/
class ExecutionSectionScope {

public:
    /**
     Constructs the scope instance for the given section.

     \param[in] sectionId the section id
     \param[in] isParent is it parent and should it be popped.
    */
    ExecutionSectionScope(ExecutionProfiler* profiler, uint32 sectionId, bool isParent);

    /**
     Ends the section and possibly pops the parent section
    */
    ~ExecutionSectionScope();

private:
    /**
     The identifier of the section to end.
    */
    uint32 m_sectionId;

    /**
     Indicates whether the section is parent section and should be popped.
    */
    bool m_isParent;

    /**
      Holds the pointer to the ExecutionProfiler instance
    */
    ExecutionProfiler* m_profiler;
};

#endif // SHAREMINDCOMMON_EXECUTIONPROFILER_H
