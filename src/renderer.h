/*
 * renderer.h
 * By Steven Smith
 */

#ifndef RENDERER_H_
#define RENDERER_H_

#include "sst.h"
#include "drawset.h"

GLFWwindow sgfeCreateWindow();

int renderLoop( GLFWwindow window, sstProgram *naked, sstProgram *point );

#endif
