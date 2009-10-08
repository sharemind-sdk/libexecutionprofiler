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

#include "GetTime.h"
#include "common/CommonLibrary.h"

// Initialize static variables
ofstream ExecutionProfiler::logfile;
stack<uint32> ExecutionProfiler::sectionStack;
string ExecutionProfiler::filename;
deque<ExecutionSection> ExecutionProfiler::sections;
map<uint32, ExecutionSection> ExecutionProfiler::sectionMap;
mutex ExecutionProfiler::profileLogMutex;
bool ExecutionProfiler::enableProfiling;
uint32 ExecutionProfiler::nextSectionId = 0;

ExecutionSection::ExecutionSection(uint16 actionCode, uint32 complexityParameter, uint32 parentSectionId) {
	this->actionCode = actionCode;
	this->complexityParameter = complexityParameter;
	this->parentSectionId = parentSectionId;
}


void ExecutionProfiler::EndSection(uint32 sectionId) {
	if (!enableProfiling)
		return;
	
	// Lock the list
	boost::mutex::scoped_lock lock (profileLogMutex);
	
	map<uint32, ExecutionSection>::iterator it = sectionMap.find (sectionId);
	if (it == sectionMap.end ()) {
		WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] Could not end section " << sectionId << ". Not in queue.");
		return;
	}

	it->second.endTime = RakNet::GetTime ();
	sections.push_back (it->second);
	sectionMap.erase (it);
}


void ExecutionProfiler::FinishLog() {
	if (!enableProfiling)
		return;
	
	// Lock the list
	boost::mutex::scoped_lock lock (profileLogMutex);

	WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Flushing profiling log file.");

	// Flush all sections to the disc
	while (sections.size () > 0) {
		// Give time in one-second slices
		ExecutionSection s = sections.front ();
		//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Logging section " << s.sectionId << ".");
		{
            boost::mutex::scoped_lock lock (Console::theMutex);
            logfile << s.sectionId << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		}
		sections.pop_front ();
	}

	// Close the log file, if necessary
	if (logfile.is_open ()) {
		WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Closing log file " << filename);
		logfile.close ();
	}
}


void ExecutionProfiler::PopParentSection() {
	if (!enableProfiling)
		return;
	
	// Lock the list
	boost::mutex::scoped_lock lock  (profileLogMutex);

	if (sectionStack.size () > 0)
		sectionStack.pop ();
}


void ExecutionProfiler::ProcessLog(uint32 timeLimitMs, bool flush) {
	if (!enableProfiling)
		return;

	// Lock the list
	boost::mutex::scoped_lock lock  (profileLogMutex);

	uint32 start = RakNet::GetTime ();
	uint32 end = start + timeLimitMs;

	uint32 leaveSections = 0;

	// The new design does not require flushing
	/*
	if (!flush)
		leaveSections = 100000;
	*/
		
	while (RakNet::GetTime () < end && sections.size () > leaveSections) {
		ExecutionSection s = sections.front ();
		//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Logging section " << s.sectionId << ".");
		{
            boost::mutex::scoped_lock lock (Console::theMutex);
            logfile << s.sectionId << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		}
		sections.pop_front ();
	}
}


void ExecutionProfiler::PushParentSection(uint32 sectionId) {
	if (!enableProfiling)
		return;
	
	// Lock the list
	boost::mutex::scoped_lock lock (profileLogMutex);
	sectionStack.push (sectionId);
}


bool ExecutionProfiler::StartLog(string filename) {
	// Lock the list
	boost::mutex::scoped_lock lock (profileLogMutex);

	// Check if we have a filename
	if (filename.length () > 0) {

		// Try to open the log file
		logfile.open (filename.c_str ());
		if (logfile.bad() || logfile.fail ()) {
			WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] ERROR: Can't open console log file " << filename << "!");
			return false;
		}

		WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Opened profiling log file " << filename << "!");

        {
            boost::mutex::scoped_lock lock (Console::theMutex);
            logfile << "SectionID" << ", " << "Start" << ", " << "End" << ", " << "Duration" << ", " << "Action" << ", " << "Complexity" << ", " << "ParentSectionID" << endl;
        }

		enableProfiling = true;
		return true;

	} else {

		// We didn't get a filename so spread the information about that.
		WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler]  ERROR: Empty log file name!");
		return false;
	}
}


uint32 ExecutionProfiler::StartSection(uint16 actionCode, uint32 complexityParameter, uint32 parentSectionId/* = 0 */) {
	if (!enableProfiling)
		return 0;
	
	// Lock the list
	boost::mutex::scoped_lock lock (profileLogMutex);

	// Automatically set parent
	uint32 usedParentSectionId = parentSectionId;
	if (parentSectionId == 0) {
		if (sectionStack.size () > 0) {
			usedParentSectionId = sectionStack.top ();
		}
	}

	// Create the entry and store it
	ExecutionSection s (actionCode, complexityParameter, usedParentSectionId);
	s.startTime = RakNet::GetTime ();
	s.endTime = 0;
	s.sectionId = nextSectionId;
	sectionMap.insert (make_pair (s.sectionId, s));
	nextSectionId++;

	//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Started section " << s.sectionId << ".");

	return s.sectionId;
}
