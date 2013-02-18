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

/* Setup */

static int rendererSetup( GLFWwindow window, sstProgram *program ) {
    return renderLoop(window, NULL, program);
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

static void populateEntity( sfgeEntity *entity, sdfNode *data,
sstDrawableSet **sets, char **setNames, int setCount ) {
    sdfNode *n;
    int i;
    n = getChildren(data, "model");
    if( n ) {
        /* Model lookup */
        for( i = 0; i < setCount; i++ ) {
            if( strcmp(setNames[i], n->data) == 0 ) {
                break;
            }
        }
        if( i >= setCount ) {
            entity->set = NULL;
        }
        else {
            entity->set = sets[i];
        }
    }
    /* Positions */
    populateFloatTriplets(entity->pos, getChildren(data, "position"));
    /* Rotation */
    populateFloatTriplets(entity->rot, getChildren(data, "rotation"));
    /* Velocity */
    entity->vel[0] = 0.0f;
    entity->vel[1] = 0.0f;
    entity->vel[2] = 0.0f;
}

/* Struct for passing args into the gamelogic thread */
struct gamelogicArgs {
    GLFWwindow window;
    sdfNode *data;
    sstDrawableSet **sets;
    char **setNames;
    int setCount;
};

static int gameLogicSetup( void *arguments ) {
    struct gamelogicArgs *args;
    sfgeEntity player;
    sdfNode *data, *d;
    sstDrawableSet **sets;
    char **setNames;
    int setCount;
    sfgeEntityList *dynamics, *d_end;
    sfgeEntityList *statics, *s_end; /* static's a keyword... */
    dynamics = d_end = statics = s_end = NULL;
    /* Step 1: Get our args */
    args = (struct gamelogicArgs*)arguments;
    data = args->data;
    sets = args->sets;
    setNames = args->setNames;
    setCount = args->setCount;
    /* Step 2: Populate player */
    populateEntity(&player, getChildren(data, "playerStart"), sets, setNames,
                   setCount);
    data = getChildren(data, "entities");
    /* Step 3: Populate dynamics entities */
    for( d = getChildren(data, "dynamic"); d; d = d->next ) {
        if( dynamics == NULL ) {
            dynamics = d_end = (sfgeEntityList*)malloc(sizeof(sfgeEntityList));
            d_end->next = NULL;
        }
        else {
            d_end->next = (sfgeEntityList*)malloc(sizeof(sfgeEntityList));
            d_end = d_end->next;
            d_end->next = NULL;
        }
        populateEntity(&d_end->entity, d->child, sets, setNames, setCount);
    }
    /* Step 4: Populate static entities */
    for( d = getChildren(data, "static"); d; d = d->next ) {
        if( statics == NULL ) {
            statics = s_end = (sfgeEntityList*)malloc(sizeof(sfgeEntityList));
            s_end->next = NULL;
        }
        else {
            s_end->next = (sfgeEntityList*)malloc(sizeof(sfgeEntityList));
            s_end = s_end->next;
            s_end->next = NULL;
        }
        populateEntity(&s_end->entity, d->child, sets, setNames, setCount);
    }
    return gameLoop(args->window, &player, dynamics, statics);
}

static int threadSetup( GLFWwindow window, sstProgram *program, sdfNode *data,
sstDrawableSet **sets, char **setNames, int setCount) {
    thrd_t logicThread;
    struct gamelogicArgs args;
    args.window = window;
    args.data = data;
    args.sets = sets;
    args.setNames = setNames;
    args.setCount = setCount;
    sfgeInitDrawBuffers(2);
    thrd_create(&logicThread, gameLogicSetup, &args);
    rendererSetup(window, program);
    thrd_join(logicThread, NULL);
    return 0;
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
    node = getChildren(node, "point");
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
    window = sfgeCreateWindow();
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
