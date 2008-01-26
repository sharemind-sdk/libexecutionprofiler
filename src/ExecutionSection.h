/*
* This file is a part of the Sharemind framework.
* 
* Copyright (C) Dan Bogdanov, 2006-2007
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
*/
#ifndef H_fedb0bcd46e8f6fdfb1e783f97e9c3f0_H
#define H_fedb0bcd46e8f6fdfb1e783f97e9c3f0_H



//BEGIN_USER_SECTION_BEFORE_CLASS_DECLARATION

//END_USER_SECTION_BEFORE_CLASS_DECLARATION


/**
 This is a data structure for storing executed sections for profiling purposes.
 
 This class is used internally by ExecutionProfiler.
*/
struct ExecutionSection
{
// constructors:
/**
 Constructs an execution section based on the given parameters
	 
 \param[in] actionCode a code describing what the section does (\see the ActionCodes enum)
 \param[in] locationCode a code describing where the section is (\see the LocationCodes enum)
 \param[in] complexityParameter a number describing the O(n) complexity of the section
 \param[in] parentSectionId the identifier of a section which contains this one
*/
public:
ExecutionSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId);


// members:

/**
 Stores the action code of the section
*/
public:
uint16 actionCode;
/**
 Stores the location code of the section
*/
public:
uint16 locationCode;
/**
 The O(n) complexity parameter for the section
*/
public:
uint32 complexityParameter;
/**
 A timestamp for the moment the section was completed
*/
public:
uint32 endTime;
/**
 The identifier of the parent section containing this one (zero, if none)
*/
public:
uint32 parentSectionId;
/**
 A timestamp for the moment the section started
*/
public:
uint32 startTime;


//methods:


//child groups:


//child classes:


};

//BEGIN_USER_SECTION_AFTER_CLASS_DECLARATION

//END_USER_SECTION_AFTER_CLASS_DECLARATION


#endif // H_fedb0bcd46e8f6fdfb1e783f97e9c3f0_H

#ifdef OBJECTS_BUILDER_PROJECT_INLINES
#ifndef H_fedb0bcd46e8f6fdfb1e783f97e9c3f0_INLINES_H
#define H_fedb0bcd46e8f6fdfb1e783f97e9c3f0_INLINES_H

#endif // H_fedb0bcd46e8f6fdfb1e783f97e9c3f0_INLINES_H
#endif //OBJECTS_BUILDER_PROJECT_INLINES
