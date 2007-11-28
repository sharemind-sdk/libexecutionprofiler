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

*/
struct ExecutionSection
{
// constructors:
public:
ExecutionSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId);


// members:

public:
uint16 actionCode;
public:
uint16 locationCode;
public:
uint32 complexityParameter;
public:
uint32 endTime;
public:
uint32 parentSectionId;
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