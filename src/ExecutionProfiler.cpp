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
#include <sstream>


namespace {

#ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
inline std::string minerNetworkStatistics(
        sharemind::MinerNetworkStatistics & startStats,
        sharemind::MinerNetworkStatistics & endStats)
{
    assert (startStats.size() == endStats.size());

    typedef sharemind::MinerNetworkStatistics::iterator MNIT;
    std::ostringstream o;

    for (MNIT sit = startStats.begin(); sit != startStats.end(); ++sit) {
        MNIT eit = endStats.find(sit->first);
        if (eit != endStats.end()) {
            /// \note The reported byte count can overflow.
            o << "[" << sit->first
              << "," << (eit->second.receivedBytes - sit->second.receivedBytes)
              << "," << (eit->second.sentBytes - sit->second.sentBytes)
              << "] ";
        } else {
            return "";
        }
    }

    return o.str();
}
#endif

}

using std::endl;
using std::make_pair;
using std::ofstream;
using std::string;
using std::map;

namespace sharemind {

ExecutionSection::ExecutionSection(
        const char * sectionName,
        uint32_t sectionId,
        uint32_t parentSectionId,
        UsTime startTime,
        UsTime endTime,
        size_t complexityParameter
        #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
        , const MinerNetworkStatistics & netStats
        #endif
        )
    : sectionId(sectionId)
    , parentSectionId(parentSectionId)
    , startTime(startTime)
    , endTime(endTime)
    , complexityParameter(complexityParameter)
    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
    , startNetworkStatistics(netStats)
    #endif
    , m_sectionName(sectionName)
    , m_nameCached(false)
{
}

ExecutionSection::ExecutionSection(
        uint32_t sectionType,
        uint32_t sectionId,
        uint32_t parentSectionId,
        UsTime startTime,
        UsTime endTime,
        size_t complexityParameter
        #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
        , const MinerNetworkStatistics & netStats
        #endif
        )
    : sectionId(sectionId)
    , parentSectionId(parentSectionId)
    , startTime(startTime)
    , endTime(endTime)
    , complexityParameter(complexityParameter)
    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
    , startNetworkStatistics(netStats)
    #endif
    , m_sectionName(sectionType)
    , m_nameCached(true)
{
}

bool ExecutionProfiler::startLog(const string & filename) {
    assert(!filename.empty());
    m_filename = filename;

    // Lock the list
    std::lock_guard<std::mutex> lock(m_profileLogMutex);

    // Try to open the log file
    m_logfile.open(m_filename.c_str());
    if (m_logfile.bad() || m_logfile.fail()) {
        m_logger.error() << "Can not open profiler log file '" << m_filename
                         << "'!";
        return false;
    }

    m_logger.debug() << "Opened profiling log file '" << m_filename << "'!";

    m_logfile << "Action;"
                 "SectionID;"
                 "ParentSectionID;"
                 "Duration;"
                 "Complexity;"
                 "NetworkStats[miner,in,out];" << endl;

    m_profilingActive = true;
    return true;
}

void ExecutionProfiler::finishLog() {
    if (!m_profilingActive)
        return;

    // Lock the list
    std::lock_guard<std::mutex> lock(m_profileLogMutex);
    __processLog();

    // Close the log file, if necessary
    if (m_logfile.is_open()) {
        m_logger.debug() << "Closing profiler log file '" << m_filename << "'";
        m_logfile.close();
    }

    m_profilingActive = false;
}

void ExecutionProfiler::processLog() {
    if (!m_profilingActive)
        return;

    std::lock_guard<std::mutex> lock(m_profileLogMutex);
    __processLog();
}

void ExecutionProfiler::__processLog() {
    m_logger.debug() << "Writing profiling log file '" << m_filename << "'";

    // Write all sections to the disc
    while (m_sections.size() > 0) {
        __processLog(1000u);
    }
}

void ExecutionProfiler::processLog(uint32_t timeLimitMs) {
    if (!m_profilingActive)
        return;

    // Lock the list
    std::lock_guard<std::mutex> lock(m_profileLogMutex);
    __processLog(timeLimitMs);
}

void ExecutionProfiler::__processLog(uint32_t timeLimitMs) {
    const UsTime end = getUsTime() + timeLimitMs * 1000u;

    while (getUsTime() < end && m_sections.size() > 0u) {
        ExecutionSection * const s = m_sections.front();

        m_logfile << getSectionName(s) << ";"
                  << s->sectionId << ";"
                  << s->parentSectionId << ";"
                  << (s->endTime - s->startTime) << ";"
                  << s->complexityParameter
                  #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
                  << ";" << minerNetworkStatistics(
                                s->startNetworkStatistics,
                                s->endNetworkStatistics)
                  #endif
                  << endl;

        delete s;
        m_sections.pop_front();
    }

    m_logfile.flush();
}

uint32_t ExecutionProfiler::newSectionType(const char * name) {
    assert(name);

    if (!m_profilingActive)
        return 0;

    // Lock the list
    std::lock_guard<std::mutex> lock(m_profileLogMutex);

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

void ExecutionProfiler::endSection(uint32_t sectionId) {
    if (!m_profilingActive)
        return;

    endSection(sectionId,
               getUsTime()
               #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
               , MinerNetworkStatistics()
               #endif
               );
}

void ExecutionProfiler::endSection(
        uint32_t sectionId,
        const UsTime endTime
        #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
        , const MinerNetworkStatistics & endNetStats
        #endif
        )
{
    if (!m_profilingActive)
        return;

    // Lock the list
    std::lock_guard<std::mutex> lock(m_profileLogMutex);

    map<uint32_t, ExecutionSection*>::iterator it = m_sectionMap.find(sectionId);
    if (it == m_sectionMap.end()) {
        m_logger.error() << "Could not end section " << sectionId
                         << ". Not in queue.";
        return;
    }

    it->second->endTime = endTime;
    #ifdef SHAREMIND_NETWORK_STATISTICS_ENABLE
    it->second->endNetworkStatistics = endNetStats;
    #endif
    m_sections.push_back(it->second);
    m_sectionMap.erase(it);
}

void ExecutionProfiler::pushParentSection(uint32_t sectionId) {
    if (!m_profilingActive)
        return;

    // Lock the list
    std::lock_guard<std::mutex> lock(m_profileLogMutex);
    m_parentSectionStack.push(sectionId);
}

void ExecutionProfiler::popParentSection() {
    if (!m_profilingActive)
        return;

    // Lock the list
    std::lock_guard<std::mutex> lock(m_profileLogMutex);

    if (!m_parentSectionStack.empty())
        m_parentSectionStack.pop();
}

} // namespace sharemind {
