// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "IAbleCoreSP.h"

#include "AbleCoreSPPrivate.h"

class FAbleCoreSP : public IAbleCoreSP
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FAbleCoreSP, AbleCoreSP)
DEFINE_LOG_CATEGORY(LogAbleSP);

void FAbleCoreSP::StartupModule()
{

}


void FAbleCoreSP::ShutdownModule()
{

}



