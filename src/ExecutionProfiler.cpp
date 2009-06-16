/*
 * This file is a part of the Sharemind framework.
 *
 * Copyright (C) AS Cybernetica, 2006-2009
 *
 * Main contributors:
 * Dan Bogdanov (dan@cyber.ee)
 */

#include "GetTime.h"
#include "Sharemind.h"

// Initialize static variables
ofstream ExecutionProfiler::logfile;
stack<uint32> ExecutionProfiler::sectionStack;
string ExecutionProfiler::filename;
deque<ExecutionSection> ExecutionProfiler::sections;
uint32 ExecutionProfiler::sectionOffset = 0;
mutex ExecutionProfiler::profileLogMutex;
bool ExecutionProfiler::enableProfiling;

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

	if ((sectionId - sectionOffset) >= sections.size ()) {
		WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] Can not find section to end. Requested section ID " << sectionId << " with offset " << sectionOffset << ".");
		return;
	}

	sections[sectionId - sectionOffset].endTime = RakNet::GetTime ();
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
		//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Logging section " << sectionOffset << ".");
		{
            boost::mutex::scoped_lock lock (Console::theMutex);
            logfile << sectionOffset << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		}
		sections.pop_front ();
		sectionOffset++;
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
	if (!flush)
		leaveSections = 10000;
		
	while (RakNet::GetTime () < end && sections.size () > leaveSections) {
		ExecutionSection s = sections.front ();
		//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Logging section " << sectionOffset << ".");
		{
            boost::mutex::scoped_lock lock (Console::theMutex);
            logfile << sectionOffset << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		}
		sections.pop_front ();
		sectionOffset++;
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
		sectionOffset = 1;

		enableProfiling = true;
		return true;

	} else {

		// We didn't get a filename so spread the information about that.
		WRITE_TO_LOG (LOG_MINIMAL, "[Console] ERROR: Empty log file name!");
		return false;
	}
}


uint32 ExecutionProfiler::StartSection(uint16 actionCode, uint32 complexityParameter, uint32 parentSectionId/* = 0 */) {
	if (!enableProfiling)
		return 0;
	
	// Lock the list
	boost::mutex::scoped_lock lock (profileLogMutex);

	if (parentSectionId > sectionOffset + (sections.size () - 1)) {
		WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] WARNING: The specified parent section " << parentSectionId << " does not exist!");
	}

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
	sections.push_back (s);

	uint32 sectionId = (sections.size () - 1) + sectionOffset;
	//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Started section " << sectionId << " (" << actionCode << ", " << complexityParameter << ", " << parentSectionId << ").");

	return sectionId;
}
