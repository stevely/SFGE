/*
 * engine.c
 * By Steven Smith
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sst.h"
#include "sdf.h"
#include "gamelogic.h"
#include "renderer.h"
#include "drawset.h"
#include "tinycthread.h"

#define TRIANGLE_COUNT 12
#define QUAD_COUNT 6
#define VERTS_PER_QUAD 4
#define VERTS_PER_TRIANGLE 3
#define VERT_COUNT (TRIANGLE_COUNT * VERTS_PER_TRIANGLE)

/*
static const char *shaders[] = {"shaders/test3.vert", "shaders/test3.frag"};
static const int shader_count = 2;

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
*/

/* Setup */

static int rendererSetup( GLFWwindow window, sstProgram *program ) {
    return renderLoop(window, program);
}

/* Struct for passing args into the gamelogic thread */
struct gamelogicArgs {
    GLFWwindow window;
    sstDrawableSet **sets;
};

static int gameLogicSetup( void *arguments ) {
    struct gamelogicArgs *args;
    sgfeEntity player;
    sgfeEntityList side[4];
    sstDrawableSet *set;
    args = (struct gamelogicArgs*)arguments;
    set = args->sets[0];
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
    side[0].entity.set = set;
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
    side[1].entity.set = set;
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
    side[2].entity.set = set;
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
    side[3].entity.set = set;
    side[3].entity.vel[0] = 0.0f;
    side[3].entity.vel[1] = 0.0f;
    side[3].entity.vel[2] = 0.0f;
    side[3].next = NULL;
    return gameLoop(args->window, &player, NULL, side);
}

/* Unused, non-static to prevent compiler errors */
/*
int old_threadSetup( GLFWwindow window, sstProgram *program, sstDrawableSet *set ) {
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
*/

static int threadSetup( GLFWwindow window, sstProgram *program, sdfNode *data,
sstDrawableSet **sets, char **setNames, int setCount) {
    thrd_t logicThread;
    struct gamelogicArgs args;
    /* Unused args */
    if( data ) {}
    if( setNames ) {}
    if( setCount ) {}
    /* End unused args */
    args.window = window;
    args.sets = sets;
    sgfeInitDrawBuffers(2);
    thrd_create(&logicThread, gameLogicSetup, &args);
    rendererSetup(window, program);
    thrd_join(logicThread, NULL);
    return 0;
}

static void populateFloatTriplets( GLfloat *values, sdfNode *data ) {
    char *s;
    while( data ) {
        s = data->data;
        *values = strtof(s, &s);
        values++;
        *values = strtof(s, &s);
        values++;
        *values = strtof(s, &s);
        values++;
        data = data->next;
    }
}

static int countIndices( sdfNode *data ) {
    int result, f;
    char *c;
    for( result = 0; data; data = data->next ) {
        f = 0;
        for( c = data->data; *c; c++ ) {
            if( *c == ' ' || *c == '\n' ) {
                f = 0;
            }
            else if( !f ) {
                result++;
                f = 1;
            }
        }
    }
    return result;
}

static void populateIndices( GLubyte *values, sdfNode *data ) {
    char *c;
    for( ; data; data = data->next ) {
        c = data->data;
        while( *c ) {
            *values = (GLubyte)strtol(c, &c, 10);
            values++;
        }
    }
}

static int extractModelData( sstProgram *program, sdfNode *tree,
sstDrawableSet ***set, char ***names ) {
    sdfNode *node, *model, *subnode;
    int i, count, modelCount, indexCount;
    GLenum type;
    GLfloat *positions, *normals;
    GLubyte *indices; /* TODO: Have to be smarter about this */
    indices = NULL;
    /* Step 1: Get model count */
    node = getChildren(tree, "models");
    model = node;
    for( modelCount = 0; model; model = model->next ) {
        modelCount++;
    }
    /* Step 2: Allocate memory to store the list of sets and names */
    *set = (sstDrawableSet**)malloc(sizeof(sstDrawableSet*) * modelCount);
    *names = (char**)malloc(sizeof(char*) * modelCount);
    /* Step 3: Populate arrays */
    model = node;
    for( i = 0; i < modelCount; i++ ) {
        (*names)[i] = model->data;
        /* Step 3a: Get type */
        subnode = getChildren(model->child, "type");
        if( !subnode ) {
            printf("Model [%s] does not have a type!\n", model->data);
        }
        else {
            if( strcmp(subnode->data, "triangles") == 0 ) {
                type = GL_TRIANGLES;
            }
            else {
                printf("Unknown draw type for model [%s]: %s\n", model->data,
                       subnode->data);
                type = GL_TRIANGLES;
            }
            /* Step 3b: Get positions */
            subnode = getChildren(model->child, "positions");
            /* Count positions */
            for( count = 0; subnode; subnode = subnode->next ) {
                count++;
            }
            positions = (GLfloat*)malloc(sizeof(GLfloat) * (count * 3));
            normals = (GLfloat*)malloc(sizeof(GLfloat) * (count * 3));
            /* Step 3c: Populate data */
            populateFloatTriplets(positions, getChildren(model->child, "positions"));
            populateFloatTriplets(normals, getChildren(model->child, "normals"));
            /* Step 3d: Check for indices */
            subnode = getChildren(model->child, "indices");
            if( subnode ) {
                indexCount = countIndices(subnode);
                indices = (GLubyte*)malloc(sizeof(GLubyte) * indexCount);
                populateIndices(indices, subnode);
            }
            else {
                indices = NULL;
            }
            /* Step 3e: Generate drawable set */
            if( indices ) {
                (*set)[i] = sstDrawableSetElements(program, type, count,
                                                   indices, GL_UNSIGNED_BYTE,
                                                   indexCount,
                                                   "in_Position", positions,
                                                   "in_Normal", normals);
            }
            else {
                (*set)[i] = sstDrawableSetArrays(program, type, count,
                                                 "in_Position", positions,
                                                 "in_Normal", normals);
            }
            free(positions);
            free(normals);
            if( indices ) {
                free(indices);
            }
        }
        model = model->next;
    }
    return modelCount;
}

