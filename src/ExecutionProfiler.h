/*
 * This file is a part of the Sharemind framework.
 *
 * Copyright (C) Dan Bogdanov, 2006-2008
 * All rights are reserved. Reproduction in whole or part is prohibited
 * without the written consent of the copyright owner.
 *
 * Main contributors:
 * Dan Bogdanov (db@math.ut.ee)
 */
#ifndef EXECUTIONPROFILER_H
#define EXECUTIONPROFILER_H

#include <stack>
#include <deque>
#include <boost/thread.hpp>
using std::stack;
using std::deque;
using boost::mutex;

/*! The codes describing actions performed in an execution section.
They are specified numerically, because the ProfileLogAnalyst tool
relies on these numbers right now. */
enum ActionCodes {

	// NetworkNode actions

	/*! All vector sending actions together (NetworkNode::SendValueVector) */
	ACTION_SEND_VECTOR = 1,
	/*! Flush the queue of packets waiting to be resent */
	ACTION_FLUSH_RESEND_QUEUE = 2,
	/*! Wait until the delivery of all previously sent packets is confirmed */
	ACTION_CONFIRM_VECTOR_DELIVERY = 3,
	/*! Wait until the network thread populates the outgoing buffers */
	ACTION_FILL_OUTBOUND_BUFFER = 4,
	/*! Wait until the outgoing buffers are emptied and sent */
	ACTION_FLUSH_OUTBOUND_BUFFER = 5,
	/*! Serialize the vector to be sent */
	ACTION_VECTOR_SERIALIZATION = 6,
	/*! Call the actual library Send method */
	ACTION_SEND_INVOCATION = 7,
	/*! All the time spent in the (NetworkNode::ReceiveValueVector) method */
	ACTION_RECEIVE_VECTOR = 10,
	/*! All the time spent in the (NetworkNode::ProcessIncomingVector) method */
	ACTION_PROCESS_INCOMING_VECTOR = 11,
	/*! Deserialize a vector from an incoming bitstream */
	ACTION_DESERIALIZE_VECTOR = 12,

	// Queries

	/*! The whole lifespan of a vector multiplication query */
	ACTION_MULTIPLY_VECTORS = 100,
	/*! The whole lifespan of a vector equality comparison query */
	ACTION_COMPARE_VECTORS_EQUALITY = 101,
	/*! The whole lifespan of a vector greater-than comparison query */
	ACTION_COMPARE_VECTORS_GREATER_THAN = 102,
	/*! The whole lifespan of a value addition query */
	ACTION_ADD_VALUES = 103

};

/*! Codes specifying the location of the action, if it does not follow directly from the action. */
enum LocationCodes {

	// NetworkNode operations

	/*! The SendValueVector method */
	LOCATION_NETWORKNODE_SENDVECTOR = 1,
	/*! The ReceiveValueVector method */
	LOCATION_NETWORKNODE_RECEIVEVECTOR = 2,

	/*! The ProcessIncomingVector method */
	LOCATION_NETWORKNODE_PROCESSINCOMINGVECTOR = 3,

	// Queries

	/*! All methods of the vectorized multiplication query */
	LOCATION_VECTORIZED_MULTIPLICATION_QUERY = 4,

	/*! All methods of the vectorized comparison query */
	LOCATION_VECTORIZED_EQUALITY_COMPARISON_QUERY = 5,

	/*! All methods of the vectorized comparison query */
	LOCATION_VECTORIZED_GREATER_THAN_COMPARISON_QUERY = 6,
	// Fundamental classws

	/*! The miner interface (all local queries) */
	LOCATION_MINER_INTERFACE = 7

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
	 \param[in] locationCode a code describing where the section is (\see the LocationCodes enum)
	 \param[in] complexityParameter a number describing the O(n) complexity of the section
	 \param[in] parentSectionId the identifier of a section which contains this one
	*/
	ExecutionSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId);

	/**
	 Stores the action code of the section
	*/
	uint16 actionCode;

	/**
	 Stores the location code of the section
	*/
	uint16 locationCode;

	/**
	 The O(n) complexity parameter for the section
	*/
	uint32 complexityParameter;

	/**
	 A timestamp for the moment the section was completed
	*/
	uint32 endTime;

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

	/**
	 Starts the profiler by specifying a log file to write sections into.

	 The profiler will open a file with the given name and will log all sections to this file.

	 \param[in] filename the name of the file to log the sections to

	 \retval true if opening the file was successful
	 \retval false if opening the file failed
	*/
	static bool StartLog(string filename);

	/**
	 Specifies the starting point of a code section for profiling.

	 This method is called before the profiled piece of code. The EndSection method is called after.
	 The profiler will store timestamps of both events and compute durations during ProcessLog invocations.

	 \param[in] actionCode a value in the ActionCodes enum which specifies what is being done in the section
	 \param[in] locationCode a value in the LocationCodes enum which specifies the location of the section in the code
	 \param[in] complexityParameter indicates the complexity parameter for the section (eg number of values in the processed vector)
	 \param[in] parentSectionId the identifier of a section which contains this new section (see also: PushParentSection)

	 \returns an unique identifier for the profiled code section which should be passed to EndSection later on
	*/
	static uint32 StartSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId = 0);

	/**
	 Completes the specified section.

	 This method finds the section specified by sectionId and completes it.
	 The current time is stored as the end timestamp for this section.

	 \param[in] sectionId the id returned by StartSection. If no such section has been started, the method does nothing.
	*/
	static void EndSection(uint32 sectionId);

	/**
	 Finishes profiling and writes or cached section to the log file.
	*/
	static void FinishLog();

	/**
	 Processes and logs sections cached in memory.

	 If more than a thousand sections are cached, work for the allowed time of milliseconds
	 and write as many as possible to the disk. For each section the duration is also computed.

	 \param[in] timeLimitMs the number of milliseconds allowed for flushing sections to the file.
	 If this is zero, no sections are flushed. The default value for this parameter is 10.
	*/
	static void ProcessLog(uint32 timeLimitMs = 10);

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
public:
	static void PushParentSection(uint32 sectionId);


	/**
	 Pops a parent section identifier from the stack.

	 If the stack is empty, nothing is done.
	*/
	static void PopParentSection();

private:

	/**
	 Handle of the file we write the profiling log to
	*/
	static ofstream logfile;

	/**
	 The stack of parent section identifiers.

	 \see PushParentSection
	*/
	static stack<uint32> sectionStack;

	/**
	 The name of the logfile to use
	*/
	static string filename;

	/**
	 The cache of sections waiting for flushing to the disk
	*/
	static deque<ExecutionSection> sections;

	/**
	 The next available section identifier
	*/
	static uint32 sectionOffset;

	static mutex profileLogMutex;

};

#endif // EXECUTIONPROFILER_H
