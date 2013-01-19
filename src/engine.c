/*
 * engine.c
 * By Steven Smith
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sst.h"
#include "gamelogic.h"
#include "renderer.h"
#include "drawset.h"
#include "tinycthread.h"

static const char *shaders[] = {"shaders/test3.vert", "shaders/test3.frag"};
static const int shader_count = 2;

#define TRIANGLE_COUNT 12
#define QUAD_COUNT 6
#define VERTS_PER_QUAD 4
#define VERTS_PER_TRIANGLE 3
#define VERT_COUNT (TRIANGLE_COUNT * VERTS_PER_TRIANGLE)

static GLfloat simpleQuad[] = {  50.0f,  50.0f, 0.0f,
                                -50.0f,  50.0f, 0.0f,
                                -50.0f, -50.0f, 0.0f,
                                 50.0f, -50.0f, 0.0f };

static GLfloat simpleQuadNorms[] = { 0.0f, 0.0f, 1.0f,
                                     0.0f, 0.0f, 1.0f,
                                     0.0f, 0.0f, 1.0f,
                                     0.0f, 0.0f, 1.0f };

static GLubyte simpleQuadIndices[] = { 0, 1, 2,
                                       0, 2, 3 };

/* Setup */

/* Struct for passing args into the gamelogic thread */
struct gamelogicArgs {
    GLFWwindow window;
    sstDrawableSet *set;
};

static int rendererSetup( GLFWwindow window, sstProgram *program ) {
    return renderLoop(window, program);
}

static int gameLogicSetup( void *arguments ) {
    struct gamelogicArgs *args;
    sgfeEntity player;
    sgfeEntityList side[4];
    args = (struct gamelogicArgs*)arguments;
    player.pos[0] = 0.0f;
    player.pos[1] = 0.0f;
    player.pos[2] = 0.0f;
    player.rot[0] = 0.0f;
    player.rot[1] = 0.0f;
    player.rot[2] = 0.0f;
    player.set = NULL;
    player.vel[0] = 0.0f;
    player.vel[1] = 0.0f;
    player.vel[2] = 0.0f;
    side[0].entity.pos[0] = -50.0f;
    side[0].entity.pos[1] = 0.0f;
    side[0].entity.pos[2] = 0.0f;
    side[0].entity.rot[0] = 0.0f;
    side[0].entity.rot[1] = M_PI / 2;
    side[0].entity.rot[2] = 0.0f;
    side[0].entity.set = args->set;
    side[0].entity.vel[0] = 0.0f;
    side[0].entity.vel[1] = 0.0f;
    side[0].entity.vel[2] = 0.0f;
    side[0].next = &side[1];
    side[1].entity.pos[0] = 50.0f;
    side[1].entity.pos[1] = 0.0f;
    side[1].entity.pos[2] = 0.0f;
    side[1].entity.rot[0] = 0.0f;
    side[1].entity.rot[1] = M_PI / -2;
    side[1].entity.rot[2] = 0.0f;
    side[1].entity.set = args->set;
    side[1].entity.vel[0] = 0.0f;
    side[1].entity.vel[1] = 0.0f;
    side[1].entity.vel[2] = 0.0f;
    side[1].next = &side[2];
    side[2].entity.pos[0] = 0.0f;
    side[2].entity.pos[1] = -50.0f;
    side[2].entity.pos[2] = 0.0f;
    side[2].entity.rot[0] = M_PI / -2;
    side[2].entity.rot[1] = 0.0f;
    side[2].entity.rot[2] = 0.0f;
    side[2].entity.set = args->set;
    side[2].entity.vel[0] = 0.0f;
    side[2].entity.vel[1] = 0.0f;
    side[2].entity.vel[2] = 0.0f;
    side[2].next = &side[3];
    side[3].entity.pos[0] = 0.0f;
    side[3].entity.pos[1] = 50.0f;
    side[3].entity.pos[2] = 0.0f;
    side[3].entity.rot[0] = M_PI / 2;
    side[3].entity.rot[1] = 0.0f;
    side[3].entity.rot[2] = 0.0f;
    side[3].entity.set = args->set;
    side[3].entity.vel[0] = 0.0f;
    side[3].entity.vel[1] = 0.0f;
    side[3].entity.vel[2] = 0.0f;
    side[3].next = NULL;
    return gameLoop(args->window, &player, NULL, side);
}

static int threadSetup( GLFWwindow window, sstProgram *program, sstDrawableSet *set ) {
    thrd_t logicThread;
    struct gamelogicArgs args;
    args.window = window;
    args.set = set;
    sgfeInitDrawBuffers(2);
    thrd_create(&logicThread, gameLogicSetup, &args);
    rendererSetup(window, program);
    thrd_join(logicThread, NULL);
    return 0;
}

static int dataSetup( GLFWwindow window ) {
    sstProgram *program;
    sstDrawableSet *set;
    int result;
    /* Create shader program */
    program = sstNewProgram(shaders, shader_count);
    if( !program ) {
        printf("Failed to start: couldn't create program!\n");
        return 1;
    }
    /* Set up data */
    set = sstDrawableSetElements(program, GL_TRIANGLES, 4, simpleQuadIndices,
                                 GL_UNSIGNED_BYTE, 3 * 2,
                                 "in_Position", simpleQuad,
                                 "in_Normal", simpleQuadNorms);
    result = threadSetup(window, program, set);
    return result;
}

int glfwContext() {
    GLFWwindow window;
    int result;
    window = sgfeCreateWindow();
    if( window ) {
        result = dataSetup(window);
    }
    else {
        result = 1;
    }
    glfwTerminate();
    return result;
}

int initContext() {
    if( !glfwInit() ) {
        printf("Failed to init GLFW!\n");
        return 1;
    }
    if( glfwContext() ) {
        return 1;
    }
    else {
        return 0;
    }
}

int main() {
    if( initContext() ) {
        exit(EXIT_FAILURE);
    }
    else {
        exit(EXIT_SUCCESS);
    }
    return 0;
}
