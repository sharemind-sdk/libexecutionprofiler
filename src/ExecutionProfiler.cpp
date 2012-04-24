/*
 * This file is a part of the Sharemind framework.
 * Copyright (C) Cybernetica AS
 *
 * All rights are reserved. Reproduction in whole or part is prohibited
 * without the written consent of the copyright owner. The usage of this
 * code is subject to the appropriate license agreement.
 */

#include <GetTime.h> /// \todo Fix RakNet include path!
#include <iostream>
#include "ExecutionProfiler.h"
#include "Logger/Logger.h"


using std::endl;
using std::ofstream;
using std::make_pair;
using std::string;
using std::map;

using namespace sharemind;

ExecutionSection::ExecutionSection(ProfilerActionCode actionCode, size_t complexityParameter, uint32_t parentSectionId)
  : actionCode (actionCode)
  , complexityParameter (complexityParameter)
  , parentSectionId (parentSectionId)
{
}


ExecutionProfiler::ExecutionProfiler(Logger& logger)
  : m_logger (logger)
  , m_nextSectionId (0)
  , m_profilingActive (false)
{
}


ExecutionProfiler::~ExecutionProfiler()
{
    finishLog();
}

bool ExecutionProfiler::startLog(const string& filename)
{
    m_filename = filename;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);

    // Check if we have a filename
    if (m_filename.length () > 0) {

        // Try to open the log file
        m_logfile.open (m_filename.c_str ());
        if (m_logfile.bad() || m_logfile.fail ()) {
            WRITE_LOG_ERROR (m_logger, "[ExecutionProfiler] ERROR: Can't open logger log file " << m_filename << "!");
            return false;
        }

        WRITE_LOG_DEBUG (m_logger, "[ExecutionProfiler] Opened profiling log file " << m_filename << "!");

        {
//            boost::mutex::scoped_lock lock (m_logger->getStreamMutex());
            m_logfile << "SectionID" << ", " << "Start" << ", " << "End"
                      << ", " << "Duration" << ", " << "Action" << ", "
                      << "Complexity" << ", " << "ParentSectionID" << endl;
        }

        m_profilingActive = true;
        return true;

    } else {

        // We didn't get a filename so spread the information about that.
        WRITE_LOG_ERROR (m_logger, "[ExecutionProfiler]  ERROR: Empty log file name!");
        return false;
    }
}

void ExecutionProfiler::finishLog()
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);

    WRITE_LOG_DEBUG (m_logger, "[ExecutionProfiler] Flushing profiling log file.");

    // Flush all sections to the disc
    while (m_sections.size () > 0) {
        // Give time in one-second slices
        ExecutionSection s = m_sections.front ();
        //WRITE_LOG_FULLDEBUG (m_logger, "[ExecutionProfiler] Logging section " << s.sectionId << ".");
        {
//            boost::mutex::scoped_lock lock (m_logger->getStreamMutex());
            m_logfile << s.sectionId << ", " << s.startTime << ", " << s.endTime
                      << ", " << (s.endTime - s.startTime) << ", "
                      << s.actionCode << ", " << s.complexityParameter
                      << ", " << s.parentSectionId << endl;
        }
        m_sections.pop_front ();
    }

    // Close the log file, if necessary
    if (m_logfile.is_open ()) {
        WRITE_LOG_DEBUG (m_logger, "[ExecutionProfiler] Closing log file " << m_filename);
        m_logfile.close ();
    }

    m_profilingActive = false;
}

void ExecutionProfiler::processLog(uint32_t timeLimitMs, bool flush)
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);

    uint64_t start = RakNet::GetTime ();
    uint64_t end = start + timeLimitMs;

    size_t leaveSections = 0;

    while (RakNet::GetTime () < end && m_sections.size () > leaveSections) {
        ExecutionSection s = m_sections.front ();
        //WRITE_LOG_FULLDEBUG (m_logger, "[ExecutionProfiler] Logging section " << s.sectionId << ".");
        {
//            boost::mutex::scoped_lock lock (m_logger->getStreamMutex());
            if (m_logfile.is_open ())
                m_logfile << s.sectionId << ", " << s.startTime << ", "
                          << s.endTime << ", " << (s.endTime - s.startTime)
                          << ", " << s.actionCode << ", " << s.complexityParameter
                          << ", " << s.parentSectionId << endl;
        }
        m_sections.pop_front ();
    }
}

uint32_t ExecutionProfiler::startSection(ProfilerActionCode actionCode, size_t complexityParameter, uint32_t parentSectionId/* = 0 */)
{
    if (!m_profilingActive)
        return 0;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);

    // Automatically set parent
    uint32_t usedParentSectionId = parentSectionId;
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

    //WRITE_LOG_FULLDEBUG (m_logger, "[ExecutionProfiler] Started section " << s.sectionId << ".");

    return s.sectionId;
}

void ExecutionProfiler::endSection(uint32_t sectionId)
{
	if (!m_profilingActive)
		return;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);

	map<uint32_t, ExecutionSection>::iterator it = m_sectionMap.find (sectionId);
	if (it == m_sectionMap.end ()) {
		WRITE_LOG_ERROR (m_logger, "[ExecutionProfiler] Could not end section " << sectionId << ". Not in queue.");
		return;
	}

	it->second.endTime = RakNet::GetTime ();
	m_sections.push_back (it->second);
	m_sectionMap.erase (it);
}

void ExecutionProfiler::pushParentSection(uint32_t sectionId)
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);
    m_sectionStack.push (sectionId);
}

void ExecutionProfiler::popParentSection()
{
	if (!m_profilingActive)
		return;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);

	if (m_sectionStack.size () > 0)
		m_sectionStack.pop ();
}

void ExecutionProfiler::logInstructionTime (const string& name, uint64_t time)
{
	uint64_t a = 0;
	if (m_instructionTimings.find(name) != m_instructionTimings.end())
		a = m_instructionTimings.at(name);
	a += time;
	m_instructionTimings[name] = a;
}

void ExecutionProfiler::dumpInstructionTimings (const string& filename)
{
	ofstream f (filename.c_str());
	f << "Op\tTime" << endl;
	BOOST_FOREACH(timingmap::value_type i, m_instructionTimings) {
		f << i.first<< "\t" <<i.second<< endl;
	}
	f.close ();
	WRITE_LOG_NORMAL(m_logger, "Logged script execution profile to " << filename << ".");
}

ExecutionSectionScope::ExecutionSectionScope(ExecutionProfiler& profiler, uint32_t sectionId, bool isParent)
  : m_profiler (profiler)
  , m_sectionId (sectionId)
  , m_isParent (isParent)
{
}

ExecutionSectionScope::~ExecutionSectionScope()
{
    if (m_isParent) {
        POP_PARENT_SECTION(m_profiler)
    }
    END_SECTION(m_profiler, m_sectionId)
}
