// Mega Storage - a high-capacity variant of the Industrial Storage Container Mk.2.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMegaStorage, Log, All);

class FDelegateHandle;

class FMegaStorageModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	FDelegateHandle InventorySizeHookHandle;
};
