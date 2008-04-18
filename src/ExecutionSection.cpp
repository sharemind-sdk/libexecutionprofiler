/*
* This file is a part of the Sharemind framework.
* 
* Copyright (C) Dan Bogdanov, 2006-2008
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
*/


//BEGIN_USER_SECTION_BEFORE_MASTER_INCLUDE

//END_USER_SECTION_BEFORE_MASTER_INCLUDE


#include "Sharemind.h"

//BEGIN_USER_SECTION_AFTER_MASTER_INCLUDE

//END_USER_SECTION_AFTER_MASTER_INCLUDE


ExecutionSection::ExecutionSection(uint16 actionCode, uint16 locationCode, uint32 complexityParameter, uint32 parentSectionId)

{//BEGIN_24fd900aed1a2990f1156138771594c8
	this->actionCode = actionCode;
	this->locationCode = locationCode;
	this->complexityParameter = complexityParameter;
	this->parentSectionId = parentSectionId;
}//END_24fd900aed1a2990f1156138771594c8


//BEGIN_USER_SECTION_AFTER_GENERATED_CODE

//END_USER_SECTION_AFTER_GENERATED_CODE