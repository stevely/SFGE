/*
 * gamelogic.c
 * By Steven Smith
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gamelogic.h"

void rotateMovement( GLfloat x, GLfloat y, GLfloat rot, GLfloat *dx, GLfloat *dy ) {
    GLfloat sinRot, cosRot;
    sinRot = sin(rot);
    cosRot = cos(rot);
    *dx += (x * cosRot) - (y * sinRot);
    *dy += (y * cosRot) - (x * sinRot);
}

static int isQuitting( GLFWwindow window ) {
    return glfwGetKey(window, GLFW_KEY_ESC)
        || glfwGetWindowParam(window, GLFW_CLOSE_REQUESTED);
}

static void handleMouseLook( GLFWwindow window, GLfloat *rot ) {
    static GLfloat mX  = -1;
    static GLfloat mY  = -1;
    static GLfloat mXl = -1;
    static GLfloat mYl = -1;
    int x;
    int y;
    if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ) {
        if( mXl == -1 ) {
            glfwGetCursorPos(window, &x, &y);
            mXl = (GLfloat)x;
            mYl = (GLfloat)y;
        }
        else {
            mXl = mX;
            mYl = mY;
        }
        glfwGetCursorPos(window, &x, &y);
        mX = (GLfloat)x;
        mY = (GLfloat)y;
    }
    else if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE ) {
        mX = mY = mXl = mYl = -1;
    }
    rot[0] += (mX - mXl) / 30.0f;
    rot[1] += (mY - mYl) / 30.0f;
}

static void handleKeyboard( GLFWwindow window, GLfloat *vel, GLfloat *rot ) {
    GLfloat v[3], sinRot, cosRot;
    if( glfwGetKey(window, 'W') == GLFW_PRESS ) {
        v[2] = -50.0f;
    }
    else if( glfwGetKey(window, 'S') == GLFW_PRESS ) {
        v[2] =  50.0f;
    }
    else {
        v[2] =  0.0f;
    }
    v[1] = 0.0f;
    if( glfwGetKey(window, 'A') == GLFW_PRESS ) {
        v[0] = -50.0f;
    }
    else if( glfwGetKey(window, 'D') == GLFW_PRESS ) {
        v[0] =  50.0f;
    }
    else {
        v[0] =  0.0f;
    }
    /* Scale in case of diagonal movement */
    if( v[0] != 0.0f && v[2] != 0.0f ) {
        v[0] /= 1.4f; /* Note: Inexact, causes diagonal movement to be */
        v[2] /= 1.4f; /* slightly too fast */
    }
    sinRot = sin(rot[0]);
    cosRot = cos(rot[0]);
    vel[0] = (v[0] * cosRot) - (v[2] * sinRot);
    vel[2] = (v[2] * cosRot) + (v[0] * sinRot);
}

static void updateDynamicEntities( sgfeEntityList *dynamics ) {
    /* TODO */
    if( dynamics ) {}
}

int gameLoop( GLFWwindow window, sgfeEntity *player, sgfeEntityList *dynamics,
sgfeEntityList *statics ) {
    double lastTime, currentTime, elapsedTime;
    sgfeRenderBuffers *buffer;
    sgfeDrawable *ds;
    sgfeEntityList *e;
    int buf_size;
    buffer = sgfeGetProducerBuffer();
    lastTime = glfwGetTime();
    while( buffer ) {
        /* Step 1: Set up the clock */
        currentTime = glfwGetTime();
        elapsedTime = currentTime - lastTime;
        lastTime = currentTime;
        /* Step 2: Handle input */
        /*glfwPollEvents();*/
        if( isQuitting(window) ) {
            sgfeSignalExit();
        }
        handleMouseLook(window, player->rot);
        handleKeyboard(window, player->vel, player->rot);
        /* Step 3: Let dynamic entities update velocity */
        updateDynamicEntities(dynamics);
        /* Step 4: Collision detection */
        /* TODO */
        /* Step 5: Update positions */
        player->pos[0] += player->vel[0] * elapsedTime;
        player->pos[1] += player->vel[1] * elapsedTime;
        player->pos[2] += player->vel[2] * elapsedTime;
        /* TODO: Update dynamic entiies */
        /* Step 6: Populate drawable buffer */
        /* Camera (ie. the player) is always the first entry */
        sgfeResetDrawables(buffer);
        ds = sgfeNextDrawable(buffer);
        ds->pos[0] = player->pos[0];
        ds->pos[1] = player->pos[1];
        ds->pos[2] = player->pos[2];
        ds->rot[0] = player->rot[0];
        ds->rot[1] = player->rot[1];
        ds->rot[2] = player->rot[2];
        ds->set = NULL;
        buf_size = 1;
        /* Dynamic entities */
        for( e = dynamics; e; e = e->next ) {
            ds = sgfeNextDrawable(buffer);
            ds->pos[0] = e->entity.pos[0];
            ds->pos[1] = e->entity.pos[1];
            ds->pos[2] = e->entity.pos[2];
            ds->rot[0] = e->entity.rot[0];
            ds->rot[1] = e->entity.rot[1];
            ds->rot[2] = e->entity.rot[2];
            ds->set = e->entity.set;
            buf_size++;
        }
        /* Static entities */
        for( e = statics; e; e = e->next ) {
            ds = sgfeNextDrawable(buffer);
            ds->pos[0] = e->entity.pos[0];
            ds->pos[1] = e->entity.pos[1];
            ds->pos[2] = e->entity.pos[2];
            ds->rot[0] = e->entity.rot[0];
            ds->rot[1] = e->entity.rot[1];
            ds->rot[2] = e->entity.rot[2];
            ds->set = e->entity.set;
            buf_size++;
        }
        /* TODO: Lights */
        /* Step 7: Swap buffers */
        buffer = sgfeSwapBuffers(buffer);
    }
    return 0;
}