static char ** getShaders( sdfNode *node, int *count ) {
    char **result;
    sdfNode *n;
    int i;
    n = node;
    /* Count vertex shaders */
    for( *count = 0; n; n = n->next ) {
        (*count)++;
    }
    result = (char**)malloc(sizeof(char*) * (*count));
    /* Populate vertex shader array */
    n = node;
    for( i = 0; i < *count; i++ ) {
        result[i] = n->data;
        n = n->next;
    }
    return result;
}

static sstProgram * extractProgram( sdfNode *tree ) {
    sstProgram *result;
    sdfNode *node, *subnode;
    char **verts, **frags;
    int vCount, fCount;
    node = getChildren(tree, "shaders");
    if( !node ) {
        printf("Found no shader programs!\n");
        return NULL;
    }
    /* Get vertex shaders */
    subnode = getChildren(node, "vertex");
    if( !subnode ) {
        printf("Found no vertex shader programs!\n");
        return NULL;
    }
    verts = getShaders(subnode, &vCount);
    /* Get fragment shaders */
    subnode = getChildren(node, "fragment");
    if( !subnode ) {
        printf("Found no fragment shader programs!\n");
        free(verts);
        return NULL;
    }
    frags = getShaders(subnode, &fCount);
    /* Create program */
    result = sstNewProgramS((const char **)verts, vCount, (const char **)frags,
                            fCount);
    free(verts);
    free(frags);
    return result;
}

/* Unused, non-static to prevent compiler errors */
/*
int old_dataSetup( GLFWwindow window ) {
    sstProgram *program;
    sstDrawableSet *set;
    int result;
    * Create shader program *
    program = sstNewProgram(shaders, shader_count);
    if( !program ) {
        printf("Failed to start: couldn't create program!\n");
        return 1;
    }
    * Set up data *
    set = sstDrawableSetElements(program, GL_TRIANGLES, 4, simpleQuadIndices,
                                 GL_UNSIGNED_BYTE, 3 * 2,
                                 "in_Position", simpleQuad,
                                 "in_Normal", simpleQuadNorms);
    result = threadSetup(window, program, set);
    return result;
}
*/

int dataSetup( GLFWwindow window, const char *filepath ) {
    sstProgram *program;
    sstDrawableSet **sets;
    char **setNames;
    int setCount;
    sdfNode *data;
    /* Step 1: Parse SDF file */
    data = parseSdfFile(filepath);
    if( !data ) {
        printf("Failed to start: couldn't parse data file!\n");
        return 1;
    }
    /* Step 2: Create program */
    program = extractProgram(data);
    if( !program ) {
        printf("Failed to start: couldn't create program!\n");
        return 1;
    }
    /* Step 3: Create drawable sets */
    setCount = extractModelData(program, data, &sets, &setNames);
    return threadSetup(window, program, data, sets, setNames, setCount);
}

int glfwContext( char *filepath ) {
    GLFWwindow window;
    int result;
    window = sgfeCreateWindow();
    if( window ) {
        result = dataSetup(window, filepath);
    }
    else {
        result = 1;
    }
    glfwTerminate();
    return result;
}

int initContext( char *filepath ) {
    if( !glfwInit() ) {
        printf("Failed to init GLFW!\n");
        return 1;
    }
    if( glfwContext(filepath) ) {
        return 1;
    }
    else {
        return 0;
    }
}

int main( int argc, char **argv ) {
    if( argc != 2 ) {
        printf("Usage: %s datafile\n", argv[0]);
        return EXIT_SUCCESS;
    }
    if( initContext(argv[1]) ) {
        return EXIT_FAILURE;
    }
    else {
        return EXIT_SUCCESS;
    }
}
