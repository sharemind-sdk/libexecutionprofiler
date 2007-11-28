/*
* This file is a part of the Sharemind framework.
* 
* Copyright (C) Dan Bogdanov, 2006-2007
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
*/


//BEGIN_USER_SECTION_BEFORE_MASTER_INCLUDE

//END_USER_SECTION_BEFORE_MASTER_INCLUDE


#include "Sharemind.h"

//BEGIN_USER_SECTION_AFTER_MASTER_INCLUDE
#include "GetTime.h"
//END_USER_SECTION_AFTER_MASTER_INCLUDE


std::ofstream ExecutionProfiler::logfile;
std::string ExecutionProfiler::filename;
std::vector<ExecutionSection> ExecutionProfiler::sections;
uint32 ExecutionProfiler::sectionOffset = 0;


void ExecutionProfiler::EndSection(uint32 sectionId)
{//BEGIN_461b9ef0616e26d0fabf96bae6b144bc
	if ((sectionId - sectionOffset) >= sections.size ()) {
		WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Can not find section to end. Requested section ID " << sectionId << " with offset " << sectionOffset << ".");
		return;
	}
		
	sections[sectionId - sectionOffset].endTime = RakNet::GetTime ();
	WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Completed section " << sectionId << ".");
}//END_461b9ef0616e26d0fabf96bae6b144bc

void ExecutionProfiler::FinishLog()
{//BEGIN_a7be3781d70e9689d8f1f5f3ade8dfd2

	WRITE_TO_LOG (LOG_NORMAL, "[ExecutionProfiler] Flushing profiling log file.");
	
	// Flush all sections to the disc
	while (sections.size () > 0) {
		// Give time in one-second slices
		if (!ProcessLog (1000)) {
			WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] ERROR: Writing log file failed!");
			break;
		}
	}
	
	// Close the log file, if necessary
	if (logfile.is_open ()) {
		WRITE_TO_LOG (LOG_NORMAL, "[ExecutionProfiler] Closing log file " << filename);
		logfile.close ();
	}
}//END_a7be3781d70e9689d8f1f5f3ade8dfd2

bool ExecutionProfiler::ProcessLog(uint32 timeLimitMs/* = 10 */)
{//BEGIN_7e51adf7891fed3037a150ab9104c2d0
	uint32 start = RakNet::GetTime ();
	uint32 end = start + timeLimitMs;
	
	while (RakNet::GetTime () < end && sections.size () > 0) {
		ExecutionSection s = sections.front ();
		//WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Logging section " << sectionOffset << ".");
		logfile << sectionOffset << ", " << s.startTime << ", " << s.endTime << ", " << (s.endTime - s.startTime) << ", " << s.actionCode << ", " << s.locationCode << ", " << s.complexityParameter << ", " << s.parentSectionId << std::endl;
		sections.erase (sections.begin ());
		sectionOffset++;
	}
	
	return true;
}//END_7e51adf7891fed3037a150ab9104c2d0

bool ExecutionProfiler::StartLog(std::string filename)
{//BEGIN_8194e0626e16b391544cf5092e337a7c
	// Check if we have a filename
	if (filename.length () > 0) {
    
		// Try to open the log file
		logfile.open (filename.c_str ());
		if (logfile.bad() || logfile.fail ()) {
			WRITE_TO_LOG (LOG_MINIMAL, "[ExecutionProfiler] ERROR: Can't open console log file " << filename << "!");
			return false;
		}
		
    WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] Opened profiling log file " << filename << "!");
		logfile << "Section ID" << ", " << "Start" << ", " << "End" << ", " << "Duration" << ", " << "Action" << ", " << "Location" << ", " << "Complexity" << ", " << "Parent Section ID" << std::endl;
		sectionOffset = 1;
		
    return true;
    
  } else {
		
    // We didn't get a filename so spread the information about that.
    WRITE_TO_LOG (LOG_MINIMAL, "[Console] ERROR: Empty log file name!");
    return false;
  }
}//END_8194e0626e16b391544cf5092e337a7c

uint32 ExecutionProfiler::StartSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId/* = 0 */)
{//BEGIN_a91c321aa0fe27343e81047a096c8e30

	if (parentSectionId > sectionOffset + (sections.size () - 1)) {
		WRITE_TO_LOG (LOG_DEBUG, "[ExecutionProfiler] WARNING: The specified parent section " << parentSectionId << " does not exist!");
	}
	
	// Create the entry and store it 
	ExecutionSection s (actionCode, locationCode, complexityParameter, parentSectionId);
	s.startTime = RakNet::GetTime ();
	s.endTime = 0;
	sections.push_back (s);

	uint32 sectionId = (sections.size () - 1) + sectionOffset;
	WRITE_TO_LOG (LOG_FULLDEBUG, "[ExecutionProfiler] Started section " << sectionId << " (" << actionCode << ", " << locationCode << ", " << complexityParameter << ", " << parentSectionId << ").");

	return sectionId;
}//END_a91c321aa0fe27343e81047a096c8e30


//BEGIN_USER_SECTION_AFTER_GENERATED_CODE

//END_USER_SECTION_AFTER_GENERATED_CODE