/*
* This file is a part of the Sharemind framework.
* 
* Copyright (C) Dan Bogdanov, 2006-2007
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
*/
#ifndef H_5688d64798958aedfa1aebedd7670125_H
#define H_5688d64798958aedfa1aebedd7670125_H



//BEGIN_USER_SECTION_BEFORE_CLASS_DECLARATION

enum ActionCodes {

	// NetworkNode actions
	ACTION_SEND_VECTOR = 1,
	ACTION_FLUSH_RESEND_QUEUE = 2,
	ACTION_CONFIRM_VECTOR_DELIVERY = 3,
	ACTION_FILL_OUTBOUND_BUFFER = 4,
	ACTION_FLUSH_OUTBOUND_BUFFER = 5,
	ACTION_RECEIVE_VECTOR = 6
};

enum LocationCodes {
	
	// Classes
	LOCATION_NETWORKNODE_SENDVECTOR = 1,
	LOCATION_NETWORKNODE_RECEIVEVECTOR = 2
	
	// Protocol rounds
};
//END_USER_SECTION_BEFORE_CLASS_DECLARATION


/**

*/
class ExecutionProfiler
{
// constructors:


// members:

private:
static std::ofstream logfile;
private:
static std::string filename;
private:
static std::vector<ExecutionSection> sections;
private:
static uint32 sectionOffset;


//methods:

public:
static void EndSection(uint32 sectionId);

public:
static void FinishLog();

private:
static bool ProcessLog(uint32 timeLimitMs = 10);

public:
static bool StartLog(std::string filename);

public:
static uint32 StartSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId = 0);



//child groups:


//child classes:


};

//BEGIN_USER_SECTION_AFTER_CLASS_DECLARATION

//END_USER_SECTION_AFTER_CLASS_DECLARATION


#endif // H_5688d64798958aedfa1aebedd7670125_H

#ifdef OBJECTS_BUILDER_PROJECT_INLINES
#ifndef H_5688d64798958aedfa1aebedd7670125_INLINES_H
#define H_5688d64798958aedfa1aebedd7670125_INLINES_H

#endif // H_5688d64798958aedfa1aebedd7670125_INLINES_H
#endif //OBJECTS_BUILDER_PROJECT_INLINES
