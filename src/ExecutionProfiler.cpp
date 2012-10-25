/*
 * This file is a part of the Sharemind framework.
 * Copyright (C) Cybernetica AS
 *
 * All rights are reserved. Reproduction in whole or part is prohibited
 * without the written consent of the copyright owner. The usage of this
 * code is subject to the appropriate license agreement.
 */

#include "ExecutionProfiler.h"

#include <cstring>
#include <iostream>
#include "Logger/Debug.h"


namespace { SHAREMIND_DEFINE_PREFIXED_LOGS("[ExecutionProfiler] "); }

using std::endl;
using std::make_pair;
using std::ofstream;
using std::string;
using std::map;

namespace sharemind {

ExecutionSection::ExecutionSection(const char * sectionName,
                                   uint32_t sectionId,
                                   uint32_t parentSectionId,
                                   MicrosecondTimerTime startTime,
                                   MicrosecondTimerTime endTime,
                                   size_t complexityParameter)
    : m_sectionName (sectionName)
    , m_nameCached (false)
    , sectionId (sectionId)
    , parentSectionId (parentSectionId)
    , startTime (startTime)
    , endTime (endTime)
    , complexityParameter (complexityParameter)
{
}

ExecutionSection::ExecutionSection(uint32_t sectionType,
                                   uint32_t sectionId,
                                   uint32_t parentSectionId,
                                   MicrosecondTimerTime startTime,
                                   MicrosecondTimerTime endTime,
                                   size_t complexityParameter)
    : m_sectionName (sectionType)
    , m_nameCached (true)
    , sectionId (sectionId)
    , parentSectionId (parentSectionId)
    , startTime (startTime)
    , endTime (endTime)
    , complexityParameter (complexityParameter)
{
}

ExecutionProfiler::ExecutionProfiler(ILogger & logger)
  : m_logger(logger)
  , m_nextSectionTypeId(0)
  , m_nextSectionId(0)
  , m_profilingActive(false)
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
    boost::mutex::scoped_lock lock(m_profileLogMutex);

    // Check if we have a filename
    if (m_filename.length() > 0) {

        // Try to open the log file
        m_logfile.open(m_filename.c_str());
        if (m_logfile.bad() || m_logfile.fail()) {
            LogError(m_logger) << "Can not open profiler log file '" << m_filename << "'!";
            return false;
        }

        LogDebug(m_logger) << "Opened profiling log file '" << m_filename << "'!";

        m_logfile << "Action;"
                     "SectionID;"
                     "ParentSectionID;"
                     "Duration;"
                     "Complexity" << endl;

        m_profilingActive = true;
        return true;

    } else {

        // We didn't get a filename so spread the information about that.
        LogError(m_logger) << "Empty log file name!";
        return false;
    }
}

void ExecutionProfiler::finishLog()
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock(m_profileLogMutex);

    LogDebug(m_logger) << "Flushing profiling log file.";

    // Flush all sections to the disc
    while (m_sections.size() > 0) {
        __processLog(1000, true);
    }

    // Close the log file, if necessary
    if (m_logfile.is_open()) {
        LogDebug(m_logger) << "Closing prifiler log file '" << m_filename << "'";
        m_logfile.close();
    }

    m_profilingActive = false;
}

void ExecutionProfiler::processLog(uint32_t timeLimitMs, bool flush)
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock(m_profileLogMutex);
    __processLog(timeLimitMs, flush);
}

void ExecutionProfiler::__processLog(uint32_t timeLimitMs, bool flush) {
    MicrosecondTimerTime end = MicrosecondTimer_get_global_time() + timeLimitMs * 1000;

    while (MicrosecondTimer_get_global_time() < end && m_sections.size() > 0) {
        ExecutionSection * s = m_sections.front();
        //LogFullDebug(m_logger) << "[ExecutionProfiler] Logging section " << s.sectionId << ".";

        m_logfile << getSectionName(s) << ";"
                  << s->sectionId << ";"
                  << s->parentSectionId << ";"
                  << (s->endTime - s->startTime) << ";"
                  << s->complexityParameter << endl;

        delete s;
        m_sections.pop_front();
    }

    if (flush)
        m_logfile.flush();
}

uint32_t ExecutionProfiler::newSectionType(const char * name) {
    assert(name);

    if (!m_profilingActive)
        return 0;

    // Lock the list
    boost::mutex::scoped_lock lock(m_profileLogMutex);

    size_t n = strlen(name);

    /// \todo Is it a good idea to reuse duplicate section types?
    for(map<uint32_t, char *>::iterator it = m_sectionTypes.begin();
        it != m_sectionTypes.end(); ++it)
    {
        if (strncmp(it->second, name, n) == 0)
            return it->first;
    }

    char * cname = new char[n + 1];
    strncpy(cname, name, n);
    cname[n] = '\0';

    m_sectionTypes.insert(make_pair(m_nextSectionTypeId, cname));
    return m_nextSectionTypeId++;
}

void ExecutionProfiler::endSection(uint32_t sectionId)
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock(m_profileLogMutex);

    map<uint32_t, ExecutionSection*>::iterator it = m_sectionMap.find(sectionId);
    if (it == m_sectionMap.end()) {
        LogError(m_logger) << "Could not end section " << sectionId << ". Not in queue.";
        return;
    }

    it->second->endTime = MicrosecondTimer_get_global_time();
    m_sections.push_back(it->second);
    m_sectionMap.erase(it);
}

void ExecutionProfiler::pushParentSection(uint32_t sectionId)
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);
    m_parentSectionStack.push(sectionId);
}

void ExecutionProfiler::popParentSection()
{
    if (!m_profilingActive)
        return;

    // Lock the list
    boost::mutex::scoped_lock lock (m_profileLogMutex);

    if (!m_parentSectionStack.empty())
        m_parentSectionStack.pop();
}

} // namespace sharemind {
