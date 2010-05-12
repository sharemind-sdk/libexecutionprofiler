/*
 * This file is a part of the Sharemind framework.
 *
 * Copyright (C) AS Cybernetica
 * All rights are reserved. Reproduction in whole or part is prohibited
 * without the written consent of the copyright owner.
 *
 * Main contributors:
 * Dan Bogdanov (dan@cyber.ee)
 */
#ifndef EXECUTIONPROFILER_H
#define EXECUTIONPROFILER_H

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
using std::ofstream;
using boost::unordered_map;
using std::stack;
using std::map;
using std::deque;
using std::ofstream;
using std::string;
using boost::mutex;

class Console;

typedef boost::unordered_map<std::string, int> timingmap;

#define PROFILE_APP
#define PROFILE_VM

#if defined(PROFILE_APP) || defined(PROFILE_VM)
	#define PUSH_PARENT_SECTION(profiler, section)\
		if (profiler) (profiler)->pushParentSection(section);
	#define POP_PARENT_SECTION(profiler)\
		if (profiler) (profiler)->popParentSection();
	#define PROCESS_SECTIONS(profiler, time)\
		if (profiler) (profiler)->processLog(time);
#else
	#define PUSH_PARENT_SECTION(profiler, section)
	#define POP_PARENT_SECTION(profiler)
	#define PROCESS_SECTIONS(profiler, time)
#endif

#ifdef PROFILE_VM
	#define START_SECTION_VM(profiler, x, type, parameter)\
		uint32 x = 0; if (profiler) x = (profiler)->startSection (type, parameter);
	#define END_SECTION_VM(profiler, x)\
		if (profiler) (profiler)->endSection(x);
#else
	#define START_SECTION_VM(profiler, x, type, parameter)
	#define END_SECTION_VM(profiler, x)
#endif

#ifdef PROFILE_APP
	#define START_SECTION_APP(profiler, x, type, parameter)\
		uint32 x = 0; if (profiler) x = (profiler)->startSection (type, parameter);
	#define END_SECTION_APP(profiler, x)\
		if (profiler) (profiler)->endSection(x);
	#define START_INSTRUCTION_TIMER(profiler, x, instruction)\
		uint32 x = 0; if (profiler) x = RakNet::GetTime ();
	#define END_INSTRUCTION_TIMER(profiler, x)\
		if (profiler) (profiler)->logInstructionTime (instructionToExecute->OpName(), RakNet::GetTime() - x);
	#define DUMP_INSTRUCTION_TIMINGS(profiler, filename)\
		if (profiler) (profiler)->dumpInstructionTimings (filename);
#else
	#define START_SECTION_APP(profiler, x, type, parameter)
	#define END_SECTION_APP(profiler, x)
	#define START_INSTRUCTION_TIMER(profiler, x, instruction)
	#define END_INSTRUCTION_TIMER(profiler, x)
	#define DUMP_INSTRUCTION_TIMINGS(profiler, filename)
#endif



/*! The codes describing actions performed in an execution section.
They are specified numerically, because the ProfileLogAnalyst tool
relies on these numbers right now. */
enum ActionCodes {
	
	// NetworkNode actions

	/*! The time spent to receive strings */
	ACTION_RECEIVE_STRING = 1,
	
	/*! The time spent to receive values */
	ACTION_RECEIVE_VALUE = 2,

	/*! The time spent to receive value vectors */
	ACTION_RECEIVE_VALUE_VECTOR = 3,

	/*! The time spent to flush outgoing network messages */
	ACTION_FLUSH_NETWORK = 4,
	
	/*! The time spent to send messages in the controller sessions' outgoing queues */
	ACTION_PROCESS_OUTGOING_QUEUES = 5,

	/*! The time spent to process and distribute incoming messages */
	ACTION_PROCESS_INCOMING_PACKETS = 6,

	/*! The time spent in the data saving synchronization protocol */
	ACTION_PROTOCOL_DATASAVING = 7,
	
	/*! The time spent in the test protocol */
	ACTION_PROTOCOL_TESTING = 8,
	
	/*! The time spent in the bit extraction protocol */	
	ACTION_PROTOCOL_VECTORIZED_ADDITION = 9,

	/*! The time spent in the bit extraction protocol */	
	ACTION_PROTOCOL_VECTORIZED_BITEXTRACTION = 10,
	
	/*! The time spent in the bitwise addition protocol */
	ACTION_PROTOCOL_VECTORIZED_BITWISEADDITION = 11,

	/*! The time spent in the share conversion protocol */
	ACTION_PROTOCOL_VECTORIZED_SHARECONVERSION = 12,

	/*! The time spent in the multiplication protocol */
	ACTION_PROTOCOL_VECTORIZED_MULTIPLICATION = 13,

	/*! The time spent in the division protocol */
	ACTION_PROTOCOL_VECTORIZED_DIVISION = 14,

