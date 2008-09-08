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

#include "GetTime.h"

#include "Sharemind.h"

// Initialize static variables
ofstream ExecutionProfiler::logfile;
stack<uint32> ExecutionProfiler::sectionStack;
string ExecutionProfiler::filename;
vector<ExecutionSection> ExecutionProfiler::sections;
uint32 ExecutionProfiler::sectionOffset = 0;


ExecutionSection::ExecutionSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId) {
	this->actionCode = actionCode;
	this->locationCode = locationCode;
	this->complexityParameter = complexityParameter;
	this->parentSectionId = parentSectionId;
}


void ExecutionProfiler::EndSection(uint32 sectionId)
{
	if ((sectionId - sectionOffset) >= sections.size ()) {
		WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] Can not find section to end. Requested section ID " << sectionId << " with offset " << sectionOffset << ".");
		return;
	}

	sections[sectionId - sectionOffset].endTime = RakNet::GetTime ();
}


void ExecutionProfiler::FinishLog() {
	WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Flushing profiling log file.");

	// Flush all sections to the disc
	while (sections.size () > 0) {
		// Give time in one-second slices
		ExecutionSection s = sections.front ();
		//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Logging section " << sectionOffset << ".");
		logfile << sectionOffset << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.locationCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		sections.erase (sections.begin ());
		sectionOffset++;
	}

	// Close the log file, if necessary
	if (logfile.is_open ()) {
		WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Closing log file " << filename);
		logfile.close ();
	}
}


void ExecutionProfiler::PopParentSection() {
	if (sectionStack.size () > 0)
		sectionStack.pop ();
}


void ExecutionProfiler::ProcessLog(uint32 timeLimitMs/* = 10 */) {
	uint32 start = RakNet::GetTime ();
	uint32 end = start + timeLimitMs;

	while (RakNet::GetTime () < end && sections.size () > 5000) {
		ExecutionSection s = sections.front ();
		//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Logging section " << sectionOffset << ".");
		logfile << sectionOffset << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.locationCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		sections.erase (sections.begin ());
		sectionOffset++;
	}
}


void ExecutionProfiler::PushParentSection(uint32 sectionId) {
	sectionStack.push (sectionId);
}


bool ExecutionProfiler::StartLog(string filename) {
	// Check if we have a filename
	if (filename.length () > 0) {

		// Try to open the log file
		logfile.open (filename.c_str ());
		if (logfile.bad() || logfile.fail ()) {
			WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] ERROR: Can't open console log file " << filename << "!");
			return false;
		}

		WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Opened profiling log file " << filename << "!");
		logfile << "SectionID" << ", " << "Start" << ", " << "End" << ", " << "Duration" << ", " << "Action" << ", " << "Location" << ", " << "Complexity" << ", " << "ParentSectionID" << endl;
		sectionOffset = 1;

		return true;

	} else {

		// We didn't get a filename so spread the information about that.
		WRITE_TO_LOG (LOG_MINIMAL, "[Console] ERROR: Empty log file name!");
		return false;
	}
}


uint32 ExecutionProfiler::StartSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId/* = 0 */) {
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
	ExecutionSection s (actionCode, locationCode, complexityParameter, usedParentSectionId);
	s.startTime = RakNet::GetTime ();
	s.endTime = 0;
	sections.push_back (s);

	uint32 sectionId = (sections.size () - 1) + sectionOffset;
	//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Started section " << sectionId << " (" << actionCode << ", " << locationCode << ", " << complexityParameter << ", " << parentSectionId << ").");

	return sectionId;
}
