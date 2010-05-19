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
using namespace std;

ExecutionSection::ExecutionSection(uint16 actionCode, uint32 complexityParameter, uint32 parentSectionId) {
	this->actionCode = actionCode;
	this->complexityParameter = complexityParameter;
	this->parentSectionId = parentSectionId;
}


ExecutionProfiler::ExecutionProfiler(Console* console) 
  : m_console (console) 
{
	m_nextSectionId = 0;
	m_enableProfiling = false;
}


ExecutionProfiler::~ExecutionProfiler() {
	finishLog();
}

void ExecutionProfiler::endSection(uint32 sectionId) {
	if (!m_enableProfiling)
		return;

	// Lock the list
	boost::mutex::scoped_lock lock (m_profileLogMutex);

	map<uint32, ExecutionSection>::iterator it = m_sectionMap.find (sectionId);
	if (it == m_sectionMap.end ()) {
		WRITE_LOG_ERROR (m_console, "[ExecutionProfiler] Could not end section " << sectionId << ". Not in queue.");
		return;
	}

	it->second.endTime = RakNet::GetTime ();
	m_sections.push_back (it->second);
	m_sectionMap.erase (it);
}


void ExecutionProfiler::finishLog() {
	if (!m_enableProfiling)
		return;

	// Lock the list
	boost::mutex::scoped_lock lock (m_profileLogMutex);

	WRITE_LOG_DEBUG (m_console, "[ExecutionProfiler] Flushing profiling log file.");

	// Flush all sections to the disc
	while (m_sections.size () > 0) {
		// Give time in one-second slices
		ExecutionSection s = m_sections.front ();
		//WRITE_LOG_FULLDEBUG (m_console, "[ExecutionProfiler] Logging section " << s.sectionId << ".");
		{
            boost::mutex::scoped_lock lock (m_console->getStreamMutex());
            m_logfile << s.sectionId << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		}
		m_sections.pop_front ();
	}

	// Close the log file, if necessary
	if (m_logfile.is_open ()) {
		WRITE_LOG_DEBUG (m_console, "[ExecutionProfiler] Closing log file " << m_filename);
		m_logfile.close ();
	}
}


void ExecutionProfiler::popParentSection() {
	if (!m_enableProfiling)
		return;

	// Lock the list
	boost::mutex::scoped_lock lock  (m_profileLogMutex);

	if (m_sectionStack.size () > 0)
		m_sectionStack.pop ();
}


void ExecutionProfiler::processLog(uint32 timeLimitMs, bool flush) {
	if (!m_enableProfiling)
		return;

	// Lock the list
	boost::mutex::scoped_lock lock  (m_profileLogMutex);

	uint32 start = RakNet::GetTime ();
	uint32 end = start + timeLimitMs;

	uint32 leaveSections = 0;

	while (RakNet::GetTime () < end && m_sections.size () > leaveSections) {
		ExecutionSection s = m_sections.front ();
		//WRITE_LOG_FULLDEBUG (m_console, "[ExecutionProfiler] Logging section " << s.sectionId << ".");
		{
            boost::mutex::scoped_lock lock (m_console->getStreamMutex());
			if (m_logfile.is_open ())
				m_logfile << s.sectionId << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.complexityParameter << ", " << s.parentSectionId << endl;
		}
		m_sections.pop_front ();
	}
}


void ExecutionProfiler::pushParentSection(uint32 sectionId) {
	if (!m_enableProfiling)
		return;

	// Lock the list
	boost::mutex::scoped_lock lock (m_profileLogMutex);
	m_sectionStack.push (sectionId);
}


bool ExecutionProfiler::startLog(const string& filename) {
	m_filename = filename;
	
	// Lock the list
	boost::mutex::scoped_lock lock (m_profileLogMutex);

	// Check if we have a filename
	if (m_filename.length () > 0) {

		// Try to open the log file
		m_logfile.open (m_filename.c_str ());
		if (m_logfile.bad() || m_logfile.fail ()) {
			WRITE_LOG_ERROR (m_console, "[ExecutionProfiler] ERROR: Can't open console log file " << m_filename << "!");
			return false;
		}

		WRITE_LOG_DEBUG (m_console, "[ExecutionProfiler] Opened profiling log file " << m_filename << "!");

        {
            boost::mutex::scoped_lock lock (m_console->getStreamMutex());
            m_logfile << "SectionID" << ", " << "Start" << ", " << "End" << ", " << "Duration" << ", " << "Action" << ", " << "Complexity" << ", " << "ParentSectionID" << endl;
        }

		m_enableProfiling = true;
		return true;

	} else {

		// We didn't get a filename so spread the information about that.
		WRITE_LOG_ERROR (m_console, "[ExecutionProfiler]  ERROR: Empty log file name!");
		return false;
	}
}


uint32 ExecutionProfiler::startSection(uint16 actionCode, uint32 complexityParameter, uint32 parentSectionId/* = 0 */) {
	if (!m_enableProfiling)
		return 0;

	// Lock the list
	boost::mutex::scoped_lock lock (m_profileLogMutex);

	// Automatically set parent
	uint32 usedParentSectionId = parentSectionId;
	if (parentSectionId == 0) {
		if (m_sectionStack.size () > 0) {
			usedParentSectionId = m_sectionStack.top ();
		}
	}

	// Create the entry and store it
	ExecutionSection s (actionCode, complexityParameter, usedParentSectionId);
	s.startTime = RakNet::GetTime ();
	s.endTime = 0;
	s.sectionId = m_nextSectionId;
	m_sectionMap.insert (make_pair (s.sectionId, s));
	m_nextSectionId++;

	//WRITE_LOG_FULLDEBUG (m_console, "[ExecutionProfiler] Started section " << s.sectionId << ".");

	return s.sectionId;
}

void ExecutionProfiler::logInstructionTime (const string& name, uint32 time) {
	uint32 a = 0;
	if (m_instructionTimings.find(name) != m_instructionTimings.end())
		a = m_instructionTimings.at(name);
	a += time;
	m_instructionTimings[name] = a;
}

void ExecutionProfiler::dumpInstructionTimings (const string& filename) {
	ofstream f (filename.c_str());
	f << "Op\tTime" << endl;
	BOOST_FOREACH(timingmap::value_type i, m_instructionTimings) {
		f << i.first<< "\t" <<i.second<< endl;
	}
	f.close ();
	WRITE_LOG_NORMAL(m_console, "Logged script execution profile to " << filename << ".");
}