	/*! The time spent in the remainder computation protocol */
	ACTION_PROTOCOL_VECTORIZED_REMAINDER = 15,

	/*! The time spent in the multiplication protocol */
	ACTION_PROTOCOL_VECTORIZED_SHIFTRIGHT = 16,

	/*! The whole lifespan of a vector equality comparison protocol */
	ACTION_PROTOCOL_VECTORIZED_EQUALITYCOMPARISON = 17,

	/*! The whole lifespan of a vector greater-than comparison protocol */
	ACTION_PROTOCOL_VECTORIZED_GREATERTHANCOMPARISON = 18,

	/*! Randomness generation */
	ACTION_RANDOMNESS_GENERATION = 19,

	/*! The execution of a script */
	ACTION_SCRIPT_EXECUTION = 20,

	/*! Various database I/O operations */
	ACTION_DATABASE_IO = 21,

	/*! Sharing and publishing of values within the VM */
	ACTION_SHARING_AND_PUBLISHING = 22,

	ACTION_VECTOR_MANAGEMENT = 23,
	
	ACTION_EXPRESSION_MANAGEMENT = 24,
	
	ACTION_EXPRESSION_EVALUATION = 25

};

/**
 This is a data structure for storing executed sections for profiling purposes.

 This class is used internally by ExecutionProfiler.
*/
struct ExecutionSection {

public:

	/**
	 Constructs an execution section based on the given parameters

	 \param[in] actionCode a code describing what the section does (\see the ActionCodes enum)
	 \param[in] complexityParameter a number describing the O(n) complexity of the section
	 \param[in] parentSectionId the identifier of a section which contains this one
	*/
	ExecutionSection(uint16 actionCode, uint32 complexityParameter, uint32 parentSectionId);

	/**
	 Stores the action code of the section
	*/
	uint16 actionCode;

	/**
	 The O(n) complexity parameter for the section
	*/
	uint32 complexityParameter;

	/**
	 A timestamp for the moment the section was completed
	*/
	uint32 endTime;

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

};


/**
 The ExecutionProfiler allows the programmer to perform pinpoint profiling by
 specifying sections of code with the Start/FinishSection methods.

 Each time the section is entered and exited the time spent is stored and logged. The
 resulting log file can be processed by various tools to create datasets and
 graphs of program execution.

 All methods are static and can be accessed from all over the code.

*/
class ExecutionProfiler {

public:

	ExecutionProfiler (Console* console);
	
	~ExecutionProfiler();
	
	/**
	 Starts the profiler by specifying a log file to write sections into.

	 The profiler will open a file with the given name and will log all sections to this file.

	 \param[in] filename the name of the file to log the sections to

	 \retval true if opening the file was successful
	 \retval false if opening the file failed
	*/
	bool startLog(const string &filename);

	/**
	 Specifies the starting point of a code section for profiling.

	 This method is called before the profiled piece of code. The EndSection method is called after.
	 The profiler will store timestamps of both events and compute durations during ProcessLog invocations.

	 \param[in] actionCode a value in the ActionCodes enum which specifies what is being done in the section
	 \param[in] complexityParameter indicates the complexity parameter for the section (eg number of values in the processed vector)
	 \param[in] parentSectionId the identifier of a section which contains this new section (see also: PushParentSection)

	 \returns an unique identifier for the profiled code section which should be passed to EndSection later on
	*/
	uint32 startSection(uint16 actionCode, uint32 complexityParameter, uint32 parentSectionId = 0);

	/**
	 Completes the specified section.

	 This method finds the section specified by sectionId and completes it.
	 The current time is stored as the end timestamp for this section.

	 \param[in] sectionId the id returned by StartSection. If no such section has been started, the method does nothing.
	*/
	void endSection(uint32 sectionId);

	/**
	 Finishes profiling and writes or cached section to the log file.
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
	
	void logInstructionTime (const string& name, uint32 time);
	
	void dumpInstructionTimings (const string& filename);
		
private:

	Console* m_console;

	timingmap m_instructionTimings;
	
	
	/**
	 Handle of the file we write the profiling log to
	*/
	ofstream m_logfile;

	/**
	 The stack of parent section identifiers.

	 \see PushParentSection
	*/
	stack<uint32> m_sectionStack;

	/**
	 The map of execution sections
	 
	 \see PushParentSection
	 */
	map<uint32, ExecutionSection> m_sectionMap;

	/**
	 The name of the logfile to use
	*/
	string m_filename;

	/**
	 The cache of sections waiting for flushing to the disk
	*/
	deque<ExecutionSection> m_sections;

	/**
	 The next available section identifier
	*/
	uint32 m_nextSectionId;

	/**
	 The lock for the profiling log
	*/
	mutex m_profileLogMutex;
	
	/**
	 True, if we have profiling enabled
	 */
	bool m_enableProfiling;

};

#endif // EXECUTIONPROFILER_H
