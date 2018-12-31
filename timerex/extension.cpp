/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include <vector>
#include "extension.h"

 /**
  * @file extension.cpp
  * @brief Implement extension code here.
  */

Extension extension;    /**< Global singleton for extension's main interface */

SMEXT_LINK(&extension);

struct TimerInfo
{
  IPluginFunction *Hook;
  IPluginContext *pContext;
  int UserData;
  int Flags;
};

std::vector<TimerInfo> sTimerVector;
IThreadHandle *pTimerThread;

static cell_t CreateTimerEx(IPluginContext *pCtx, const cell_t *params)
{
  IPluginFunction *pFunc;
  TimerInfo Info;
  int flags = params[3];

  pFunc = pCtx->GetFunctionById(params[1]);
  if (!pFunc)
  {
    return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
  }

  Info.UserData = params[2];
  Info.Flags = flags;
  Info.Hook = pFunc;
  Info.pContext = pCtx;

  sTimerVector.push_back(Info);

  return 1;
}

const sp_nativeinfo_t MyNatives[] = {
  {"CreateTimerEx", CreateTimerEx},
  {NULL, NULL},
};

void Extension::RunThread(IThreadHandle *pHandle) {
  while (true) {
    rootconsole->ConsolePrint("test");
  }
}
void Extension::OnTerminate(IThreadHandle *pHandle, bool cancel) {

}

bool Extension::SDK_OnLoad(char *error, size_t maxlen, bool late) {
  sTimerVector.reserve(100);

  ThreadParams params;
  params.flags = Thread_Default;
  params.prio = ThreadPrio_Maximum;
  pTimerThread = threader->MakeThread(this, &params);

  return true;
}

void Extension::SDK_OnUnload() {
  if (pTimerThread != NULL)
  {
    pTimerThread->DestroyThis();
  }
}

void Extension::SDK_OnAllLoaded() {
  sharesys->AddNatives(myself, MyNatives);
}
