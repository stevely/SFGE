/*
 * gamelogic.h
 * By Steven Smith
 */

#ifndef GAMELOGIC_H_
#define GAMELOGIC_H_

#include "sst.h"
#include "drawset.h"

int gameLoop( GLFWwindow window, sgfeEntity *player, sgfeEntityList *actives,
sgfeEntityList *passives );

#endif
